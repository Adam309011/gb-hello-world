#include <gb/gb.h>
#include <gb/console.h>
#include <stdio.h>

void main(void) {

    gotoxy(2, 2);
    printf("HELLO WORLD");

    while(1) {
        wait_vbl_done();
    }
}