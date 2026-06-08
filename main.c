#include <gb/gb.h>
#include <stdio.h>

INT16 playerX = 80 << 8;
INT16 playerY = 120 << 8;  // near bottom
INT16 enemyX = 80 << 8;
INT16 enemyY = 40 << 8;
INT8 enemyDX = 2;
INT8 enemyDY = 1;
UINT8 score = 0;
UINT8 gameRunning = 1;

void main() {
    DISPLAY_ON;
    SHOW_SPRITES;
    
    // Simple square sprites
    UINT8 playerTile[] = { 0x3C,0x7E,0xDB,0xFF,0xFF,0xDB,0x7E,0x3C };
    UINT8 enemyTile[]  = { 0x18,0x3C,0x7E,0xFF,0xFF,0x7E,0x3C,0x18 };
    set_sprite_data(0, 1, playerTile);
    set_sprite_data(1, 1, enemyTile);
    
    while(1) {
        if (gameRunning) {
            // Move player with D-pad
            if (joypad() & J_LEFT) playerX -= (3 << 8);
            if (joypad() & J_RIGHT) playerX += (3 << 8);
            if (playerX < (8 << 8)) playerX = (8 << 8);
            if (playerX > (152 << 8)) playerX = (152 << 8);
            
            // Move enemy
            enemyX += (enemyDX << 8);
            enemyY += (enemyDY << 8);
            if ((enemyX >> 8) < 8 || (enemyX >> 8) > 152) enemyDX = -enemyDX;
            if ((enemyY >> 8) < 8 || (enemyY >> 8) > 136) enemyDY = -enemyDY;
            
            // Collision
            if (((playerX - enemyX) >> 8) < 12 && ((playerX - enemyX) >> 8) > -12 &&
                ((playerY - enemyY) >> 8) < 12 && ((playerY - enemyY) >> 8) > -12) {
                gameRunning = 0;
            }
            
            // Score increases over time
            score++;
            
            // Draw
            set_sprite_tile(0, 0);
            move_sprite(0, playerX >> 8, playerY >> 8);
            set_sprite_tile(1, 1);
            move_sprite(1, enemyX >> 8, enemyY >> 8);
            
            // Print UI
            printf("\n SCORE: %05u   ", score);
        } else {
            printf("\n GAME OVER     \n PRESS A TO TRY");
            if (joypad() & J_A) {
                // Reset everything
                playerX = 80 << 8;
                enemyX = 80 << 8;
                enemyY = 40 << 8;
                score = 0;
                gameRunning = 1;
            }
        }
        wait_vbl_done();
    }
}