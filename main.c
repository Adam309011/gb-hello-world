#include <gb/gb.h>
#include <stdio.h>
#include <string.h>

void printScore(UINT8 x, UINT8 y, UINT16 score) {
    char buffer[10];
    sprintf(buffer, "%05u", score);
    UINT8 i;
    for (i = 0; i < 5; i++) {
        set_bkg_tile_xy(x + i, y, buffer[i] - 32);
    }
}

void printText(UINT8 x, UINT8 y, char* text) {
    UINT8 i;
    for (i = 0; i < strlen(text); i++) {
        set_bkg_tile_xy(x + i, y, text[i] - 32);
    }
}

void main() {
    printf(" ");  // load font
    
    INT16 playerX = 80, playerY = 120;
    INT16 enemyX = 80, enemyY = 40;
    INT8 enemyDX = 2, enemyDY = 1;   // direction and base speed
    UINT8 gameRunning = 1;
    UINT16 score = 0;
    UINT8 speedLevel = 0;   // 0 = base speed, 1 = faster, etc.
    
    DISPLAY_ON;
    SHOW_SPRITES;
    SHOW_BKG;
    
    // Clear background
    for (UINT8 i = 0; i < 32; i++) {
        for (UINT8 j = 0; j < 18; j++) {
            set_bkg_tile_xy(i, j, 0);
        }
    }
    printText(0, 0, "SCORE:");
    
    UINT8 playerTile[] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    UINT8 enemyTile[]  = { 0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55 };
    set_sprite_data(0, 1, playerTile);
    set_sprite_data(1, 1, enemyTile);
    
    while(1) {
        if (gameRunning) {
            // Player movement
            if (joypad() & J_LEFT)  playerX--;
            if (joypad() & J_RIGHT) playerX++;
            if (joypad() & J_UP)    playerY--;
            if (joypad() & J_DOWN)  playerY++;
            
            if (playerX < 8) playerX = 8;
            if (playerX > 152) playerX = 152;
            if (playerY < 16) playerY = 16;
            if (playerY > 136) playerY = 136;
            
            score++;
            
            // Increase difficulty every 300 points (increase speed magnitude)
            // speedLevel = 0 (score<300), 1 (300-599), 2 (600-899), etc.
            speedLevel = score / 300;
            if (speedLevel > 6) speedLevel = 6;  // cap at +6 extra speed
            
            // Calculate current speed based on base direction + speedLevel
            // We keep enemyDX as the sign (+1 or -1) and multiply by speed
            // But for simplicity, store signed magnitude directly
            // Actually we need to update enemyDX/enemyDY magnitudes
            // Let's recompute from base direction signs:
            INT8 baseDX = (enemyDX > 0) ? 1 : -1;
            INT8 baseDY = (enemyDY > 0) ? 1 : -1;
            // Speed values: start at 2 for X, 1 for Y, increase by speedLevel/2? Let's make it clean:
            // X speed = 2 + speedLevel, Y speed = 1 + speedLevel/2
            INT8 currentSpeedX = 2 + speedLevel;
            INT8 currentSpeedY = 1 + (speedLevel / 2);
            enemyDX = baseDX * currentSpeedX;
            enemyDY = baseDY * currentSpeedY;
            
            // Move enemy
            enemyX += enemyDX;
            enemyY += enemyDY;
            
            // Boundary bounce with position correction
            if (enemyX < 8) {
                enemyX = 8 + (8 - enemyX);
                enemyDX = -enemyDX;   // reverse direction (sign flips, magnitude stays)
            }
            if (enemyX > 152) {
                enemyX = 152 - (enemyX - 152);
                enemyDX = -enemyDX;
            }
            if (enemyY < 16) {
                enemyY = 16 + (16 - enemyY);
                enemyDY = -enemyDY;
            }
            if (enemyY > 136) {
                enemyY = 136 - (enemyY - 136);
                enemyDY = -enemyDY;
            }
            
            // Collision
            if (playerX < enemyX + 8 && playerX + 8 > enemyX &&
                playerY < enemyY + 8 && playerY + 8 > enemyY) {
                gameRunning = 0;
            }
            
            printScore(7, 0, score);
        } else {
            printText(0, 1, "GAME OVER");
            printText(0, 2, "PRESS A");
            if (joypad() & J_A) {
                playerX = 80; playerY = 120;
                enemyX = 80; enemyY = 40;
                enemyDX = 2; enemyDY = 1;  // reset to base speed
                score = 0;
                speedLevel = 0;
                gameRunning = 1;
                printText(0, 1, "        ");
                printText(0, 2, "       ");
                printText(0, 0, "SCORE:");
            }
        }
        
        set_sprite_tile(0, 0);
        move_sprite(0, playerX, playerY);
        set_sprite_tile(1, 1);
        move_sprite(1, enemyX, enemyY);
        
        wait_vbl_done();
    }
}