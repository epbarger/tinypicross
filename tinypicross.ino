/*
 * Tiny Arduboy version of Picross
 * 
 * TODO
 * - timer?
 */

#include <Arduboy2.h>
#include "tinyfont.h"
#include "puzzles.h"

#define GRID_WIDTH 15
#define GRID_HEIGHT 7
#define CELL_SIZE 7
#define GRID_X_OFFSET 32
#define GRID_Y_OFFSET 20
#define COLUMN_HINT_MAX_NUMS 3
#define ROW_HINT_MAX_NUMS 5

#define MENU_X_OFFSET 18
#define MENU_Y_OFFSET 8
#define MENU_WIDTH 18
#define MENU_HEIGHT 9

#define BASE_EEPROM_LOCATION 100
#define SUPPORTED_PUZZLE_COUNT MENU_WIDTH * MENU_HEIGHT

// Cell states
#define CS_EMPTY 0
#define CS_X 1
#define CS_FILL 2

// Game states
#define GS_MENU 0
#define GS_PLAYING 1
#define GS_PAUSED 2
#define GS_WIN 3
#define GS_DELAY 4

const byte bitMaskForYIndex[7] PROGMEM {
  0b01000000,
  0b00100000,
  0b00010000,
  0b00001000,
  0b00000100,
  0b00000010,
  0b00000001
};

struct Cell {
  public:
    byte state;
};

struct Grid {
  public:
    Cell cells[GRID_WIDTH][GRID_HEIGHT];
    char cursorX;
    char cursorY;
};

struct Puzzle {
  public:
    bool cellFilled[GRID_WIDTH][GRID_HEIGHT]; // wasteful, but we have extra memory
    byte columnHints[GRID_WIDTH][COLUMN_HINT_MAX_NUMS + 1]; // possible index bound issues?
    byte rowHints[GRID_HEIGHT][ROW_HINT_MAX_NUMS +1 ]; // possible index bound issues?
    byte puzzleIndex;
    bool eepromState;
};

struct Menu {
  public:
    char cursorX;
    char cursorY;
    byte cursorPuzzleNumber;
};

Arduboy2 arduboy;
byte screenWidth = Arduboy2::width();
byte screenHeight = Arduboy2::height();
Tinyfont tinyfont = Tinyfont(arduboy.sBuffer, screenWidth, screenHeight);

Grid gameGrid;
Puzzle gamePuzzle;
Menu menu;

byte gameState = GS_MENU;
bool okayToAbandon = false;

void setup() {
  arduboy.boot();
  arduboy.blank();
  arduboy.flashlight();
  arduboy.systemButtons();

  arduboy.setFrameRate(30);
  arduboy.initRandomSeed();
  Serial.begin(9600);
  
  menu.cursorX = 0;
  menu.cursorY = 0;
  menu.cursorPuzzleNumber = 1;
  initEEPROM();
}

void loop() {
  if (!(arduboy.nextFrame())) return;
  updateState();
  drawState();
}

///////////// Updates /////////////////

void updateState(){
  arduboy.pollButtons();
  switch(gameState){
    case GS_MENU:
      updateMenu();
      break;
    case GS_PLAYING:
      updateGrid();
      break;
    case GS_PAUSED:
      updatePaused();
      break;
    case GS_DELAY:
      delay(2000);
      gameState = GS_WIN;
      break;
    case GS_WIN:
      updateWin();
      break;
  }
}

void updateWin(){
  if (arduboy.justPressed(A_BUTTON | B_BUTTON)) {
    gameState = GS_MENU;
  }
}

void updatePaused(){
  if (!okayToAbandon && arduboy.notPressed(UP_BUTTON | DOWN_BUTTON | LEFT_BUTTON | RIGHT_BUTTON)){
    okayToAbandon = true;
  }
  
  if (okayToAbandon && arduboy.pressed(UP_BUTTON | DOWN_BUTTON | LEFT_BUTTON | RIGHT_BUTTON)){
    okayToAbandon = false;
    gameState = GS_MENU;
  } else if (arduboy.justPressed(A_BUTTON | B_BUTTON)) {
    okayToAbandon = false;
    gameState = GS_PLAYING;
  }
}

void updateMenu(){
  if (arduboy.justPressed(RIGHT_BUTTON)){
    menu.cursorX = modulo(menu.cursorX + 1, MENU_WIDTH);
  } else if (arduboy.justPressed(LEFT_BUTTON)){
    menu.cursorX = modulo(menu.cursorX - 1, MENU_WIDTH);
  }

  if (arduboy.justPressed(DOWN_BUTTON)){
    menu.cursorY = modulo(menu.cursorY + 1, MENU_HEIGHT);
  } else if (arduboy.justPressed(UP_BUTTON)){
    menu.cursorY = modulo(menu.cursorY - 1, MENU_HEIGHT);
  } 

  menu.cursorPuzzleNumber = (menu.cursorX + 1) + (MENU_WIDTH * (menu.cursorY));

  if (arduboy.justPressed(A_BUTTON | B_BUTTON)){
    gameState = GS_PLAYING;
    startGame(menu.cursorPuzzleNumber - 1);
  }
}

void updateGrid(){  
  if (arduboy.pressed(UP_BUTTON | DOWN_BUTTON | LEFT_BUTTON | RIGHT_BUTTON)){
    gameState = GS_PAUSED;
  } else {
    if (arduboy.justPressed(RIGHT_BUTTON)){
      gameGrid.cursorX = modulo(gameGrid.cursorX + 1, GRID_WIDTH);
    } else if (arduboy.justPressed(LEFT_BUTTON)){
      gameGrid.cursorX = modulo(gameGrid.cursorX - 1, GRID_WIDTH);
    }
  
    if (arduboy.justPressed(DOWN_BUTTON)){
      gameGrid.cursorY = modulo(gameGrid.cursorY + 1, GRID_HEIGHT);
    } else if (arduboy.justPressed(UP_BUTTON)){
      gameGrid.cursorY = modulo(gameGrid.cursorY - 1, GRID_HEIGHT);
    }
  
    if (arduboy.justPressed(A_BUTTON)){
      if (cursorCell()->state == CS_X){
        cursorCell()->state = CS_EMPTY;
      } else {
        cursorCell()->state = CS_X;
      }
      checkPuzzleComplete();
    } else if (arduboy.justPressed(B_BUTTON)){
      if (cursorCell()->state == CS_FILL){
        cursorCell()->state = CS_EMPTY;
      } else {
        cursorCell()->state = CS_FILL;
      }
      checkPuzzleComplete();
    }
  }
}

///////////// Draws //////////////////

void drawState(){
  arduboy.clear();
  switch(gameState){
    case GS_MENU:
      drawTitle();
      drawPuzzleSelection();
      break;
    case GS_PLAYING:
      drawGrid();
      break;
    case GS_PAUSED:
      drawGrid();
      drawMessage(F("RETURN TO MENU?"),27,30);
      break;
    case GS_DELAY:
      drawGrid();
      break;
    case GS_WIN:
      drawGrid();
      drawMessage(F("PUZZLE COMPLETE!"), 25, 30);
  }
  arduboy.display();
}

void drawMessage(String msg, byte x, byte y){
  arduboy.fillRect(screenWidth / 8, screenHeight / 3, screenWidth - (screenWidth / 4), screenHeight - (2*screenHeight / 3), BLACK);
  arduboy.drawRect(screenWidth / 8, screenHeight / 3, screenWidth - (screenWidth / 4), screenHeight - (2*screenHeight / 3), WHITE);
  tinyfont.setCursor(x,y);
  tinyfont.print(msg);
}

void drawTitle(){
  arduboy.fillRect(0,0,screenWidth,6,WHITE);
  tinyfont.setTextColor(BLACK);
  tinyfont.setCursor(1,1);
  tinyfont.print(F("TINY PICROSS"));
  tinyfont.setTextColor(WHITE);
}

void drawPuzzleSelection(){
  for (byte x = 0; x < MENU_WIDTH; x++){
    for (byte y = 0; y < MENU_HEIGHT; y++){
      byte drawX = MENU_X_OFFSET + (x * (CELL_SIZE - 1));
      byte drawY = MENU_Y_OFFSET + (y * (CELL_SIZE - 1));
      if (EEPROM.read(BASE_EEPROM_LOCATION + (x+1) + (MENU_WIDTH * y))){
        arduboy.fillRect(drawX+1, drawY+1, CELL_SIZE-2, CELL_SIZE-2, WHITE);
      } else {
        arduboy.drawLine(drawX + 2, drawY + 2, drawX + 4, drawY + 4, WHITE);
        arduboy.drawLine(drawX + 4, drawY + 2, drawX + 2, drawY + 4, WHITE);
      }
    }
  }

  byte cursorDrawX = MENU_X_OFFSET + (menu.cursorX * (CELL_SIZE - 1));
  byte cursorDrawY = MENU_Y_OFFSET + (menu.cursorY * (CELL_SIZE - 1));
  arduboy.drawRect(cursorDrawX, cursorDrawY, CELL_SIZE, CELL_SIZE, BLACK);
  arduboy.drawRect(cursorDrawX - 1, cursorDrawY - 1, CELL_SIZE + 2, CELL_SIZE + 2, WHITE);

  String cursorPuzzleNumber = String(menu.cursorPuzzleNumber);
  drawMenuRow(cursorPuzzleNumber);
}

void drawGrid(){
  char first = '>';
  char last = '<';
  if (gamePuzzle.eepromState){
    char tmp = first;
    first = last;
    last = tmp;
  }
  tinyfont.setTextColor(WHITE);
  tinyfont.setCursor(1,1);
  tinyfont.print(first + String(gamePuzzle.puzzleIndex + 1) + last);
  
  for (byte x = 0; x < GRID_WIDTH; x++){
    for (byte y = 0; y < GRID_HEIGHT; y++){
      byte drawX = GRID_X_OFFSET + (x * (CELL_SIZE - 1));
      byte drawY = GRID_Y_OFFSET + (y * (CELL_SIZE - 1));
      switch(gameGrid.cells[x][y].state){
        case CS_EMPTY:
          break;
        case CS_X:
          arduboy.drawLine(drawX + 2, drawY + 2, drawX + 4, drawY + 4, WHITE);
          arduboy.drawLine(drawX + 4, drawY + 2, drawX + 2, drawY + 4, WHITE);
          break;
        case CS_FILL:
          arduboy.fillRect(drawX+1, drawY+1, CELL_SIZE-2, CELL_SIZE-2, WHITE);
          break;
      }
    }
  }

  arduboy.fillRect(GRID_X_OFFSET + 1 + (gameGrid.cursorX * (CELL_SIZE - 1)), 0, CELL_SIZE - 1, GRID_Y_OFFSET - 2, WHITE);

  for (byte i = 0; i < GRID_WIDTH; i++){
    drawHintColumn(i);
  }

  arduboy.fillRect(0, GRID_Y_OFFSET + 1 + (gameGrid.cursorY * (CELL_SIZE - 1)), GRID_X_OFFSET - 2, CELL_SIZE - 1, WHITE);

  for (byte i = 0; i < GRID_HEIGHT; i++){
    drawHintRow(i);
  }

  byte cursorDrawX = GRID_X_OFFSET + (gameGrid.cursorX * (CELL_SIZE - 1));
  byte cursorDrawY = GRID_Y_OFFSET + (gameGrid.cursorY * (CELL_SIZE - 1));
  arduboy.drawRect(cursorDrawX, cursorDrawY, CELL_SIZE, CELL_SIZE, BLACK);
  arduboy.drawRect(cursorDrawX - 1, cursorDrawY - 1, CELL_SIZE + 2, CELL_SIZE + 2, WHITE);
}

void drawMenuRow(String str){
  arduboy.fillRect(0, MENU_Y_OFFSET + 1 + (menu.cursorY * (CELL_SIZE - 1)), MENU_X_OFFSET - 2, CELL_SIZE - 1, WHITE);

  byte drawIndexX = MENU_X_OFFSET - 7;
  byte drawIndexY = MENU_Y_OFFSET + 2 + (menu.cursorY * (CELL_SIZE - 1));
  tinyfont.setTextColor(BLACK);
  for (char i = str.length() - 1; i >= 0; i--){
    tinyfont.setCursor(drawIndexX, drawIndexY);
    tinyfont.print(str.charAt(i));
    drawIndexX -= 5;
  }
  tinyfont.setTextColor(WHITE);
}

void drawHintColumn(byte columnIndex){
  byte drawIndexX = GRID_X_OFFSET + 2 + (columnIndex * (CELL_SIZE - 1));
  byte drawIndexY = GRID_Y_OFFSET - 7;
  byte zeroHints = 0;
  tinyfont.setTextColor(gameGrid.cursorX != columnIndex);
  for (char i = COLUMN_HINT_MAX_NUMS - 1; i >= 0; i--){
    byte hintNum = gamePuzzle.columnHints[columnIndex][i];
    if (hintNum > 0) {
      tinyfont.setCursor(drawIndexX, drawIndexY);
      tinyfont.print(String(hintNum));
      drawIndexY -= 5;
    } else {
      zeroHints++;
    }
  }
  if (zeroHints >= COLUMN_HINT_MAX_NUMS){
    tinyfont.setCursor(drawIndexX, drawIndexY);
    tinyfont.print(F("0"));
  }
  tinyfont.setTextColor(WHITE);
}

void drawHintRow(byte rowIndex){
  byte drawIndexX = GRID_X_OFFSET - 7;
  byte drawIndexY = GRID_Y_OFFSET + 2 + (rowIndex * (CELL_SIZE - 1));
  byte zeroHints = 0;
  tinyfont.setTextColor(gameGrid.cursorY != rowIndex);
  for (int i = ROW_HINT_MAX_NUMS - 1; i >= 0; i--){
    byte hintNum = gamePuzzle.rowHints[rowIndex][i];
    if (hintNum > 0) {
      if (hintNum > 9) { 
        drawIndexX -= 4;
        tinyfont.setCursor(drawIndexX, drawIndexY);
        tinyfont.print(String(hintNum).charAt(0));
        tinyfont.setCursor(drawIndexX+4, drawIndexY);
        tinyfont.print(String(hintNum).charAt(1));
        drawIndexX -= 8;
      } else {
        tinyfont.setCursor(drawIndexX, drawIndexY);
        tinyfont.print(String(hintNum));
        drawIndexX -= 6;
      }
    } else {
      zeroHints++;
    }
  }
  if (zeroHints >= ROW_HINT_MAX_NUMS){
    tinyfont.setCursor(drawIndexX, drawIndexY);
    tinyfont.print(F("0"));
  }
  tinyfont.setTextColor(WHITE);
}


/////////////////// Other /////////////

void initializePuzzle(byte puzzleIndex){
  gamePuzzle.puzzleIndex = puzzleIndex;
  gamePuzzle.eepromState = EEPROM.read(BASE_EEPROM_LOCATION + 1 + gamePuzzle.puzzleIndex);
  for (byte x = 0; x < GRID_WIDTH; x++){
    for (byte y = 0; y < GRID_HEIGHT; y++){
      bool cellFilled = pgm_read_byte_near(&puzzles[puzzleIndex][x]) & pgm_read_byte_near(&bitMaskForYIndex[y]);
      gamePuzzle.cellFilled[x][y] = cellFilled;
      for (byte i = 0; i < ROW_HINT_MAX_NUMS; i++){
        gamePuzzle.rowHints[y][i] = 0;
      }
    }
    for (byte i = 0; i < COLUMN_HINT_MAX_NUMS; i++){
      gamePuzzle.columnHints[x][i] = 0;
    }
  }

  for (byte x = 0; x < GRID_WIDTH; x++){
    byte hintNum = 0;
    byte hintIndex = 0;
    for (byte y = 0; y < GRID_HEIGHT; y++){
      if (gamePuzzle.cellFilled[x][y]){
        hintNum++;
      } else if (hintNum > 0) {
        gamePuzzle.columnHints[x][hintIndex] = hintNum;
        hintNum = 0;
        hintIndex++;
      }

      if (!(y + 1 < GRID_HEIGHT)){
        gamePuzzle.columnHints[x][hintIndex] = hintNum;
      }
    }
  }

  
  for (byte y = 0; y < GRID_HEIGHT; y++){
    byte hintNum = 0;
    byte hintIndex = 0;
    for (byte x = 0; x < GRID_WIDTH; x++){
      if (gamePuzzle.cellFilled[x][y]){
        hintNum++;
      } else if (hintNum > 0) {
        gamePuzzle.rowHints[y][hintIndex] = hintNum;
        hintNum = 0;
        hintIndex++;
      }

      if (!(x + 1 < GRID_WIDTH)){
        gamePuzzle.rowHints[y][hintIndex] = hintNum;
      }
    }
  }  
}

void initializeGrid(){
  for (byte x = 0; x < GRID_WIDTH; x++){
    for (byte y = 0; y < GRID_HEIGHT; y++){
      Cell newCell;
      newCell.state = CS_EMPTY;
      gameGrid.cells[x][y] = newCell;
    }
  }
  gameGrid.cursorX = 0;
  gameGrid.cursorY = 0;
}

Cell* cursorCell(){
  return &gameGrid.cells[gameGrid.cursorX][gameGrid.cursorY];
}

int modulo(int x, int y){
  return x < 0 ? ((x + 1) % y) + y - 1 : x % y;
}

void startGame(byte puzzleIndex){
  initializePuzzle(puzzleIndex);
  initializeGrid();
}

void checkPuzzleComplete(){
  for (byte x = 0; x < GRID_WIDTH; x++){
    for (byte y = 0; y < GRID_HEIGHT; y++){
      if (gamePuzzle.cellFilled[x][y] != (gameGrid.cells[x][y].state == CS_FILL)) {
        return;
      }
    }
  }
  EEPROM.put(BASE_EEPROM_LOCATION + 1 + gamePuzzle.puzzleIndex, true);
  gameState = GS_DELAY;
}

void initEEPROM(){
  byte checksum = dumbPuzzleChecksum();
  if (EEPROM.read(BASE_EEPROM_LOCATION) != checksum) {
    EEPROM.put(BASE_EEPROM_LOCATION, checksum);
    clearEEPROM();
  }
}

void clearEEPROM(){
  for (byte puzzle = 1; puzzle <= SUPPORTED_PUZZLE_COUNT; puzzle++){
    EEPROM.put(BASE_EEPROM_LOCATION + puzzle, 0);
  }
}

byte dumbPuzzleChecksum(){
  return puzzles[0][0] ^ puzzles[1][1] ^ puzzles[2][2] ^ 0x69;
}

