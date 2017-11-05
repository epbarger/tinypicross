/*
 * Tiny Arduboy version of Picross
 */

#include <Tinyfont.h>
#include <Arduboy2.h>
#include "puzzles.h"

#define GRID_WIDTH 15
#define GRID_HEIGHT 7
#define CELL_SIZE 7
#define GRID_X_OFFSET 32
#define GRID_Y_OFFSET 20
#define COLUMN_HINT_MAX_NUMS 3
#define ROW_HINT_MAX_NUMS 5

// Cell states
#define CS_EMPTY 0
#define CS_X 1
#define CS_FILL 2

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
    int cursorX;
    int cursorY;
};

struct Puzzle {
  public:
    bool cellFilled[GRID_WIDTH][GRID_HEIGHT];
    byte columnHints[GRID_WIDTH][COLUMN_HINT_MAX_NUMS];
    byte rowHints[GRID_HEIGHT][ROW_HINT_MAX_NUMS];
};

Arduboy2 arduboy;
byte screenWidth = Arduboy2::width();
byte screenHeight = Arduboy2::height();
Tinyfont tinyfont = Tinyfont(arduboy.sBuffer, screenWidth, screenHeight);

Grid gameGrid;
Puzzle gamePuzzle;

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(30);
  arduboy.initRandomSeed();
  Serial.begin(9600);
  Serial.println(puzzle1[0], BIN);
  initializePuzzle();
  initializeGrid();
}

void loop() {
  if (!(arduboy.nextFrame())) return;
  updateState();
  drawState();
}

///////////// Updates /////////////////

void updateState(){
  arduboy.pollButtons();
  updateGrid();
}

void updateGrid(){  
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
  } else if (arduboy.justPressed(B_BUTTON)){
    if (cursorCell()->state == CS_FILL){
      cursorCell()->state = CS_EMPTY;
    } else {
      if (gamePuzzle.cellFilled[gameGrid.cursorX][gameGrid.cursorY]) {
        cursorCell()->state = CS_FILL;
      }
    }
  }
}

///////////// Draws //////////////////

void drawState(){
  arduboy.clear();
  drawHUD();
  drawGrid();
  arduboy.display();
}

void drawHUD(){
//  arduboy.drawFastHLine(0, GRID_Y_OFFSET, screenWidth / 2, WHITE);
//  arduboy.drawFastVLine(GRID_X_OFFSET, 0, screenHeight / 2, WHITE);
}

void drawGrid(){
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

void drawHintColumn(byte columnIndex){
  byte drawIndexX = GRID_X_OFFSET + 2 + (columnIndex * (CELL_SIZE - 1));
  byte drawIndexY = GRID_Y_OFFSET - 7;
  byte zeroHints = 0;
  tinyfont.setTextColor(gameGrid.cursorX != columnIndex);
  for (int i = COLUMN_HINT_MAX_NUMS - 1; i >= 0; i--){
    byte hintNum = gamePuzzle.columnHints[columnIndex][i];
    if (hintNum > 0) {
      tinyfont.setCursor(drawIndexX, drawIndexY);
      tinyfont.print(String(hintNum));
      drawIndexY -= 6;
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
      if (hintNum > 9) { drawIndexX -= 5; }
      tinyfont.setCursor(drawIndexX, drawIndexY);
      tinyfont.print(String(hintNum));
      drawIndexX -= 6;
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

void initializePuzzle(){
  for (byte x = 0; x < GRID_WIDTH; x++){
    for (byte y = 0; y < GRID_HEIGHT; y++){
      const byte * puzzleToLoad = &puzzles[0];
      bool cellFilled = pgm_read_byte_near(&puzzleToLoad[x]) & pgm_read_byte_near(&bitMaskForYIndex[y]);
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
      newCell.state = CS_EMPTY; // gamePuzzle.cellFilled[x][y]; 
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

