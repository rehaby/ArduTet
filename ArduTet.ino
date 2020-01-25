#include <stdint.h>
#include <Arduboy2.h>

#include "t.h"

#define BLOCK_WIDTH 3

#define PLAY_START_X ((WIDTH / 2) - ((PLAYAREA_WIDTH  * BLOCK_WIDTH) / 2))
#define PLAY_START_Y ((HEIGHT / 2) - ((PLAYAREA_HEIGHT * BLOCK_WIDTH) / 2))
#define PLAY_END_X (PLAY_START_X + (PLAYAREA_WIDTH  * BLOCK_WIDTH))
#define PLAY_END_Y (PLAY_START_Y + (PLAYAREA_HEIGHT * BLOCK_WIDTH))

static Arduboy2 arduboy;

static Tet_State gTetState;
static ETet_GameState lastState = eGameOver;
static Point oldPlayTetromino[4] = {};
static Point oldNextTetromino[4] = {};
static uint8_t oldLevel = 30;
static uint32_t oldScore = 0xFFFFFFFF;
static PROGMEM const uint8_t blockBitmap[3] = {0x7,0x5,0x7};

static inline int16_t
ConvertPlayAreaToScreenX(int16_t x);

static inline int16_t
ConvertPlayAreaToScreenY(int16_t y);

static void
DrawBlock(int16_t X, int16_t Y, uint8_t Colour);

static void
DrawSideBorder();

static void
DrawBottomBorder();

static void
DrawTetronmino(Piece * Piece, int16_t X, int16_t Y, Point * oldState);

static void
DrawCurrentPlayArea(uint8_t * PlayArea , uint8_t TopMostBlock);

static void
DrawLabelsAndBox(void);

static void
DrawLevelAndScore(uint16_t level, uint32_t score);

static void
DrawGameState(uint8_t UpdateType);

static void
DrawStateWithText(const char * __restrict__ const text);

static void
DrawCpuUsage(void);

static void
DrawLevelSelect(uint16_t level);

static void
DrawBoarder(void);

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  arduboy.begin();
  arduboy.setFrameRate(60);
  arduboy.initRandomSeed();

  init_tet(&gTetState, 0, 0);
}

void loop() {
  // put your main code here, to run repeatedly:
   // pause render until it's time for the next frame
  if (!(arduboy.nextFrame())) {
    return;
  }

  arduboy.pollButtons();

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
    oldScore = 0xFFFFFFF;
    oldLevel = 30;
    lastState = gTetState.m_State;
    updateType = PIECE_UPDATED | PLAYAREA_UPDATED | PLAYAREA_LINE;


    switch(gTetState.m_State) {
    case eInit:
      DrawStateWithText("ArduTet");
      break;
    case eLevelSelection:
      DrawLevelSelect(gTetState.m_Level);
      break;
    case eGame:
      DrawCpuUsage();
      DrawBoarder();
      DrawLabelsAndBox();
      DrawGameState(updateType);
      break;
    case ePaused:
      DrawBoarder();
      DrawGameState(updateType);
      DrawStateWithText("Pause");
      break;
    case eGameOver:
      DrawStateWithText("Game Over");
      break;
    }
  } else {

    switch(gTetState.m_State) {
    case eInit:
    case ePaused:
    case eGameOver:
      break;
    case eLevelSelection:
      DrawLevelSelect(gTetState.m_Level);
      break;
    case eGame:
      DrawCpuUsage();
      DrawGameState(updateType);
      break;
    }
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
  arduboy.drawBitmap(X, Y, blockBitmap, BLOCK_WIDTH, BLOCK_WIDTH, Colour);
}

static void
DrawSolidBlock(int16_t X, int16_t Y, uint8_t Colour)
{
  arduboy.fillRect(X,Y,BLOCK_WIDTH, BLOCK_WIDTH, Colour);
}

static void
DrawSideBorder()
{
  arduboy.drawFastVLine(PLAY_START_X - 1, PLAY_START_Y, PLAY_END_Y - PLAY_START_Y + 1, WHITE);
  arduboy.drawFastVLine(PLAY_END_X,       PLAY_START_Y, PLAY_END_Y - PLAY_START_Y + 1, WHITE);

  arduboy.drawFastVLine(PLAY_START_X - BLOCK_WIDTH, PLAY_START_Y, PLAY_END_Y - PLAY_START_Y + BLOCK_WIDTH, WHITE);
  arduboy.drawFastVLine(PLAY_END_X + 2,             PLAY_START_Y, PLAY_END_Y - PLAY_START_Y + BLOCK_WIDTH, WHITE);
}

static void
DrawBottomBorder()
{
  arduboy.drawFastHLine(PLAY_START_X - 1, PLAY_END_Y, PLAY_END_X - PLAY_START_X + 1, WHITE);
  arduboy.drawFastHLine(PLAY_START_X - 2, PLAY_END_Y + 2, PLAY_END_X - PLAY_START_X + 4, WHITE);
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
DrawTetronmino(Piece * Piece, int16_t X, int16_t Y, Point * oldState)
{
  Point newTetromino[4];
  uint8_t i = 0;
  for (uint8_t y = 0; y < 4; ++y) {
    for (uint8_t x = 0; x < 4; ++x) {
      if (GetTerominoPart(Piece->m_Teromino, x, y, Piece->m_Rotation)) {
        newTetromino[i  ].x = X + (x * BLOCK_WIDTH);
        newTetromino[i++].y = Y + (y * BLOCK_WIDTH);
      }
    }
  }

  if (!compare_coord_array(newTetromino, oldState, 4)) {
    for (uint8_t i = 0; i < 4; ++i) {
      if (oldState[i].x > 0 && oldState[i].y > 0) {
        DrawBlock(oldState[i].x, oldState[i].y, BLACK);
      }
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
  DrawSideBorder();
  DrawBottomBorder();

  // Draw caps
  arduboy.drawPixel(PLAY_START_X - 2 , PLAY_START_Y,   WHITE);
  arduboy.drawPixel(PLAY_END_X + 1 ,   PLAY_START_Y,   WHITE);
}

static void
DrawCurrentPlayArea(uint8_t * PlayArea, uint8_t TopMostBlock)
{
  // We only draw from the top most block down
  // This should make drawing faster
  arduboy.fillRect(PLAY_START_X, TopMostBlock, PLAY_END_X - PLAY_START_X, PLAY_END_Y - TopMostBlock, BLACK);
  int16_t pixelX = PLAY_START_X;
  int16_t pixelY = PLAY_START_Y;
  for (int16_t y = TopMostBlock; y < PLAYAREA_HEIGHT; ++y, pixelY += BLOCK_WIDTH) {
    for (int16_t x = 0; x < PLAYAREA_WIDTH; ++x, pixelX += BLOCK_WIDTH) {
      uint8_t v = PlayArea[y * PLAYAREA_WIDTH + x];
      if (v > 0) {
        DrawBlock(ConvertPlayAreaToScreenX(x), ConvertPlayAreaToScreenY(y), v == 1 ? WHITE : BLACK);
      }
    }
  }
}

static void
DrawCpuUsage(void)
{
  /*static int oldCpuLoad = 0;

  int cpuLoad = arduboy.cpuLoad();
  if (cpuLoad != oldCpuLoad) {
    oldCpuLoad = cpuLoad;
    Serial.println(oldCpuLoad);
  }*/
}

static void
DrawLabelsAndBox(void)
{
  arduboy.setCursor(0, 0);
  arduboy.print("Level:");
  arduboy.setCursor(PLAY_END_X + 5, 0);
  arduboy.print("Score:");
}

static void
DrawLevelAndScore(uint16_t level, uint32_t score)
{
  if (oldLevel != level) {
    oldLevel = level;
    arduboy.setCursor(0, 10);
    arduboy.print(level);
  }

  if (oldScore != score) {
    oldScore = score;
    arduboy.setCursor(PLAY_END_X + 5, 10);
    arduboy.print(score);
  }

  /* Box for next piece */
  const int16_t nextPieceX = PLAY_END_X + 20;
  const int16_t nextPieceY = PLAY_START_Y + 20;
  arduboy.drawRect(nextPieceX - 1, nextPieceY - 1, 4 * BLOCK_WIDTH + 2, 4 * BLOCK_WIDTH + 2);
}

static void
DrawGameState(uint8_t UpdateType)
{
  if (UpdateType & PLAYAREA_UPDATED) {
    const int16_t nextPieceX = PLAY_END_X + 20;
    const int16_t nextPieceY = PLAY_START_Y + 20;
    DrawTetronmino(&(gTetState.m_NextPiece), nextPieceX, nextPieceY, oldNextTetromino);
    memset(oldPlayTetromino, 0, sizeof(Point) * 4);
  }

  if (UpdateType & PIECE_UPDATED) {
    const int16_t pieceX = ConvertPlayAreaToScreenX(gTetState.m_Piece.m_X);
    const int16_t pieceY = ConvertPlayAreaToScreenY(gTetState.m_Piece.m_Y);
    DrawTetronmino(&(gTetState.m_Piece), pieceX, pieceY, oldPlayTetromino);
    DrawLevelAndScore(gTetState.m_Level, gTetState.m_Score);
  }

  if (UpdateType & PLAYAREA_LINE) {
    DrawCurrentPlayArea(&(gTetState.m_PlayArea[0]), gTetState.m_TopMostBlockInBoard);
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
  if (oldLevel != level) {
    oldLevel = level;
    DrawStateWithText("Select Level");
    arduboy.setCursor(0,20);
    arduboy.print(level);
  }
}
