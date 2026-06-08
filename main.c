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
    
    INT16 playerX = 80, playerY = 120;   // signed for calculations
    INT16 enemyX = 80, enemyY = 40;
    INT8 enemyDX = 2, enemyDY = 1;
    UINT8 gameRunning = 1;
    UINT16 score = 0;
    UINT8 speedBonus = 0;
    
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
            
            // Player boundaries (keep inside screen, sprite is 8x8)
            if (playerX < 8) playerX = 8;
            if (playerX > 152) playerX = 152;
            if (playerY < 16) playerY = 16;
            if (playerY > 136) playerY = 136;
            
            score++;
            
            speedBonus = (score / 300);
            if (speedBonus > 6) speedBonus = 6;
            
            // Enemy movement with current speed
            enemyX += enemyDX + (speedBonus / 2);
            enemyY += enemyDY + (speedBonus / 2);
            
            // Boundary bounce with correction to prevent sticking
            if (enemyX < 8) {
                enemyX = 8 + (8 - enemyX);  // push back inside
                enemyDX = -enemyDX;
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
            
            // Collision detection (simple AABB)
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
                enemyDX = 2; enemyDY = 1;
                score = 0;
                speedBonus = 0;
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