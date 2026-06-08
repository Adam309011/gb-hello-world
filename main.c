#include <gb/gb.h>
#include <stdio.h>

void main() {
    gotoxy(2, 2);
    printf("HELLO WORLD");

    while(1) {
        wait_vbl_done();
    }
}