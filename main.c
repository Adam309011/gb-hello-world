#include <gb/gb.h>
#include <stdio.h>

void main(void) {
    DISPLAY_ON;                 // Turn screen on
    printf("HELLO WORLD");      // Print using default font
    
    while(1) {
        wait_vbl_done();        // Keep the game running
    }
}