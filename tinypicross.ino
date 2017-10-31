/*
 * Tiny Arduboy version of Picross
 */

#include <Tinyfont.h>
#include <Arduboy2.h>

#define GRID_WIDTH 15
#define GRID_HEIGHT 7
#define CELL_SIZE 7
#define GRID_X_OFFSET 32
#define GRID_Y_OFFSET 20

// Cell states
#define CS_EMPTY 0
#define CS_X 1
#define CS_FILL 2

struct Cell {
  public:
    byte state;
    bool good;
};

struct Grid {
  public:
    Cell cells[GRID_WIDTH][GRID_HEIGHT];
    byte cursorX;
    byte cursorY;
};

Arduboy2 arduboy;
byte screenWidth = Arduboy2::width();
byte screenHeight = Arduboy2::height();
Tinyfont tinyfont = Tinyfont(arduboy.sBuffer, screenWidth, screenHeight);

Grid gameGrid;

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(30);
  arduboy.initRandomSeed();
  initializeGrid();
}

void loop() {
  if (!(arduboy.nextFrame())) return;
  updateState();
  drawState();
}

///////////// Updates /////////////////

void updateState(){
  updateGrid();
}

void updateGrid(){
  if (arduboy.pressed(RIGHT_BUTTON)){
    gameGrid.cursorX = (gameGrid.cursorX + 1) % GRID_WIDTH;
  } else if (arduboy.pressed(LEFT_BUTTON)){
    gameGrid.cursorX = (gameGrid.cursorX - 1) % GRID_WIDTH;
  }

  if (arduboy.pressed(DOWN_BUTTON)){
    gameGrid.cursorY = (gameGrid.cursorY + 1) % GRID_HEIGHT;
  } else if (arduboy.pressed(UP_BUTTON)){
    gameGrid.cursorY = (gameGrid.cursorY - 1) % GRID_HEIGHT;
  }
}

///////////// Draws //////////////////

void drawState(){
  arduboy.clear();
  drawGrid();
  
//  tinyfont.setCursor(0, gridYOffset + 1);
//  tinyfont.print("1");
//  tinyfont.setCursor(6, gridYOffset + 1);
//  tinyfont.print("2");
//  tinyfont.setCursor(12, gridYOffset + 1);
//  tinyfont.print("3");
//  tinyfont.setCursor(18, gridYOffset + 1);
//  tinyfont.print("4");
//  tinyfont.setCursor(24, gridYOffset + 1);
//  tinyfont.print("5");
//
//  tinyfont.setCursor(gridXOffset + 1, 0);
//  tinyfont.print("1");
//  tinyfont.setCursor(gridXOffset + 1, 6);
//  tinyfont.print("2");
//  tinyfont.setCursor(gridXOffset + 1, 12);
//  tinyfont.print("3");

  arduboy.display();
}

void drawGrid(){
  for (byte x = 0; x < GRID_WIDTH; x++){
    for (byte y = 0; y < GRID_HEIGHT; y++){
      byte drawX = GRID_X_OFFSET + (x * (CELL_SIZE - 1));
      byte drawY = GRID_Y_OFFSET + (y * (CELL_SIZE - 1));
      arduboy.drawRect(drawX, drawY, CELL_SIZE, CELL_SIZE, WHITE);
      switch(gameGrid.cells[x][y].state){
        case CS_EMPTY:
          break;
        case CS_X:
          arduboy.drawLine(drawX + 2, drawY + 2, drawX + 4, drawY + 4, WHITE);
          arduboy.drawLine(drawX + 4, drawY + 2, drawX + 2, drawY + 4, WHITE);
          break;
        case CS_FILL:
          arduboy.fillRect(drawX, drawY, CELL_SIZE, CELL_SIZE, WHITE);
          break;
      }
    }
  }

  byte cursorDrawX = GRID_X_OFFSET + (gameGrid.cursorX * (CELL_SIZE - 1));
  byte cursorDrawY = GRID_Y_OFFSET + (gameGrid.cursorY * (CELL_SIZE - 1));
  arduboy.drawRect(cursorDrawX, cursorDrawY, CELL_SIZE, CELL_SIZE, WHITE);
  arduboy.drawRect(cursorDrawX - 1, cursorDrawY - 1, CELL_SIZE + 2, CELL_SIZE + 2, WHITE);
}

/////////////////// Other /////////////

void initializeGrid(){
  for (byte x = 0; x < GRID_WIDTH; x++){
    for (byte y = 0; y < GRID_HEIGHT; y++){
      Cell newCell;
      newCell.state = random(0, 3);
      newCell.good = true;
      gameGrid.cells[x][y] = newCell;
    }
  }
  gameGrid.cursorX = 0;
  gameGrid.cursorY = 0;
}

