#include <gb/gb.h>
#include <gbdk/console.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rand.h>

// Tile placeholders (replace with your graphics later)
const unsigned char playerTile[] = {0xFF,0x81,0x81,0x81,0x81,0x81,0x81,0xFF, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char enemyTile[]  = {0x00,0x7E,0x42,0x42,0x42,0x42,0x7E,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char grassTile[]  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

typedef enum { STATE_OVERWORLD, STATE_BATTLE, STATE_MENU, STATE_SHOP, STATE_GAMEOVER } GameState;
GameState gameState = STATE_OVERWORLD;

typedef struct {
    char name[12];
    UINT16 hp, maxHp;
    UINT8 level, exp, nextExp;
    UINT8 str, def, agi;
    UINT16 gold;
    UINT8 potions;
} Player;
Player player;

#define WORLD_W 64
#define WORLD_H 64
UINT8 worldTiles[WORLD_W][WORLD_H];
UINT8 worldZone[WORLD_W][WORLD_H];

UINT8 playerX, playerY, screenX, screenY;
UINT8 encounterRate = 32;
UINT8 stepCounter = 0;

typedef struct {
    char name[12];
    UINT8 level;
    UINT16 hp, maxHp;
    UINT8 str, def, agi;
    UINT16 expReward, goldReward;
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

UINT8 battleRunning = 1;

typedef struct {
    UINT16 potionPrice;
    UINT16 healPrice;
} Shop;
Shop currentShop = {50, 80};

UINT8 menuSelection = 0;

void updateDisplay(void);
void drawWorld(void);
void drawUI(void);
void battleLoop(void);
void doPlayerAttack(void);
void doEnemyAttack(void);
void gainExp(UINT16 exp);
void showMessage(const char* msg);
void waitForKey(void);
void generateWorld(void);
void initPlayer(void);
void levelUp(void);
void endBattle(UINT8 victory);
void shopMenu(void);
void mainMenu(void);
void handleOverworld(void);

// ---------- UTILITIES ----------
void showMessage(const char* msg) {
    gotoxy(0,17);
    printf("%-20s", msg);
    wait_vbl_done();
    delay(1000);
}
void waitForKey(void) {
    while(!joypad());
    while(joypad());
}

// ---------- WORLD ----------
void generateWorld(void) {
    UINT8 x,y,val;
    for(y=0; y<WORLD_H; y++) {
        for(x=0; x<WORLD_W; x++) {
            worldTiles[x][y] = 0;
            val = (x * 131) ^ (y * 253);
            val = (val + (val>>4)) & 0x0F;
            if (val < 3) worldZone[x][y] = 1;
            else if (val < 5) worldZone[x][y] = 2;
            else if (val == 5) worldZone[x][y] = 3;
            else worldZone[x][y] = 0;
        }
    }
    for(y=WORLD_H/2-2; y<=WORLD_H/2+2; y++) {
        for(x=WORLD_W/2-2; x<=WORLD_W/2+2; x++) {
            if(x<WORLD_W && y<WORLD_H) {
                worldTiles[x][y] = 4;
                worldZone[x][y] = 4;
            }
        }
    }
}

// ---------- PLAYER ----------
void initPlayer(void) {
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

void levelUp(void) {
    while(player.exp >= player.nextExp) {
        player.exp -= player.nextExp;
        player.level++;
        player.maxHp += 8 + (rand() % 5);
        player.str += 1 + (rand() % 3);
        player.def += 1 + (rand() % 2);
        player.agi += 1 + (rand() % 3);
        player.hp = player.maxHp;
        player.nextExp = player.level * 50;
        showMessage("Level Up!");
        waitForKey();
    }
}

void gainExp(UINT16 exp) {
    player.exp += exp;
    levelUp();
}

// ---------- BATTLE ----------
void doPlayerAttack(void) {
    UINT16 damage = player.str + (rand() % (player.str/2 + 2)) - (currentEnemy.def/2);
    if(damage < 1) damage = 1;
    currentEnemy.hp -= damage;
    char msg[20];
    sprintf(msg, "Dealt %u dmg!", damage);
    showMessage(msg);
}
void doEnemyAttack(void) {
    UINT16 damage = currentEnemy.str + (rand() % (currentEnemy.str/2 + 1)) - (player.def/2);
    if(damage < 1) damage = 1;
    player.hp -= damage;
    char msg[20];
    sprintf(msg, "Enemy hits for %u", damage);
    showMessage(msg);
}
void endBattle(UINT8 victory) {
    if(victory) {
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
void battleLoop(void) {
    if(!battleRunning) return;
    drawUI();
    printf("\n\n %s Lv.%d\n HP:%d/%d\n", currentEnemy.name, currentEnemy.level,
           currentEnemy.hp, currentEnemy.maxHp);
    printf("\n1.Attack 2.Item\n3.Flee");
    waitForKey();
    UINT8 key = joypad();
    UINT8 battleAction = 0;
    if(key & J_A) battleAction = 1;
    else if(key & J_B) battleAction = 2;
    else if(key & J_START) battleAction = 3;
    else return;

    if(battleAction == 1) doPlayerAttack();
    else if(battleAction == 2) {
        if(player.potions > 0 && player.hp < player.maxHp) {
            player.hp += 20;
            if(player.hp > player.maxHp) player.hp = player.maxHp;
            player.potions--;
            showMessage("Used potion!");
            waitForKey();
        } else showMessage("No potion / full HP!");
        waitForKey();
    }
    else if(battleAction == 3) {
        if((rand() % 100) < (player.agi * 10)) {
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
    if(currentEnemy.hp <= 0) { endBattle(1); return; }
    doEnemyAttack();
    if(player.hp <= 0) {
        showMessage("You died...");
        waitForKey();
        gameState = STATE_GAMEOVER;
        return;
    }
    wait_vbl_done();
}

// ---------- SHOP & MENU ----------
void shopMenu(void) {
    drawUI();
    printf("\nSHOP\n1.Potion(%uG)\n2.Heal(%uG)\n3.Exit", currentShop.potionPrice, currentShop.healPrice);
    UINT8 key = joypad();
    if(key & J_A) {
        if(player.gold >= currentShop.potionPrice) {
            player.gold -= currentShop.potionPrice;
            player.potions++;
            showMessage("Bought potion");
        } else showMessage("Not enough gold");
    } else if(key & J_B) {
        if(player.gold >= currentShop.healPrice && player.hp < player.maxHp) {
            player.gold -= currentShop.healPrice;
            player.hp = player.maxHp;
            showMessage("Fully healed");
        } else showMessage("Full HP or poor");
    } else if(key & J_START) gameState = STATE_OVERWORLD;
    waitForKey();
}
void mainMenu(void) {
    drawUI();
    printf("\n%s Lv.%d\nHP:%d/%d\nG:%u\nPots:%d\n\n", player.name, player.level,
           player.hp, player.maxHp, player.gold, player.potions);
    printf("1.Status 2.Potion\n3.Save 4.Quit");
    UINT8 key = joypad();
    if(key & J_A) menuSelection = 0;
    else if(key & J_B) menuSelection = 1;
    else if(key & J_SELECT) menuSelection = 2;
    else if(key & J_START) menuSelection = 3;
    switch(menuSelection) {
        case 0: showMessage("Stats: STR/DEF/AGI"); waitForKey(); break;
        case 1:
            if(player.potions>0 && player.hp<player.maxHp) {
                player.hp+=20;
                if(player.hp>player.maxHp) player.hp=player.maxHp;
                player.potions--;
                showMessage("Used potion");
            } else showMessage("No potion/full HP");
            waitForKey(); break;
        case 2: showMessage("Game saved (mock)"); waitForKey(); break;
        case 3: gameState = STATE_OVERWORLD; break;
    }
}

// ---------- OVERWORLD ----------
void handleOverworld(void) {
    UINT8 newX = playerX, newY = playerY, moved = 0;
    UINT8 keys = joypad();
    if(keys & J_LEFT)  { newX--; moved=1; }
    if(keys & J_RIGHT) { newX++; moved=1; }
    if(keys & J_UP)    { newY--; moved=1; }
    if(keys & J_DOWN)  { newY++; moved=1; }
    if(moved && newX<WORLD_W && newY<WORLD_H) {
        playerX = newX; playerY = newY;
        stepCounter++;
        if(worldZone[playerX][playerY] != 4 && (rand() % encounterRate) == 0) {
            UINT8 enemyIdx = (worldZone[playerX][playerY] + player.level/2) % NUM_ENEMIES;
            if(enemyIdx >= NUM_ENEMIES) enemyIdx = NUM_ENEMIES-1;
            currentEnemy = enemies[enemyIdx];
            gameState = STATE_BATTLE;
            battleRunning = 1;
            return;
        }
    }
    if(worldZone[playerX][playerY] == 4 && (joypad() & J_START)) gameState = STATE_SHOP;
    if(joypad() & J_SELECT) gameState = STATE_MENU;
    screenX = playerX - 10; if(screenX > WORLD_W-20) screenX = WORLD_W-20; if(screenX < 0) screenX = 0;
    screenY = playerY - 8;  if(screenY > WORLD_H-16) screenY = WORLD_H-16; if(screenY < 0) screenY = 0;
}

// ---------- DRAWING ----------
void drawWorld(void) {
    UINT8 x,y,wx,wy;
    for(y=0; y<16; y++) {
        for(x=0; x<20; x++) {
            wx = screenX + x; wy = screenY + y;
            if(wx < WORLD_W && wy < WORLD_H) {
                UINT8 tile = 0;
                if(wx == playerX && wy == playerY) tile = 1;
                else if(worldZone[wx][wy] == 4) tile = 4;
                else if(worldZone[wx][wy] == 1) tile = 2;
                else if(worldZone[wx][wy] == 2) tile = 3;
                else tile = 0;
                set_bkg_tile_xy(x, y, tile);
            }
        }
    }
}
void drawUI(void) {
    UINT8 i;
    for(i=0; i<20; i++) { set_bkg_tile_xy(i, 17, 0); set_bkg_tile_xy(i, 16, 0); }
    gotoxy(0,16);
    printf("Lv.%d HP:%d/%d G:%u", player.level, player.hp, player.maxHp, player.gold);
}
void updateDisplay(void) {
    drawWorld();
    drawUI();
}

// ---------- MAIN ----------
void main(void) {
    DISPLAY_ON;
    SHOW_BKG;
    SHOW_SPRITES;
    set_bkg_data(0, 1, grassTile);
    set_sprite_data(0, 1, playerTile);
    set_sprite_data(1, 1, enemyTile);
    initrand(DIV_REG);
    generateWorld();
    initPlayer();
    playerX = WORLD_W/2; playerY = WORLD_H/2;
    screenX = playerX - 10; screenY = playerY - 8;
    while(1) {
        switch(gameState) {
            case STATE_OVERWORLD:
                handleOverworld();
                updateDisplay();
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
                playerX = WORLD_W/2; playerY = WORLD_H/2;
                gameState = STATE_OVERWORLD;
                break;
        }
        wait_vbl_done();
    }
}