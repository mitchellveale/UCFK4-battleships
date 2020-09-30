#include "system.h"
#include "navswitch.h"
#include "ledmat.h"
#include "led.h"
#include "pacer.h"
#include "pio.h"

/** Define PIO pins driving LED matrix rows.  */
static const pio_t rows[] =
        {
                LEDMAT_ROW1_PIO, LEDMAT_ROW2_PIO, LEDMAT_ROW3_PIO,
                LEDMAT_ROW4_PIO, LEDMAT_ROW5_PIO, LEDMAT_ROW6_PIO,
                LEDMAT_ROW7_PIO
        };


/** Define PIO pins driving LED matrix columns.  */
static const pio_t cols[] =
        {
                LEDMAT_COL1_PIO, LEDMAT_COL2_PIO, LEDMAT_COL3_PIO,
                LEDMAT_COL4_PIO, LEDMAT_COL5_PIO
        };

static void display_column(uint8_t row_pattern, uint8_t current_column) {
    if (current_column == 0) {
        pio_output_high(cols[4]);
    } else {
        pio_output_high(cols[current_column - 1]);
    }

    if(!(row_pattern << 1))
        return;

    for (int i = 0; i < 7; i++) {
        (row_pattern >> i) & 1 ? pio_output_low(rows[i]) : pio_output_high(rows[i]);
    }

    pio_output_low(cols[current_column]);

}

static void placeShip(uint8_t* frame, uint8_t* position, uint8_t length){
    //ship is horizontal
    if (!(*position >> 7)) {
        // is the ship out of the left bound?
        if ((length == 3 || length == 4) && *position % 5 == 0)
            *position+= 1;
        else if ((length == 2 || length == 3) && *position % 5 == 4)
            *position-= 1;
        else if (length == 4 && *position % 5 >= 3)
            *position -= 3 - (5 - (*position % 5));
        if (length == 2){
            for (int i = *position % 5; i < length + (*position % 5); i++) {
                frame[i] |= (1 << (*position / 5));
            }
        }else{
            for (int i = (*position % 5) - 1; i < length + (*position % 5) - 1; i++) {
                frame[i] |= (1 << (*position / 5));
            }
        }
    }else /* Ship is vertical */{
        // is the ship out of the top bound
        if ((length == 3 || length == 4) && *position - 128 < 5)
            *position += 5;
        else if ((length == 2 || length == 3) && *position - 128 > 29)
            *position -= 5;
        else if (length == 4 && *position - 128 > 24)
            *position -= 5 * (3 - (7 - ((*position - 128) / 5)));
        if (length == 2){
            for (int i = (*position - 128) / 5; i < ((*position - 128) / 5) + length; i++) {
                frame[(*position - 128) % 5] |= (1 << i);
            }
        }else{
            for (int i = ((*position - 128) / 5) - 1; i < length + ((*position - 128) / 5) - 1; i++) {
                frame[(*position - 128) % 5] |= (1 << i);
            }
        }
    }
}

static void moveShipUp(uint8_t* frame, uint8_t* position){
    // first check if we can move the ship
    for (int i = 0; i < 5; i++){
        if (frame[i] & 1)
            return;
    }

    *position -= 5;
    for (int i = 0; i < 5; i++){
        frame[i] >>= 1;
    }
}

static void moveShipDown(uint8_t* frame, uint8_t* position){
    // first check if we can move the ship
    for (int i = 0; i < 5; i++){
        if (frame[i] & (1 << 6))
            return;
    }

    *position += 5;
    for (int i = 0; i < 5; i++){
        frame[i] <<= 1;
    }
}

static void moveShipLeft(uint8_t* frame, uint8_t* position){
    // first check if we can move the ship
    if (frame[0] != 0)
        return;

    *position -= 1;
    for (int i = 1; i < 5; i++){
        frame[i-1] = frame[i];
    }
    frame[4] = 0;
}

static void moveShipRight(uint8_t* frame, uint8_t* position){
    // first check if we can move the ship
    if (frame[4] != 0)
        return;

    *position += 1;
    for (int i = 4; i > 0; i--){
        frame[i] = frame[i-1];
    }
    frame[0] = 0;
}



int main(void) {
    system_init();
    led_init();
    navswitch_init();
    ledmat_init();
    pacer_init(500);

    led_set(0, 1);
    uint8_t current_column = 0;

    // dim frame
    //uint8_t frame1[5];
    //bright frame
    uint8_t frame2[5] = {0, 0, 0, 0, 0};

    // first bit is for rotation, second is 0, rest is location
    uint8_t shipPosition = 30;


    // first battle ship
    placeShip(frame2, &shipPosition, 3);

    while (1) {
        pacer_wait();
        navswitch_update();
        if (navswitch_push_event_p(0))
            moveShipUp(frame2, &shipPosition);

        if (navswitch_push_event_p(2))
            moveShipDown(frame2, &shipPosition);

        if (navswitch_push_event_p(1))
            moveShipRight(frame2, &shipPosition);

        if (navswitch_push_event_p(3))
            moveShipLeft(frame2, &shipPosition);

        if (navswitch_push_event_p(4)){
            shipPosition ^= 1 << 7;
            for (int i = 0; i < 5; i++){
                frame2[i] = 0;
            }
            placeShip(frame2, &shipPosition, 3);
        }

        current_column++;
        if (current_column > 4)
            current_column = 0;

        display_column(frame2[current_column], current_column);
    }
}
