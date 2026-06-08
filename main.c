#include <gb/gb.h>
#include <stdio.h>
#include <gb/font.h>

void main(void) {

    font_init();
    font_set(font_load(font_ibm));

    printf("HELLO WORLD");

    while(1) {
        wait_vbl_done();
    }
}