#include <stdint.h>
#include <Arduboy2.h>

#include "t.h"

#define BLOCK_WIDTH 3

#define PLAY_START_X ((WIDTH / 2) - ((PLAYAREA_WIDTH  * BLOCK_WIDTH) / 2))
#define PLAY_START_Y ((HEIGHT / 2) - ((PLAYAREA_HEIGHT * BLOCK_WIDTH) / 2))
#define PLAY_END_X (PLAY_START_X + (PLAYAREA_WIDTH  * BLOCK_WIDTH))
#define PLAY_END_Y (PLAY_START_Y + (PLAYAREA_HEIGHT * BLOCK_WIDTH))

Arduboy2 arduboy;

Tet_State gTetState;
ETet_GameState lastState = eGameOver;
Point oldPlayTetromino[4] = {};
Point oldNextTetromino[4] = {};


static inline int16_t
ConvertPlayAreaToScreenX(int16_t x);

static inline int16_t
ConvertPlayAreaToScreenY(int16_t y);

static void 
DrawBlock(int16_t X, int16_t Y, uint8_t Colour);

static void 
DrawSideBorder(int16_t X, int16_t Y);

static void 
DrawBottomBorder(int16_t X, int16_t Y);

static void 
DrawTeronmino(Piece * Piece, int16_t X, int16_t Y, Point * oldState);

static void 
DrawCurrentPlayArea(uint8_t * PlayArea);

static void
DrawLevelAndScore(uint16_t level, uint32_t score);

static void 
DrawGameState(uint8_t UpdateType);

static void
DrawStateWithText(const char * __restrict__ const text);

static void
DrawLevelSelect(uint16_t level);


void setup() {
  // put your setup code here, to run once:
  arduboy.begin();
  arduboy.setFrameRate(60);
  arduboy.initRandomSeed();

  Serial.write("SETUP\n");
  Serial.flush();

  init_tet(&gTetState, 0, 0);
}

void loop() {
  // put your main code here, to run repeatedly:
   // pause render until it's time for the next frame
  if (!(arduboy.nextFrame())) {
    return;
  }

  arduboy.pollButtons();
  //arduboy.clear();

  EButtons pressed = eNone;
  if (arduboy.pressed(A_BUTTON)) {
    pressed = eA;
  } else if (arduboy.pressed(B_BUTTON)) {
    pressed = eB;
  } else if (arduboy.pressed(LEFT_BUTTON)) {
    pressed = eLeft;
  } else if (arduboy.pressed(RIGHT_BUTTON)) {
    pressed = eRight;
  } else if (arduboy.pressed(UP_BUTTON)) {
    pressed = eUp;
  } else if (arduboy.pressed(DOWN_BUTTON)) {
    pressed = eDown;
  }

  uint8_t updateType = DoGameTick(pressed, &gTetState);

  if (lastState != gTetState.m_State) {
    arduboy.clear();
    lastState = gTetState.m_State;
    updateType = PIECE_UPDATED | PLAYAREA_UPDATED;
  }

  switch(gTetState.m_State) {
  case eInit:
    DrawStateWithText("ArduTet");
    break;
  case eLevelSelection:
    DrawLevelSelect(gTetState.m_Level);
    break;
  case eGame:
    DrawGameState(updateType);
    break;
  case ePaused:
    DrawGameState(updateType);
    DrawStateWithText("Pause");
    break;
  case eGameOver:
    DrawStateWithText("Game Over");
    break;
  }

  arduboy.display();
}

static inline int16_t
ConvertPlayAreaToScreenX(int16_t x)
{
  return (x * BLOCK_WIDTH) + PLAY_START_X;
}

static inline int16_t
ConvertPlayAreaToScreenY(int16_t y)
{
  return (y * BLOCK_WIDTH) + PLAY_START_Y;
}


static void 
DrawBlock(int16_t X, int16_t Y, uint8_t Colour) 
{
  arduboy.drawPixel(X + 0 , Y + 0, Colour);
  arduboy.drawPixel(X + 0 , Y + 1, Colour);
  arduboy.drawPixel(X + 0 , Y + 2, Colour);
  
  arduboy.drawPixel(X + 1 , Y + 0, Colour);
  arduboy.drawPixel(X + 1 , Y + 1, Colour == WHITE ? BLACK : WHITE);
  arduboy.drawPixel(X + 1 , Y + 2, Colour);
  
  arduboy.drawPixel(X + 2 , Y + 0, Colour);
  arduboy.drawPixel(X + 2 , Y + 1, Colour);
  arduboy.drawPixel(X + 2 , Y + 2, Colour);
}

static void
DrawSolidBlock(int16_t X, int16_t Y, uint8_t Colour)
{
  //arduboy.drawRect(X,Y,BLOCK_WIDTH, Colour);
  arduboy.drawPixel(X + 0 , Y + 0, Colour);
  arduboy.drawPixel(X + 0 , Y + 1, Colour);
  arduboy.drawPixel(X + 0 , Y + 2, Colour);
  
  arduboy.drawPixel(X + 1 , Y + 0, Colour);
  arduboy.drawPixel(X + 1 , Y + 1, Colour);
  arduboy.drawPixel(X + 1 , Y + 2, Colour);
  
  arduboy.drawPixel(X + 2 , Y + 0, Colour);
  arduboy.drawPixel(X + 2 , Y + 1, Colour);
  arduboy.drawPixel(X + 2 , Y + 2, Colour);
}

static void 
DrawSideBorder(int16_t X, int16_t Y) 
{
  arduboy.drawPixel(X + 0 , Y + 0, WHITE);
  arduboy.drawPixel(X + 0 , Y + 1, WHITE);
  arduboy.drawPixel(X + 0 , Y + 2, WHITE);
  
  arduboy.drawPixel(X + 1 , Y + 0, BLACK);
  arduboy.drawPixel(X + 1 , Y + 1, BLACK);
  arduboy.drawPixel(X + 1 , Y + 2, BLACK);
  
  arduboy.drawPixel(X + 2 , Y + 0, WHITE);
  arduboy.drawPixel(X + 2 , Y + 1, WHITE);
  arduboy.drawPixel(X + 2 , Y + 2, WHITE);
}

static void 
DrawBottomBorder(int16_t X, int16_t Y) 
{
  arduboy.drawPixel(X + 0 , Y + 0, WHITE);
  arduboy.drawPixel(X + 0 , Y + 1, BLACK);
  arduboy.drawPixel(X + 0 , Y + 2, WHITE);
  
  arduboy.drawPixel(X + 1 , Y + 0, WHITE);
  arduboy.drawPixel(X + 1 , Y + 1, BLACK);
  arduboy.drawPixel(X + 1 , Y + 2, WHITE);
  
  arduboy.drawPixel(X + 2 , Y + 0, WHITE);
  arduboy.drawPixel(X + 2 , Y + 1, BLACK);
  arduboy.drawPixel(X + 2 , Y + 2, WHITE);
}

static int
compare_coord(Point * a, Point * b)
{
  return (a && b && a->x == b->x && a->y == b->y);
}



static int
compare_coord_array(Point * a, Point * b, int n)
{
  int i;
  for (i = 0; i < n; ++i) {
    if (!compare_coord(&(a[i]), &(b[i]))) {
      return 0;
    }
  }
  return 1;
}


static void 
DrawTeronmino(Piece * Piece, int16_t X, int16_t Y, Point * oldState)
{
  Point newTetromino[4];
  uint8_t i = 0;
  for (uint8_t y = 0; y < 4; ++y) {
    for (uint8_t x = 0; x < 4; ++x) {
      if (GetTerominoPart(Piece->m_Teromino, x, y, Piece->m_Rotation)) {
        //DrawBlock(X + (x * BLOCK_WIDTH), Y + (y * BLOCK_WIDTH), WHITE);
        newTetromino[i  ].x = X + (x * BLOCK_WIDTH);
        newTetromino[i++].y = Y + (y * BLOCK_WIDTH);
      }
    }
  }

  if (!compare_coord_array(newTetromino, oldState, 4)) {
    for (uint8_t i = 0; i < 4; ++i) {
      DrawSolidBlock(oldState[i].x, oldState[i].y, BLACK);
    }

    for (uint8_t i = 0; i < 4; ++i) {
      DrawBlock(newTetromino[i].x, newTetromino[i].y, WHITE);
    }
    memcpy(oldState, newTetromino, sizeof(Point) * 4);
  }
}

static void
DrawBoarder(void)
{
  // TODO make this heaps better
  
  // Fill in play area
  for (int16_t y = PLAY_START_Y; y <= PLAY_END_Y; ++y) {
      DrawSideBorder(PLAY_START_X - BLOCK_WIDTH,y);
      DrawSideBorder(PLAY_END_X,y);
  }
  for (int16_t x = PLAY_START_X; x <= PLAY_END_X - BLOCK_WIDTH; ++x) {
    DrawBottomBorder(x,PLAY_END_Y);
  }
  
  // Draw caps
  arduboy.drawPixel(PLAY_START_X - 2 , PLAY_START_Y,   WHITE);
  arduboy.drawPixel(PLAY_START_X- 2,   PLAY_END_Y + 2, WHITE);
  arduboy.drawPixel(PLAY_END_X + 1 ,   PLAY_START_Y,   WHITE);
  arduboy.drawPixel(PLAY_END_X + 1 ,   PLAY_END_Y + 2, WHITE);
  
  // Create hole in sides
  arduboy.drawPixel(PLAY_START_X - 1, PLAY_END_Y + 1, BLACK);
  arduboy.drawPixel(PLAY_END_X  ,     PLAY_END_Y + 1, BLACK);
}

static void 
DrawCurrentPlayArea(uint8_t * PlayArea)
{
  // TODO make this heaps better
  for (int16_t y = 0; y < PLAYAREA_HEIGHT; ++y) {
    for (int16_t x = 0; x < PLAYAREA_WIDTH; ++x) {
      uint8_t v = PlayArea[y * PLAYAREA_WIDTH + x];
      if (v > 0) {
        DrawBlock(ConvertPlayAreaToScreenX(x), ConvertPlayAreaToScreenY(y), v == 1 ? WHITE : BLACK);
      } else {
        DrawSolidBlock(ConvertPlayAreaToScreenX(x), ConvertPlayAreaToScreenY(y),BLACK);
      }
    }
  }
}

static void
DrawLevelAndScore(uint16_t level, uint32_t score)
{
  arduboy.setCursor(0, 0);
  arduboy.print("L: ");
  arduboy.print(level);
  arduboy.setCursor(0, 10);
  arduboy.print("S: ");
  arduboy.print(score);

  arduboy.setCursor(0, 20);
  arduboy.print("C: ");
  arduboy.print(arduboy.cpuLoad());
}

static void 
DrawGameState(uint8_t UpdateType)
{
  if (UpdateType & PLAYAREA_UPDATED) {
    DrawBoarder();
    DrawCurrentPlayArea(&(gTetState.m_PlayArea[0]));
    
    const int16_t nextPieceX = PLAY_END_X + 20;
    const int16_t nextPieceY = PLAY_START_Y + 20;
    DrawTeronmino(&(gTetState.m_NextPiece), nextPieceX, nextPieceY, oldNextTetromino);
    memset(oldPlayTetromino, 0, sizeof(Point) * 4);
  }


  if (UpdateType & PIECE_UPDATED) {
    const int16_t pieceX = ConvertPlayAreaToScreenX(gTetState.m_Piece.m_X);
    const int16_t pieceY = ConvertPlayAreaToScreenY(gTetState.m_Piece.m_Y);
    DrawTeronmino(&(gTetState.m_Piece), pieceX, pieceY, oldPlayTetromino);

    DrawLevelAndScore(gTetState.m_Level, gTetState.m_Score);
  }

  
}

static void
DrawStateWithText(const char * __restrict__ const text)
{
  arduboy.setTextSize(2);
  arduboy.setCursor(0, 0);
  arduboy.print(text);
  arduboy.setTextSize(1);
}


static void
DrawLevelSelect(uint16_t level)
{
  DrawStateWithText("Select Level");
  arduboy.setCursor(0,20);
  arduboy.print(level);
}
