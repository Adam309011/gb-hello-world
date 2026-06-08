#include <gb/gb.h>

void main() {
    UINT8 x = 80, y = 72;
    DISPLAY_ON;
    SHOW_SPRITES;
    
    // A simple 8x8 solid square sprite
    UINT8 tile[] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    set_sprite_data(0, 1, tile);
    set_sprite_tile(0, 0);
    
    while(1) {
        if (joypad() & J_LEFT)  x--;
        if (joypad() & J_RIGHT) x++;
        if (joypad() & J_UP)    y--;
        if (joypad() & J_DOWN)  y++;
        
        move_sprite(0, x, y);
        wait_vbl_done();
    }
}