#include <gb/gb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rand.h>

// ---------- TILE PLACEHOLDERS ----------
// (Will be replaced with actual art later)
const unsigned char playerTile[]   = {0xFF,0x81,0x81,0x81,0x81,0x81,0x81,0xFF, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char enemyTile[]    = {0x00,0x7E,0x42,0x42,0x42,0x42,0x7E,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char grassTile[]    = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

// ---------- GAME STATES ----------
typedef enum {
    STATE_OVERWORLD,
    STATE_BATTLE,
    STATE_MENU,
    STATE_SHOP,
    STATE_GAMEOVER
} GameState;

GameState gameState = STATE_OVERWORLD;

// ---------- PLAYER STATS ----------
typedef struct {
    char name[12];
    uint16_t hp, maxHp;
    uint8_t level, exp, nextExp;
    uint8_t str, def, agi;
    uint16_t gold;
    // Inventory simple (items are just potions)
    uint8_t potions;
} Player;

Player player;

// ---------- WORLD MAP ----------
// 256x256 grid (procedurally generated on start)
// Using simple tile IDs
#define WORLD_W 64   // Reduced for memory, but feels large due to zones
#define WORLD_H 64
uint8_t worldTiles[WORLD_W][WORLD_H];
uint8_t worldZone[WORLD_W][WORLD_H];  // 0=field,1=forest,2=mountain,3=dungeon

uint8_t playerX, playerY;      // world coordinates (0..WORLD_W-1)
uint8_t screenX, screenY;      // camera offset (0..WORLD_W-20, etc.)

// ---------- ENEMY DATA ----------
typedef struct {
    char name[12];
    uint8_t level;
    uint16_t hp, maxHp;
    uint8_t str, def, agi;
    uint16_t expReward, goldReward;
} Enemy;

Enemy enemies[] = {
    {"Slime",   1, 8,8, 2,1,1, 10, 5},
    {"Goblin",  2,12,12,3,2,2, 20,10},
    {"Wolf",    3,18,18,4,2,3, 35,15},
    {"Orc",     5,35,35,6,4,2, 60,30},
    {"Skeleton",7,50,50,7,5,4,100,50},
    {"Ogre",   10,85,85,12,8,3,200,100},
    {"Dragon", 15,200,200,20,15,6,500,300}
};
#define NUM_ENEMIES 7

Enemy currentEnemy;

// ---------- BATTLE VARIABLES ----------
uint8_t battleTurn = 0;  // 0 = player, 1 = enemy
uint8_t battleAction;    // 1=attack, 2=item, 3=flee
uint8_t battleRunning = 1;

// ---------- SHOP ----------
typedef struct {
    uint16_t potionPrice;
    uint16_t healPrice;
} Shop;
Shop currentShop = {50, 80};

// ---------- MENU STATE ----------
uint8_t menuSelection = 0;
const char* menuOptions[] = {"Status", "Use Potion", "Save", "Quit"};

// ---------- UTILITIES ----------
void updateDisplay();
void drawWorld();
void drawUI();
void battleLoop();
void doPlayerAttack();
void doEnemyAttack();
void gainExp(uint16_t exp);
void showMessage(const char* msg);
void waitForKey();

// ---------- RANDOM ENCOUNTERS ----------
uint8_t encounterRate = 32;  // 1 in 32 steps
uint8_t stepCounter = 0;

// ---------- WORLD GENERATION (simple but varied) ----------
void generateWorld() {
    // Fill with grass (0)
    for (uint8_t y=0; y<WORLD_H; y++) {
        for (uint8_t x=0; x<WORLD_W; x++) {
            worldTiles[x][y] = 0;
            // Zone based on Perlin-like noise using rand
            uint8_t val = (x * 131) ^ (y * 253);
            val = (val + (val>>4)) & 0x0F;
            if (val < 3) worldZone[x][y] = 1;      // forest
            else if (val < 5) worldZone[x][y] = 2; // mountain
            else if (val == 5) worldZone[x][y] = 3;// dungeon entrance
            else worldZone[x][y] = 0;
        }
    }
    // Place a town in the center
    for (int8_t y=-2; y<=2; y++) {
        for (int8_t x=-2; x<=2; x++) {
            if (WORLD_W/2+x < WORLD_W && WORLD_H/2+y < WORLD_H) {
                worldTiles[WORLD_W/2+x][WORLD_H/2+y] = 4; // town tile
                worldZone[WORLD_W/2+x][WORLD_H/2+y] = 4;  // special zone
            }
        }
    }
}

// ---------- PLAYER INIT ----------
void initPlayer() {
    strcpy(player.name, "Hero");
    player.level = 1;
    player.exp = 0;
    player.nextExp = 50;
    player.maxHp = 30;
    player.hp = player.maxHp;
    player.str = 5;
    player.def = 4;
    player.agi = 3;
    player.gold = 100;
    player.potions = 3;
}

// ---------- LEVEL UP ----------
void levelUp() {
    while (player.exp >= player.nextExp) {
        player.exp -= player.nextExp;
        player.level++;
        player.maxHp += 8 + (rand() % 5);
        player.str += 1 + (rand() % 3);
        player.def += 1 + (rand() % 2);
        player.agi += 1 + (rand() % 2);
        player.hp = player.maxHp;
        player.nextExp = player.level * 50;
        showMessage("Level Up!");
        waitForKey();
    }
}

// ---------- BATTLE REWARDS ----------
void endBattle(uint8_t victory) {
    if (victory) {
        gainExp(currentEnemy.expReward);
        player.gold += currentEnemy.goldReward;
        char msg[20];
        sprintf(msg, "Got %u G", currentEnemy.goldReward);
        showMessage(msg);
        waitForKey();
    }
    gameState = STATE_OVERWORLD;
    battleRunning = 1;
    updateDisplay();
}

// ---------- BATTLE LOOP ----------
void battleLoop() {
    if (!battleRunning) return;
    drawUI();
    printf("\n\n %s Lv.%d\n HP:%d/%d\n", currentEnemy.name, currentEnemy.level,
           currentEnemy.hp, currentEnemy.maxHp);
    printf("\n1.Attack 2.Item\n3.Flee");
    waitForKey();
    uint8_t key = joypad();
    if (key & J_A) battleAction = 1;
    else if (key & J_B) battleAction = 2;
    else if (key & J_START) battleAction = 3;
    else return;

    if (battleAction == 1) doPlayerAttack();
    else if (battleAction == 2) {
        if (player.potions > 0 && player.hp < player.maxHp) {
            player.hp += 20;
            if (player.hp > player.maxHp) player.hp = player.maxHp;
            player.potions--;
            showMessage("Used potion!");
            waitForKey();
        } else showMessage("No potion / full HP!");
        waitForKey();
    }
    else if (battleAction == 3) {
        // flee chance based on agility
        if ((rand() % 100) < (player.agi * 10)) {
            showMessage("Fled safely!");
            waitForKey();
            gameState = STATE_OVERWORLD;
            battleRunning = 0;
            return;
        } else {
            showMessage("Couldn't flee!");
            waitForKey();
        }
    }

    if (currentEnemy.hp <= 0) {
        endBattle(1);
        return;
    }

    // Enemy turn
    doEnemyAttack();
    if (player.hp <= 0) {
        showMessage("You died...");
        waitForKey();
        gameState = STATE_GAMEOVER;
        return;
    }
    wait_vbl_done();
}

void doPlayerAttack() {
    uint16_t damage = player.str + (rand() % (player.str/2 + 2)) - (currentEnemy.def/2);
    if (damage < 1) damage = 1;
    currentEnemy.hp -= damage;
    char msg[20];
    sprintf(msg, "Dealt %u dmg!", damage);
    showMessage(msg);
}

void doEnemyAttack() {
    uint16_t damage = currentEnemy.str + (rand() % (currentEnemy.str/2 + 1)) - (player.def/2);
    if (damage < 1) damage = 1;
    player.hp -= damage;
    char msg[20];
    sprintf(msg, "Enemy hits for %u", damage);
    showMessage(msg);
}

// ---------- GAIN EXP ----------
void gainExp(uint16_t exp) {
    player.exp += exp;
    levelUp();
}

// ---------- SHOPPING ----------
void shopMenu() {
    drawUI();
    printf("\nSHOP\n1.Potion(%uG)\n2.Heal(%uG)\n3.Exit", currentShop.potionPrice, currentShop.healPrice);
    uint8_t key = joypad();
    if (key & J_A) {
        if (player.gold >= currentShop.potionPrice) {
            player.gold -= currentShop.potionPrice;
            player.potions++;
            showMessage("Bought potion");
        } else showMessage("Not enough gold");
    } else if (key & J_B) {
        if (player.gold >= currentShop.healPrice && player.hp < player.maxHp) {
            player.gold -= currentShop.healPrice;
            player.hp = player.maxHp;
            showMessage("Fully healed");
        } else showMessage("Full HP or poor");
    } else if (key & J_START) gameState = STATE_OVERWORLD;
    waitForKey();
}

// ---------- MENU HANDLING ----------
void mainMenu() {
    drawUI();
    printf("\n%s Lv.%d\nHP:%d/%d\nG:%u\nPots:%d\n\n", player.name, player.level,
           player.hp, player.maxHp, player.gold, player.potions);
    printf("1.Status 2.Potion\n3.Save 4.Quit");
    uint8_t key = joypad();
    if (key & J_A) menuSelection = 0;
    else if (key & J_B) menuSelection = 1;
    else if (key & J_SELECT) menuSelection = 2;
    else if (key & J_START) menuSelection = 3;

    switch(menuSelection) {
        case 0: // Status
            showMessage("Show stats (placeholder)");
            waitForKey();
            break;
        case 1: // Use Potion
            if (player.potions>0 && player.hp<player.maxHp) {
                player.hp+=20;
                if(player.hp>player.maxHp) player.hp=player.maxHp;
                player.potions--;
                showMessage("Used potion");
            } else showMessage("No potion/full HP");
            waitForKey();
            break;
        case 2: // Save
            // In real GB, save to RAM; here just message
            showMessage("Game saved (mock)");
            waitForKey();
            break;
        case 3: // Quit to overworld
            gameState = STATE_OVERWORLD;
            break;
    }
}

// ---------- OVERWORLD MOVEMENT & ENCOUNTERS ----------
void handleOverworld() {
    uint8_t newX = playerX, newY = playerY;
    uint8_t moved = 0;
    uint8_t keys = joypad();

    if (keys & J_LEFT)  { newX--; moved=1; }
    if (keys & J_RIGHT) { newX++; moved=1; }
    if (keys & J_UP)    { newY--; moved=1; }
    if (keys & J_DOWN)  { newY++; moved=1; }

    if (moved && newX<WORLD_W && newY<WORLD_H) {
        playerX = newX;
        playerY = newY;
        stepCounter++;
        // Random encounter
        if (worldZone[playerX][playerY] != 4 && (rand() % encounterRate) == 0) {
            // pick enemy based on zone difficulty
            uint8_t enemyIdx = (worldZone[playerX][playerY] + player.level/2) % NUM_ENEMIES;
            if (enemyIdx >= NUM_ENEMIES) enemyIdx = NUM_ENEMIES-1;
            currentEnemy = enemies[enemyIdx];
            gameState = STATE_BATTLE;
            battleRunning = 1;
            return;
        }
    }

    // Town zone (center) opens shop if START pressed
    if (worldZone[playerX][playerY] == 4 && (joypad() & J_START)) {
        gameState = STATE_SHOP;
    }
    if (joypad() & J_SELECT) {
        gameState = STATE_MENU;
    }

    // Update camera
    screenX = playerX - 10;
    if (screenX > WORLD_W-20) screenX = WORLD_W-20;
    if (screenX < 0) screenX = 0;
    screenY = playerY - 8;
    if (screenY > WORLD_H-16) screenY = WORLD_H-16;
    if (screenY < 0) screenY = 0;
}

// ---------- DRAWING ----------
void drawWorld() {
    // Simple tilemap drawing (placeholder)
    for (uint8_t y=0; y<16; y++) {
        for (uint8_t x=0; x<20; x++) {
            uint8_t wx = screenX + x;
            uint8_t wy = screenY + y;
            if (wx < WORLD_W && wy < WORLD_H) {
                // Use different tile IDs based on zone and if it's player position
                uint8_t tile = 0;
                if (wx == playerX && wy == playerY) tile = 1; // player sprite
                else if (worldZone[wx][wy] == 4) tile = 4;
                else if (worldZone[wx][wy] == 1) tile = 2;
                else if (worldZone[wx][wy] == 2) tile = 3;
                else tile = 0;
                set_bkg_tile_xy(x, y, tile);
            }
        }
    }
}

void drawUI() {
    // Clear bottom 2 rows for HUD
    for (uint8_t i=0; i<20; i++) {
        set_bkg_tile_xy(i, 17, 0);
        set_bkg_tile_xy(i, 16, 0);
    }
    gotoxy(0,16);
    printf("Lv.%d HP:%d/%d G:%u", player.level, player.hp, player.maxHp, player.gold);
}

// ---------- MESSAGE & WAIT ----------
void showMessage(const char* msg) {
    gotoxy(0,17);
    printf("%-20s", msg);
    wait_vbl_done();
    delay(1000);
}

void waitForKey() {
    while(!joypad());
    while(joypad());
}

// ---------- MAIN GAME LOOP ----------
void main() {
    DISPLAY_ON;
    SHOW_BKG;
    SHOW_SPRITES;
    // Init tiles (placeholder)
    set_bkg_data(0, 1, grassTile);
    set_sprite_data(0, 1, playerTile);
    set_sprite_data(1, 1, enemyTile);
    // Random seed
    initrand(DIV_REG);
    generateWorld();
    initPlayer();
    playerX = WORLD_W/2;
    playerY = WORLD_H/2;
    screenX = playerX - 10;
    screenY = playerY - 8;

    while(1) {
        switch(gameState) {
            case STATE_OVERWORLD:
                handleOverworld();
                drawWorld();
                drawUI();
                break;
            case STATE_BATTLE:
                battleLoop();
                break;
            case STATE_MENU:
                mainMenu();
                break;
            case STATE_SHOP:
                shopMenu();
                break;
            case STATE_GAMEOVER:
                gotoxy(0,10);
                printf("GAME OVER");
                waitForKey();
                initPlayer();
                playerX = WORLD_W/2;
                playerY = WORLD_H/2;
                gameState = STATE_OVERWORLD;
                break;
        }
        wait_vbl_done();
    }
}