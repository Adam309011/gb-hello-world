#include <gb/gb.h>
#include <stdio.h>

void main() {
    UINT8 playerX = 80, playerY = 120;
    UINT8 enemyX = 80, enemyY = 40;
    INT8 enemyDX = 2, enemyDY = 1;
    UINT8 gameRunning = 1;
    UINT16 score = 0;
    UINT8 speedBonus = 0;
    
    DISPLAY_ON;
    SHOW_SPRITES;
    
    UINT8 playerTile[] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    UINT8 enemyTile[]  = { 0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55 };
    set_sprite_data(0, 1, playerTile);
    set_sprite_data(1, 1, enemyTile);
    
    while(1) {
        if (gameRunning) {
            // Movement (same as before)
            if (joypad() & J_LEFT)  playerX--;
            if (joypad() & J_RIGHT) playerX++;
            if (joypad() & J_UP)    playerY--;
            if (joypad() & J_DOWN)  playerY++;
            
            if (playerX < 8) playerX = 8;
            if (playerX > 152) playerX = 152;
            if (playerY < 16) playerY = 16;
            if (playerY > 136) playerY = 136;
            
            // Score increases
            score++;
            
            // Difficulty scaling
            speedBonus = (score / 300) * 1;
            if (speedBonus > 6) speedBonus = 6;
            
            // Enemy movement
            enemyX += enemyDX + speedBonus/2;
            enemyY += enemyDY + speedBonus/2;
            if (enemyX < 8 || enemyX > 152) enemyDX = -enemyDX;
            if (enemyY < 16 || enemyY > 136) enemyDY = -enemyDY;
            
            // Collision
            if (playerX < enemyX + 8 && playerX + 8 > enemyX &&
                playerY < enemyY + 8 && playerY + 8 > enemyY) {
                gameRunning = 0;
            }
        } else {
            if (joypad() & J_A) {
                playerX = 80;
                playerY = 120;
                enemyX = 80;
                enemyY = 40;
                enemyDX = 2;
                enemyDY = 1;
                score = 0;
                speedBonus = 0;
                gameRunning = 1;
            }
        }
        
        // ===== FIXED DISPLAY =====
        // Position cursor at top-left (row 0, column 0) and print score
        gotoxy(0, 0);
        printf("SCORE: %05u     ", score);  // Spaces after to clear old digits
        
        if (!gameRunning) {
            gotoxy(0, 1);
            printf("GAME OVER      ");
            gotoxy(0, 2);
            printf("PRESS A        ");
        } else {
            // Clear the GAME OVER lines when restarting
            gotoxy(0, 1);
            printf("               ");
            gotoxy(0, 2);
            printf("               ");
        }
        
        // Draw sprites
        set_sprite_tile(0, 0);
        move_sprite(0, playerX, playerY);
        set_sprite_tile(1, 1);
        move_sprite(1, enemyX, enemyY);
        
        wait_vbl_done();
    }
}