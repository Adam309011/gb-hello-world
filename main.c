#include <gb/gb.h>
#include <stdio.h>
#include <stdlib.h>
#include <rand.h>

// Player position (fixed point 8.8)
INT16 playerX = 80 << 8;
INT16 playerY = 72 << 8;

#define MAX_ENEMIES 8
INT16 enemyX[MAX_ENEMIES];
INT16 enemyY[MAX_ENEMIES];
UINT8 enemyActive[MAX_ENEMIES];

#define MAX_PROJECTILES 4
INT16 projX[MAX_PROJECTILES];
INT16 projY[MAX_PROJECTILES];
UINT8 projActive[MAX_PROJECTILES];

UINT16 score = 0;
UINT8 gameRunning = 1;
UINT8 frameCounter = 0;
UINT8 attackCooldown = 0;

UINT8 playerSprite = 0;
UINT8 enemySprite = 1;
UINT8 projSprite = 2;

// Simple absolute value (avoid stdlib abs issues)
INT8 my_abs(INT16 a) {
    return (a < 0) ? -a : a;
}

void handleInput() {
    if (joypad() & J_LEFT)  playerX -= (3 << 8);
    if (joypad() & J_RIGHT) playerX += (3 << 8);
    if (joypad() & J_UP)    playerY -= (3 << 8);
    if (joypad() & J_DOWN)  playerY += (3 << 8);
    
    if (playerX < (8 << 8)) playerX = (8 << 8);
    if (playerX > (152 << 8)) playerX = (152 << 8);
    if (playerY < (8 << 8)) playerY = (8 << 8);
    if (playerY > (136 << 8)) playerY = (136 << 8);
}

void spawnEnemy() {
    UINT8 i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!enemyActive[i]) {
            enemyActive[i] = 1;
            switch (rand() % 4) {
                case 0: enemyX[i] = (rand() % 160) << 8; enemyY[i] = 0; break;
                case 1: enemyX[i] = 160 << 8; enemyY[i] = (rand() % 144) << 8; break;
                case 2: enemyX[i] = (rand() % 160) << 8; enemyY[i] = 144 << 8; break;
                case 3: enemyX[i] = 0; enemyY[i] = (rand() % 144) << 8; break;
            }
            break;
        }
    }
}

void updateEnemies() {
    UINT8 i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (enemyActive[i]) {
            INT16 dx = playerX - enemyX[i];
            INT16 dy = playerY - enemyY[i];
            if (dx > 0) enemyX[i] += (1 << 8);
            if (dx < 0) enemyX[i] -= (1 << 8);
            if (dy > 0) enemyY[i] += (1 << 8);
            if (dy < 0) enemyY[i] -= (1 << 8);
            
            // Collision
            if (my_abs((playerX - enemyX[i]) >> 8) < 12 && 
                my_abs((playerY - enemyY[i]) >> 8) < 12) {
                gameRunning = 0;
            }
        }
    }
}

void updateWeapon() {
    if (!gameRunning) return;
    
    if (attackCooldown == 0) {
        attackCooldown = 30;
        UINT8 i;
        for (i = 0; i < MAX_PROJECTILES; i++) {
            if (!projActive[i]) {
                projActive[i] = 1;
                projX[i] = playerX;
                projY[i] = playerY;
                break;
            }
        }
    } else {
        attackCooldown--;
    }
    
    UINT8 i, j;
    for (i = 0; i < MAX_PROJECTILES; i++) {
        if (projActive[i]) {
            // Move outward
            switch (i) {
                case 0: projX[i] += (4 << 8); break;
                case 1: projY[i] += (4 << 8); break;
                case 2: projX[i] -= (4 << 8); break;
                case 3: projY[i] -= (4 << 8); break;
            }
            for (j = 0; j < MAX_ENEMIES; j++) {
                if (enemyActive[j] && 
                    my_abs((projX[i] - enemyX[j]) >> 8) < 8 && 
                    my_abs((projY[i] - enemyY[j]) >> 8) < 8) {
                    enemyActive[j] = 0;
                    projActive[i] = 0;
                    score += 10;
                    break;
                }
            }
            if ((projX[i] >> 8) > 168 || (projX[i] >> 8) < 8 ||
                (projY[i] >> 8) > 152 || (projY[i] >> 8) < 8) {
                projActive[i] = 0;
            }
        }
    }
}

void drawSprites() {
    UINT8 i;
    set_sprite_tile(0, playerSprite);
    move_sprite(0, playerX >> 8, playerY >> 8);
    
    UINT8 spriteIdx = 1;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (enemyActive[i]) {
            set_sprite_tile(spriteIdx, enemySprite);
            move_sprite(spriteIdx, enemyX[i] >> 8, enemyY[i] >> 8);
            spriteIdx++;
        }
    }
    for (; spriteIdx < 40; spriteIdx++) move_sprite(spriteIdx, 0, 0);
    
    for (i = 0; i < MAX_PROJECTILES; i++) {
        if (projActive[i]) {
            set_sprite_tile(20 + i, projSprite);
            move_sprite(20 + i, projX[i] >> 8, projY[i] >> 8);
        } else {
            move_sprite(20 + i, 0, 0);
        }
    }
}

void drawUI() {
    // Use background text (no gotoxy needed, just printf with position)
    // But printf only prints at current cursor – we can use it with newlines
    // Simpler: use positioned background tiles manually? Not needed.
    // We'll just print at top-left corner.
    printf("\n SCORE: %05u    ", score);
    if (!gameRunning) {
        printf("\n GAME OVER     \n PRESS A       ");
    }
    // Clear rest of screen? Not required.
}

void resetGame() {
    playerX = 80 << 8;
    playerY = 72 << 8;
    score = 0;
    gameRunning = 1;
    frameCounter = 0;
    attackCooldown = 0;
    UINT8 i;
    for (i = 0; i < MAX_ENEMIES; i++) enemyActive[i] = 0;
    for (i = 0; i < MAX_PROJECTILES; i++) projActive[i] = 0;
    for (i = 0; i < 3; i++) spawnEnemy();
}

void main() {
    DISPLAY_ON;
    SHOW_SPRITES;
    SHOW_BKG;
    
    // Simple 8x8 tile data
    UINT8 playerTile[] = { 0x3C,0x7E,0xDB,0xFF,0xFF,0xDB,0x7E,0x3C };
    UINT8 enemyTile[]  = { 0x18,0x3C,0x7E,0xFF,0xFF,0x7E,0x3C,0x18 };
    UINT8 projTile[]   = { 0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x00 };
    
    set_sprite_data(playerSprite, 1, playerTile);
    set_sprite_data(enemySprite, 1, enemyTile);
    set_sprite_data(projSprite, 1, projTile);
    
    resetGame();
    
    while(1) {
        if (gameRunning) {
            handleInput();
            updateEnemies();
            updateWeapon();
            if ((UINT8)rand() < 8 && attackCooldown % 10 == 0) spawnEnemy();
            score++;
        } else {
            if (joypad() & J_A) resetGame();
        }
        
        drawSprites();
        drawUI();
        wait_vbl_done();
    }
}