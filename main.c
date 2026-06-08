#include <gb/gb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// -------- TILE DATA (16 bytes per tile) --------
// Player: solid filled square (colour 1)
const unsigned char playerTile[] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,  // low plane
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00   // high plane
};

// Enemy: X shape (colour 1)
const unsigned char enemyTile[] = {
    0x81,0x42,0x24,0x18,0x18,0x24,0x42,0x81,  // low plane
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00   // high plane
};

#define MAX_ENEMIES 4

typedef struct {
    uint8_t x, y;
    uint8_t active;
    uint8_t speed;
} Enemy;

Enemy enemies[MAX_ENEMIES];
uint8_t playerX;
const uint8_t playerY = 136;          // 144 - 8 (bottom area)
uint16_t score = 0;
uint8_t gameRunning = 1;
uint8_t gameOverFlag = 0;

// -------------------- GAME LOGIC --------------------
void initGame() {
    playerX = (160 - 8) / 2;          // centered
    score = 0;
    gameRunning = 1;
    gameOverFlag = 0;
    for (uint8_t i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].active = 0;
    }
    // Clear screen messages
    gotoxy(0,0); printf("Score: 000");
    gotoxy(0,2); printf("            ");
    gotoxy(0,3); printf("            ");
}

void spawnEnemy() {
    for (uint8_t i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            enemies[i].x = rand() % (160 - 8);   // 0..152
            enemies[i].y = 0;
            enemies[i].speed = 1 + (rand() % 3); // 1,2 or 3
            enemies[i].active = 1;
            break;
        }
    }
}

void updateEnemies() {
    for (uint8_t i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;

        enemies[i].y += enemies[i].speed;

        if (enemies[i].y >= 144) {               // fell off bottom
            enemies[i].active = 0;
            score++;                             // reward for dodging
        } else {
            // Collision detection (8x8 bounding boxes)
            if (gameRunning &&
                enemies[i].x < playerX + 8 &&
                enemies[i].x + 8 > playerX &&
                enemies[i].y < playerY + 8 &&
                enemies[i].y + 8 > playerY) {
                gameRunning = 0;
                gameOverFlag = 1;
            }
        }
    }
}

void updatePlayerInput() {
    if (!gameRunning) return;
    uint8_t joy = joypad();
    if (joy & J_LEFT) {
        if (playerX >= 2) playerX -= 2;
    }
    if (joy & J_RIGHT) {
        if (playerX <= 160 - 8 - 2) playerX += 2;
    }
}

// -------------------- DISPLAY --------------------
void drawSprites() {
    // Player sprite (sprite 0)
    set_sprite_tile(0, 0);
    move_sprite(0, playerX, playerY);

    // Enemy sprites (1 .. MAX_ENEMIES)
    uint8_t spriteIdx = 1;
    for (uint8_t i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            set_sprite_tile(spriteIdx, 1);
            move_sprite(spriteIdx, enemies[i].x, enemies[i].y);
            spriteIdx++;
        } else {
            // hide unused sprites
            set_sprite_tile(spriteIdx, 0);
            move_sprite(spriteIdx, 0, 0);
        }
    }
    // hide any remaining sprites
    for (uint8_t i = spriteIdx; i <= MAX_ENEMIES; i++) {
        set_sprite_tile(i, 0);
        move_sprite(i, 0, 0);
    }
}

void updateScoreDisplay() {
    gotoxy(0,0);
    printf("Score: %03u", score);
    if (gameOverFlag) {
        gotoxy(0,2);
        printf("GAME OVER   ");
        gotoxy(0,3);
        printf("Press Start");
    } else {
        gotoxy(0,2);
        printf("            ");
        gotoxy(0,3);
        printf("            ");
    }
}

// -------------------- RESTART --------------------
void handleRestart() {
    if (gameOverFlag && (joypad() & J_START)) {
        initGame();
    }
}

// -------------------- MAIN --------------------
void main() {
    DISPLAY_ON;
    SHOW_SPRITES;
    SHOW_BKG;

    // Load tile graphics
    set_sprite_data(0, 1, playerTile);
    set_sprite_data(1, 1, enemyTile);

    // Random seed from hardware timer
    srand(DIV_REG);

    initGame();

    uint8_t frameCounter = 0;

    while (1) {
        if (!gameRunning && gameOverFlag) {
            handleRestart();
            wait_vbl_done();
            continue;
        }

        updatePlayerInput();

        if (gameRunning) {
            // spawn new enemy ~ every 30 frames
            if ((rand() % 30) == 0) spawnEnemy();
            updateEnemies();

            // survival score (every 8 frames)
            frameCounter++;
            if (frameCounter >= 8) {
                frameCounter = 0;
                score++;
            }
        }

        updateScoreDisplay();
        drawSprites();
        wait_vbl_done();
    }
}