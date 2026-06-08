#include <gb/gb.h>
#include <stdio.h>

void main() {
    UINT8 playerX = 80, playerY = 120;
    UINT8 enemyX = 80, enemyY = 40;
    INT8 enemyDX = 2, enemyDY = 1;
    UINT8 gameRunning = 1;
    UINT16 score = 0;
    UINT8 speedBonus = 0;  // extra speed added over time
    
    DISPLAY_ON;
    SHOW_SPRITES;
    
    UINT8 playerTile[] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    UINT8 enemyTile[]  = { 0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55 };
    set_sprite_data(0, 1, playerTile);
    set_sprite_data(1, 1, enemyTile);
    
    while(1) {
        if (gameRunning) {
            // Move player
            if (joypad() & J_LEFT)  playerX--;
            if (joypad() & J_RIGHT) playerX++;
            if (joypad() & J_UP)    playerY--;
            if (joypad() & J_DOWN)  playerY++;
            
            if (playerX < 8) playerX = 8;
            if (playerX > 152) playerX = 152;
            if (playerY < 16) playerY = 16;
            if (playerY > 136) playerY = 136;
            
            // Increase score (survival time)
            score++;
            
            // Difficulty: every 300 points, enemy gets faster (max +6)
            speedBonus = (score / 300) * 1;
            if (speedBonus > 6) speedBonus = 6;
            
            // Move enemy with current speed
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
            // Game over: press A to reset everything
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
        
        // Show score (using background text)
        printf("\n SCORE: %05u    ", score);
        if (!gameRunning) {
            printf("\n GAME OVER     \n PRESS A       ");
        }
        
        // Draw sprites
        set_sprite_tile(0, 0);
        move_sprite(0, playerX, playerY);
        set_sprite_tile(1, 1);
        move_sprite(1, enemyX, enemyY);
        
        wait_vbl_done();
    }
}