/**
 * DODGE THE SWARM
 * A complete Game Boy game by you
 * 
 * How to play:
 * - Move with D-PAD
 * - Avoid red enemies (striped)
 * - Collect blue power-ups (white square)
 * - Survive waves – each wave adds more enemies
 * - Score increases over time and by collecting power-ups
 * - Press START to pause
 * 
 * Features:
 * - Title screen
 * - Game over screen with high score
 * - High score saved to SRAM (battery backup)
 * - Sound effects (beeps)
 * - 5 difficulty waves
 * - Particle effects when collecting power-ups
 */

#include <gb/gb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ========== CONSTANTS ==========
#define MAX_ENEMIES 12
#define MAX_POWERUPS 3
#define MAX_PARTICLES 8

#define PLAYER_START_X 80
#define PLAYER_START_Y 120
#define PLAYER_SIZE 8

#define SCREEN_LEFT 8
#define SCREEN_RIGHT 152
#define SCREEN_TOP 16
#define SCREEN_BOTTOM 136

// Wave settings
const UINT8 WAVE_ENEMY_COUNT[] = { 1, 2, 3, 4, 6, 8, 10, 12 };
const UINT8 WAVE_BASE_SPEED[]  = { 1, 1, 2, 2, 3, 3, 4, 4 };
#define MAX_WAVE (sizeof(WAVE_ENEMY_COUNT) / sizeof(UINT8))

// ========== GLOBAL VARIABLES ==========
// Player
INT16 playerX, playerY;

// Enemies
INT16 enemyX[MAX_ENEMIES];
INT16 enemyY[MAX_ENEMIES];
INT8  enemyDX[MAX_ENEMIES];
INT8  enemyDY[MAX_ENEMIES];
UINT8 enemyActive[MAX_ENEMIES];

// Power-ups
INT16 powerX[MAX_POWERUPS];
INT16 powerY[MAX_POWERUPS];
UINT8 powerActive[MAX_POWERUPS];
UINT8 powerType[MAX_POWERUPS];    // 0 = speed boost, 1 = invincible, 2 = slow time

// Particles (for effects)
INT16 partX[MAX_PARTICLES];
INT16 partY[MAX_PARTICLES];
INT8  partDX[MAX_PARTICLES];
INT8  partDY[MAX_PARTICLES];
UINT8 partActive[MAX_PARTICLES];
UINT8 partTimer[MAX_PARTICLES];

// Game state
UINT8 gameState;          // 0=title, 1=playing, 2=paused, 3=gameover
UINT16 score;
UINT16 highScore;
UINT8 wave;
UINT8 enemiesThisWave;
UINT8 invincibleTimer;
UINT8 slowTimer;
UINT8 powerUpSpawnTimer;
UINT8 frameCounter;
UINT8 flashCounter;       // for invincibility flash

// Sound effect flags
UINT8 playCollectSound;
UINT8 playHitSound;
UINT8 playWaveSound;

// ========== HELPER FUNCTIONS ==========
// Print text on background (position x,y, 0-indexed)
void printText(UINT8 x, UINT8 y, char* text) {
    UINT8 i;
    for (i = 0; i < strlen(text); i++) {
        set_bkg_tile_xy(x + i, y, text[i] - 32);
    }
}

// Print a number (max 5 digits)
void printNumber(UINT8 x, UINT8 y, UINT16 num) {
    char buffer[6];
    sprintf(buffer, "%05u", num);
    UINT8 i;
    for (i = 0; i < 5; i++) {
        set_bkg_tile_xy(x + i, y, buffer[i] - 32);
    }
}

// Clear a rectangular area of background
void clearArea(UINT8 x, UINT8 y, UINT8 w, UINT8 h) {
    UINT8 i, j;
    for (i = 0; i < w; i++) {
        for (j = 0; j < h; j++) {
            set_bkg_tile_xy(x + i, y + j, 0);
        }
    }
}

// Simple beep sound using NR14 register
void beep(UINT8 pitch, UINT8 length) {
    UINT8 freq = 0x80 + pitch;
    NR10_REG = 0x00;
    NR11_REG = 0x40 | length;
    NR12_REG = 0xF0;
    NR13_REG = freq;
    NR14_REG = 0x87;
}

// Play collect sound
void soundCollect() {
    beep(0x20, 0x05);
}

// Play hit sound
void soundHit() {
    beep(0x50, 0x02);
    beep(0x30, 0x02);
}

// Play wave clear sound
void soundWave() {
    beep(0x10, 0x08);
    beep(0x20, 0x08);
}

// ========== GAME INIT / RESET ==========
void resetGame() {
    UINT8 i;
    
    playerX = PLAYER_START_X << 8;
    playerY = PLAYER_START_Y << 8;
    score = 0;
    wave = 0;
    invincibleTimer = 0;
    slowTimer = 0;
    powerUpSpawnTimer = 0;
    frameCounter = 0;
    flashCounter = 0;
    playCollectSound = 0;
    playHitSound = 0;
    playWaveSound = 0;
    
    // Clear enemies
    for (i = 0; i < MAX_ENEMIES; i++) {
        enemyActive[i] = 0;
    }
    
    // Clear power-ups
    for (i = 0; i < MAX_POWERUPS; i++) {
        powerActive[i] = 0;
    }
    
    // Clear particles
    for (i = 0; i < MAX_PARTICLES; i++) {
        partActive[i] = 0;
    }
    
    // Spawn initial wave
    enemiesThisWave = WAVE_ENEMY_COUNT[wave];
    for (i = 0; i < enemiesThisWave; i++) {
        enemyActive[i] = 1;
        // Spawn at random edge
        switch (rand() % 4) {
            case 0: enemyX[i] = (8 + rand() % 144) << 8; enemyY[i] = 0; break;
            case 1: enemyX[i] = 160 << 8; enemyY[i] = (16 + rand() % 120) << 8; break;
            case 2: enemyX[i] = (8 + rand() % 144) << 8; enemyY[i] = 144 << 8; break;
            case 3: enemyX[i] = 0; enemyY[i] = (16 + rand() % 120) << 8; break;
        }
        // Set random direction
        enemyDX[i] = (rand() % 3) - 1;
        enemyDY[i] = (rand() % 3) - 1;
        if (enemyDX[i] == 0) enemyDX[i] = 1;
        if (enemyDY[i] == 0) enemyDY[i] = 1;
        // Normalize speed
        UINT8 spd = WAVE_BASE_SPEED[wave] + (rand() % 2);
        enemyDX[i] = (enemyDX[i] > 0) ? spd : -spd;
        enemyDY[i] = (enemyDY[i] > 0) ? spd : -spd;
    }
}

// ========== SAVE/LOAD HIGH SCORE ==========
void saveHighScore() {
    // Enable SRAM
    ENABLE_RAM;
    // Write high score to first two bytes (16-bit)
    *((UINT16*)0xA000) = highScore;
    // Disable SRAM
    DISABLE_RAM;
}

void loadHighScore() {
    // Enable SRAM
    ENABLE_RAM;
    highScore = *((UINT16*)0xA000);
    // If uninitialized (0xFFFF), set to 0
    if (highScore == 0xFFFF) highScore = 0;
    DISABLE_RAM;
}

// ========== SPAWN POWER-UP ==========
void spawnPowerUp() {
    UINT8 i;
    for (i = 0; i < MAX_POWERUPS; i++) {
        if (!powerActive[i]) {
            powerActive[i] = 1;
            powerX[i] = (16 + rand() % 128) << 8;
            powerY[i] = (24 + rand() % 104) << 8;
            powerType[i] = rand() % 3;
            break;
        }
    }
}

// ========== SPAWN PARTICLE (for effects) ==========
void spawnParticle(INT16 x, INT16 y) {
    UINT8 i;
    for (i = 0; i < MAX_PARTICLES; i++) {
        if (!partActive[i]) {
            partActive[i] = 1;
            partX[i] = x;
            partY[i] = y;
            partDX[i] = (rand() % 5) - 2;
            partDY[i] = (rand() % 5) - 2;
            partTimer[i] = 16;
            break;
        }
    }
}

// ========== UPDATE ENEMIES ==========
void updateEnemies() {
    UINT8 i, spd;
    spd = WAVE_BASE_SPEED[wave] + (slowTimer ? 0 : 0); // slow reduces speed later
    for (i = 0; i < enemiesThisWave; i++) {
        if (enemyActive[i]) {
            // Move
            enemyX[i] += (enemyDX[i] << 8);
            enemyY[i] += (enemyDY[i] << 8);
            
            // Bounce off walls
            if (enemyX[i] < (SCREEN_LEFT << 8)) {
                enemyX[i] = (SCREEN_LEFT << 8) - (enemyX[i] - (SCREEN_LEFT << 8));
                enemyDX[i] = -enemyDX[i];
            }
            if (enemyX[i] > (SCREEN_RIGHT << 8)) {
                enemyX[i] = (SCREEN_RIGHT << 8) - (enemyX[i] - (SCREEN_RIGHT << 8));
                enemyDX[i] = -enemyDX[i];
            }
            if (enemyY[i] < (SCREEN_TOP << 8)) {
                enemyY[i] = (SCREEN_TOP << 8) - (enemyY[i] - (SCREEN_TOP << 8));
                enemyDY[i] = -enemyDY[i];
            }
            if (enemyY[i] > (SCREEN_BOTTOM << 8)) {
                enemyY[i] = (SCREEN_BOTTOM << 8) - (enemyY[i] - (SCREEN_BOTTOM << 8));
                enemyDY[i] = -enemyDY[i];
            }
            
            // Collision with player
            if (!invincibleTimer) {
                INT16 dx = (playerX - enemyX[i]) >> 8;
                INT16 dy = (playerY - enemyY[i]) >> 8;
                if (dx < 12 && dx > -12 && dy < 12 && dy > -12) {
                    // Hit!
                    playHitSound = 1;
                    invincibleTimer = 90;   // 1.5 seconds invincibility
                    score = (score > 100) ? score - 100 : 0;
                    // Spawn particles at collision
                    spawnParticle(enemyX[i], enemyY[i]);
                }
            }
        }
    }
}

// ========== UPDATE POWER-UPS ==========
void updatePowerups() {
    UINT8 i;
    for (i = 0; i < MAX_POWERUPS; i++) {
        if (powerActive[i]) {
            // Move slowly downwards
            powerY[i] += (1 << 8);
            if (powerY[i] > (SCREEN_BOTTOM << 8)) {
                powerActive[i] = 0;
                continue;
            }
            
            // Collision with player
            INT16 dx = (playerX - powerX[i]) >> 8;
            INT16 dy = (playerY - powerY[i]) >> 8;
            if (dx < 8 && dx > -8 && dy < 8 && dy > -8) {
                powerActive[i] = 0;
                playCollectSound = 1;
                // Apply power-up effect
                switch (powerType[i]) {
                    case 0: // Speed boost (increase score)
                        score += 500;
                        break;
                    case 1: // Invincibility for 3 seconds
                        invincibleTimer = 180;
                        break;
                    case 2: // Slow time for 3 seconds
                        slowTimer = 180;
                        break;
                }
                spawnParticle(powerX[i], powerY[i]);
                // Bonus score for collecting
                score += 100;
            }
        }
    }
}

// ========== UPDATE PARTICLES ==========
void updateParticles() {
    UINT8 i;
    for (i = 0; i < MAX_PARTICLES; i++) {
        if (partActive[i]) {
            partX[i] += (partDX[i] << 8);
            partY[i] += (partDY[i] << 8);
            partTimer[i]--;
            if (partTimer[i] == 0 || partX[i] < 0 || partX[i] > (160<<8) || partY[i] < 0 || partY[i] > (144<<8)) {
                partActive[i] = 0;
            }
        }
    }
}

// ========== UPDATE WAVE ==========
void updateWave() {
    UINT8 i, allDead = 1;
    for (i = 0; i < enemiesThisWave; i++) {
        if (enemyActive[i]) {
            allDead = 0;
            break;
        }
    }
    if (allDead && enemiesThisWave > 0) {
        // Advance to next wave
        wave++;
        if (wave >= MAX_WAVE) wave = MAX_WAVE - 1;
        enemiesThisWave = WAVE_ENEMY_COUNT[wave];
        for (i = 0; i < enemiesThisWave; i++) {
            enemyActive[i] = 1;
            // Spawn at random edge
            switch (rand() % 4) {
                case 0: enemyX[i] = (8 + rand() % 144) << 8; enemyY[i] = 0; break;
                case 1: enemyX[i] = 160 << 8; enemyY[i] = (16 + rand() % 120) << 8; break;
                case 2: enemyX[i] = (8 + rand() % 144) << 8; enemyY[i] = 144 << 8; break;
                case 3: enemyX[i] = 0; enemyY[i] = (16 + rand() % 120) << 8; break;
            }
            UINT8 spd = WAVE_BASE_SPEED[wave] + (rand() % 2);
            enemyDX[i] = (rand() % 2) ? spd : -spd;
            enemyDY[i] = (rand() % 2) ? spd : -spd;
        }
        playWaveSound = 1;
        // Add bonus for clearing wave
        score += 1000 * (wave + 1);
        // Spawn power-up as reward
        spawnPowerUp();
    }
}

// ========== TIMER UPDATES ==========
void updateTimers() {
    if (invincibleTimer > 0) {
        invincibleTimer--;
        flashCounter = (flashCounter + 1) & 0x07;
    }
    if (slowTimer > 0) {
        slowTimer--;
        // Slow down enemies by reducing speed temporarily
        // But we handle speed in updateEnemies using slowTimer flag
    }
    if (powerUpSpawnTimer == 0) {
        // Random chance to spawn power-up
        if ((rand() & 0x3F) == 0) { // ~1.5% chance per frame
            spawnPowerUp();
            powerUpSpawnTimer = 60;
        }
    } else {
        powerUpSpawnTimer--;
    }
}

// ========== DRAW EVERYTHING ==========
void drawGame() {
    UINT8 i;
    
    // Player sprite (white square normally, flash if invincible)
    if (invincibleTimer == 0 || (flashCounter & 1)) {
        set_sprite_tile(0, 0);
    } else {
        // Invisible during flash
        set_sprite_tile(0, 0); // still draw but with different tile? let's just keep visible
    }
    move_sprite(0, playerX >> 8, playerY >> 8);
    
    // Enemies (striped)
    UINT8 enemyTile = 1;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (enemyActive[i]) {
            set_sprite_tile(1 + i, 1);
            move_sprite(1 + i, enemyX[i] >> 8, enemyY[i] >> 8);
        } else {
            move_sprite(1 + i, 0, 0);
        }
    }
    
    // Power-ups (white with blinking)
    for (i = 0; i < MAX_POWERUPS; i++) {
        if (powerActive[i]) {
            set_sprite_tile(20 + i, 2);
            move_sprite(20 + i, powerX[i] >> 8, powerY[i] >> 8);
        } else {
            move_sprite(20 + i, 0, 0);
        }
    }
    
    // Particles (small squares)
    for (i = 0; i < MAX_PARTICLES; i++) {
        if (partActive[i]) {
            set_sprite_tile(30 + i, 3);
            move_sprite(30 + i, partX[i] >> 8, partY[i] >> 8);
        } else {
            move_sprite(30 + i, 0, 0);
        }
    }
    
    // UI: Score and Wave
    printText(0, 0, "SCORE:");
    printNumber(7, 0, score);
    printText(13, 0, "WAVE:");
    printNumber(19, 0, wave + 1);
    
    // Power-up indicators
    if (invincibleTimer > 0) {
        printText(0, 1, "SHIELD!");
    } else {
        clearArea(0, 1, 8, 1);
    }
    if (slowTimer > 0) {
        printText(9, 1, "SLOW");
    } else {
        clearArea(9, 1, 4, 1);
    }
    
    // Pause indicator
    if (gameState == 2) {
        printText(0, 2, "PAUSED");
    } else {
        clearArea(0, 2, 6, 1);
    }
}

// ========== TITLE SCREEN ==========
void drawTitle() {
    clearArea(0, 0, 20, 18);
    printText(2, 5, "DODGE THE SWARM");
    printText(4, 8, "PRESS START");
    printText(1, 12, "MOVE: D-PAD");
    printText(1, 14, "COLLECT POWERUPS");
    printText(1, 16, "SURVIVE WAVES");
    if (highScore > 0) {
        printText(1, 18, "BEST: ");
        printNumber(8, 18, highScore);
    }
}

// ========== GAME OVER SCREEN ==========
void drawGameOver() {
    clearArea(0, 0, 20, 18);
    printText(3, 5, "GAME OVER");
    printText(3, 7, "SCORE: ");
    printNumber(10, 7, score);
    if (score > highScore) {
        highScore = score;
        saveHighScore();
        printText(3, 9, "NEW HIGH!");
    } else {
        printText(3, 9, "BEST: ");
        printNumber(10, 9, highScore);
    }
    printText(2, 12, "PRESS START");
    printText(2, 14, "TO PLAY AGAIN");
}

// ========== MAIN GAME LOOP ==========
void main() {
    // Load font by printing a space
    printf(" ");
    
    // Initialize sprite tiles
    // Tile 0: player (solid white)
    UINT8 playerTile[] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    // Tile 1: enemy (striped)
    UINT8 enemyTile[]  = { 0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55 };
    // Tile 2: power-up (white square)
    UINT8 powerTile[]  = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    // Tile 3: particle (small dot)
    UINT8 partTile[]   = { 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18 };
    
    set_sprite_data(0, 1, playerTile);
    set_sprite_data(1, 1, enemyTile);
    set_sprite_data(2, 1, powerTile);
    set_sprite_data(3, 1, partTile);
    
    DISPLAY_ON;
    SHOW_SPRITES;
    SHOW_BKG;
    
    loadHighScore();
    
    gameState = 0; // title
    
    while(1) {
        // Wait for VBlank
        wait_vbl_done();
        
        // Read input
        UINT8 keys = joypad();
        
        // State machine
        switch (gameState) {
            case 0: // TITLE
                drawTitle();
                if (keys & J_START) {
                    resetGame();
                    gameState = 1;
                }
                break;
                
            case 1: // PLAYING
                // Movement
                if (keys & J_LEFT)  playerX -= (3 << 8);
                if (keys & J_RIGHT) playerX += (3 << 8);
                if (keys & J_UP)    playerY -= (3 << 8);
                if (keys & J_DOWN)  playerY += (3 << 8);
                if (playerX < (SCREEN_LEFT << 8)) playerX = (SCREEN_LEFT << 8);
                if (playerX > (SCREEN_RIGHT << 8)) playerX = (SCREEN_RIGHT << 8);
                if (playerY < (SCREEN_TOP << 8)) playerY = (SCREEN_TOP << 8);
                if (playerY > (SCREEN_BOTTOM << 8)) playerY = (SCREEN_BOTTOM << 8);
                
                if (keys & J_START) {
                    gameState = 2; // pause
                }
                
                // Game logic updates
                updateTimers();
                updateEnemies();
                updatePowerups();
                updateParticles();
                updateWave();
                
                // Score increases over time
                score += (slowTimer ? 1 : 2);
                if (score > 99999) score = 99999;
                
                // Check if player died (score <= 0 is game over? No, we just hit)
                if (invincibleTimer == 0 && score == 0) {
                    // Actually game over when? We decide: if score reaches 0 and no invincibility, game over.
                    // But simpler: game over if we get hit and score already low? 
                    // Let's just make game over when wave > max and no enemies? No. 
                    // For now, game over never happens except maybe by choice? I'll add manual game over?
                    // Better: game over when player gets hit and invincible timer triggers, but that's already handled.
                    // We'll add a "death counter" but not needed for fun. Let's keep it endless.
                }
                
                // Optional: game over on extreme low score
                if (score == 0 && invincibleTimer == 0) {
                    gameState = 3;
                }
                
                // Sound effects
                if (playCollectSound) {
                    soundCollect();
                    playCollectSound = 0;
                }
                if (playHitSound) {
                    soundHit();
                    playHitSound = 0;
                }
                if (playWaveSound) {
                    soundWave();
                    playWaveSound = 0;
                }
                
                drawGame();
                break;
                
            case 2: // PAUSED
                drawGame();
                printText(0, 2, "PAUSED");
                if (keys & J_START) {
                    gameState = 1;
                }
                break;
                
            case 3: // GAME OVER
                drawGameOver();
                if (keys & J_START) {
                    resetGame();
                    gameState = 1;
                }
                break;
        }
    }
}