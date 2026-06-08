#include <gb/gb.h>
#include <rand.h>

// Player position (8.8 fixed point for smooth movement)
INT16 playerX = 80 << 8;
INT16 playerY = 72 << 8;

// Enemy array (max 8)
#define MAX_ENEMIES 8
INT16 enemyX[MAX_ENEMIES];
INT16 enemyY[MAX_ENEMIES];
UINT8 enemyActive[MAX_ENEMIES];

// Weapon projectiles (simple circle attack)
#define MAX_PROJECTILES 4
INT16 projX[MAX_PROJECTILES];
INT16 projY[MAX_PROJECTILES];
UINT8 projActive[MAX_PROJECTILES];

// Game state
UINT16 score = 0;      // frames survived (≈60 per second)
UINT8 gameRunning = 1;
UINT8 frameCounter = 0;

// Sprite indices
UINT8 playerSprite = 0;
UINT8 enemySprite = 1;
UINT8 projSprite = 2;

// Move player with D-pad (speed: 2 pixels per frame)
void handleInput() {
    if (joypad() & J_LEFT)  playerX -= (3 << 8);
    if (joypad() & J_RIGHT) playerX += (3 << 8);
    if (joypad() & J_UP)    playerY -= (3 << 8);
    if (joypad() & J_DOWN)  playerY += (3 << 8);
    
    // Keep player inside screen (16x16 sprite, screen 160x144)
    if (playerX < (8 << 8)) playerX = (8 << 8);
    if (playerX > (152 << 8)) playerX = (152 << 8);
    if (playerY < (8 << 8)) playerY = (8 << 8);
    if (playerY > (136 << 8)) playerY = (136 << 8);
}

// Spawn a new enemy at random edge
void spawnEnemy() {
    UINT8 i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!enemyActive[i]) {
            enemyActive[i] = 1;
            // Random edge: 0=top,1=right,2=bottom,3=left
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

// Update enemies: move toward player
void updateEnemies() {
    UINT8 i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (enemyActive[i]) {
            INT16 dx = playerX - enemyX[i];
            INT16 dy = playerY - enemyY[i];
            // Move 1 pixel per frame toward player
            if (dx > 0) enemyX[i] += (1 << 8);
            if (dx < 0) enemyX[i] -= (1 << 8);
            if (dy > 0) enemyY[i] += (1 << 8);
            if (dy < 0) enemyY[i] -= (1 << 8);
            
            // Collision with player? Game over
            if (abs((playerX - enemyX[i]) >> 8) < 12 && abs((playerY - enemyY[i]) >> 8) < 12) {
                gameRunning = 0;
            }
        }
    }
}

// Auto-attack: spawn projectiles in a circle every 30 frames
void updateWeapon() {
    if (!gameRunning) return;
    
    frameCounter++;
    if (frameCounter >= 30) { // Attack every half second
        frameCounter = 0;
        UINT8 i;
        for (i = 0; i < MAX_PROJECTILES; i++) {
            if (!projActive[i]) {
                projActive[i] = 1;
                projX[i] = playerX;
                projY[i] = playerY;
                // Direction based on index: 0=right,1=down,2=left,3=up
                // But we'll just move outward in a cross shape
                break;
            }
        }
    }
    
    // Move projectiles and check enemy hit
    UINT8 i, j;
    for (i = 0; i < MAX_PROJECTILES; i++) {
        if (projActive[i]) {
            // Move outward (simple: projectile 0 right, 1 down, 2 left, 3 up)
            switch (i) {
                case 0: projX[i] += (4 << 8); break;
                case 1: projY[i] += (4 << 8); break;
                case 2: projX[i] -= (4 << 8); break;
                case 3: projY[i] -= (4 << 8); break;
            }
            // Check collision with enemies
            for (j = 0; j < MAX_ENEMIES; j++) {
                if (enemyActive[j] && abs((projX[i] - enemyX[j]) >> 8) < 8 && abs((projY[i] - enemyY[j]) >> 8) < 8) {
                    enemyActive[j] = 0;
                    projActive[i] = 0;
                    score += 10; // 10 points per kill
                    break;
                }
            }
            // Deactivate if off screen
            if ((projX[i] >> 8) > 168 || (projX[i] >> 8) < 8 || (projY[i] >> 8) > 152 || (projY[i] >> 8) < 8) {
                projActive[i] = 0;
            }
        }
    }
}

// Draw sprites
void drawSprites() {
    UINT8 i;
    // Player
    set_sprite_tile(0, playerSprite);
    move_sprite(0, (playerX >> 8), (playerY >> 8));
    
    // Enemies
    UINT8 spriteIndex = 1;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (enemyActive[i]) {
            set_sprite_tile(spriteIndex, enemySprite);
            move_sprite(spriteIndex, (enemyX[i] >> 8), (enemyY[i] >> 8));
            spriteIndex++;
        }
    }
    // Hide remaining sprites
    for (; spriteIndex < 40; spriteIndex++) {
        move_sprite(spriteIndex, 0, 0);
    }
    
    // Draw projectiles (we'll use simple squares for now)
    for (i = 0; i < MAX_PROJECTILES; i++) {
        if (projActive[i]) {
            set_sprite_tile(20 + i, projSprite);
            move_sprite(20 + i, (projX[i] >> 8), (projY[i] >> 8));
        } else {
            move_sprite(20 + i, 0, 0);
        }
    }
}

// Draw score on background using printf
void drawUI() {
    // Clear background text area (top line)
    gotoxy(0, 0);
    printf("SCORE:%05u   ", score);
    if (!gameRunning) {
        gotoxy(0, 2);
        printf("GAME OVER  ");
        gotoxy(0, 3);
        printf("PRESS A    ");
    }
}

// Reset the game
void resetGame() {
    playerX = 80 << 8;
    playerY = 72 << 8;
    score = 0;
    gameRunning = 1;
    frameCounter = 0;
    
    UINT8 i;
    for (i = 0; i < MAX_ENEMIES; i++) enemyActive[i] = 0;
    for (i = 0; i < MAX_PROJECTILES; i++) projActive[i] = 0;
    
    // Initial 3 enemies
    for (i = 0; i < 3; i++) spawnEnemy();
}

void main() {
    DISPLAY_ON;
    SHOW_SPRITES;
    SHOW_BKG;
    
    // Simple 8x8 sprite tiles (we'll use built-in shapes)
    // For simplicity, use default tiles: player=0x0F (solid square), enemy=0xF0, projectile=0xAA
    // But we need to set them. Using GBDK's built-in: we can set tiles manually.
    // Actually GBDK starts with blank tiles. Let's create simple patterns.
    UINT8 playerTile[] = { 0x3C, 0x7E, 0xDB, 0xFF, 0xFF, 0xDB, 0x7E, 0x3C };
    UINT8 enemyTile[]  = { 0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x3C, 0x18 };
    UINT8 projTile[]   = { 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 };
    
    set_sprite_data(playerSprite, 1, playerTile);
    set_sprite_data(enemySprite, 1, enemyTile);
    set_sprite_data(projSprite, 1, projTile);
    
    resetGame();
    
    while(1) {
        if (gameRunning) {
            handleInput();
            updateEnemies();
            updateWeapon();
            // Spawn enemies occasionally
            if ((UINT8)rand() < 8 && frameCounter % 10 == 0) spawnEnemy();
            // Increase score by time survived
            score++;
        } else {
            // Game over: press A to reset
            if (joypad() & J_A) {
                resetGame();
            }
        }
        
        drawSprites();
        drawUI();
        
        wait_vbl_done();
    }
}