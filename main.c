#include <gb/gb.h>

void main() {
    UINT8 playerX = 80, playerY = 120;
    UINT8 enemyX = 80, enemyY = 40;
    INT8 enemyDX = 2, enemyDY = 1;
    
    DISPLAY_ON;
    SHOW_SPRITES;
    
    // Two different-looking squares (player white, enemy striped)
    UINT8 playerTile[] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    UINT8 enemyTile[]  = { 0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55 };
    set_sprite_data(0, 1, playerTile);
    set_sprite_data(1, 1, enemyTile);
    
    while(1) {
        // Move player
        if (joypad() & J_LEFT)  playerX--;
        if (joypad() & J_RIGHT) playerX++;
        if (joypad() & J_UP)    playerY--;
        if (joypad() & J_DOWN)  playerY++;
        
        // Keep player on screen
        if (playerX < 8) playerX = 8;
        if (playerX > 152) playerX = 152;
        if (playerY < 16) playerY = 16;
        if (playerY > 136) playerY = 136;
        
        // Move enemy
        enemyX += enemyDX;
        enemyY += enemyDY;
        if (enemyX < 8 || enemyX > 152) enemyDX = -enemyDX;
        if (enemyY < 16 || enemyY > 136) enemyDY = -enemyDY;
        
        // Draw
        set_sprite_tile(0, 0);
        move_sprite(0, playerX, playerY);
        set_sprite_tile(1, 1);
        move_sprite(1, enemyX, enemyY);
        
        wait_vbl_done();
    }
}