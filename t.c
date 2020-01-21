#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "t.h"

#define MIN(a,b) ( a < b ? a : b )

enum {
  TET_I = 0x2222,
  TET_L = 0x6220,
  TET_J = 0x0226,
  TET_O = 0x0660,
  TET_S = 0x0630,
  TET_T = 0x0720,
  TET_Z = 0x0360
};

typedef enum {
  eOneShot,
  eRepeatOnDelay,
  eRepeat
} EButtonBehaviour;

static const uint16_t kTetromino[] = { TET_I, TET_L, TET_J, TET_O, TET_S, TET_T, TET_Z };
#define TETRONMINO_COUNT (sizeof(kTetromino) / sizeof(int16_t))

/////////////////
// Globals

/////////////////
// Function Defs

static int16_t
DoesPieceFit(Tet_State *restrict pState, ERotation Rotation, int16_t X, int16_t Y);

static void
LockPieceInBoard(Tet_State *restrict pState);

static uint8_t
CheckForLines(Tet_State *restrict pState, int16_t Y);

static void
CleanUpLines(Tet_State *restrict pState);

static void
NewPiece(Tet_State *restrict pState, Piece *restrict pPiece);

static void
NextPiece(Tet_State *restrict pState);

static uint32_t
CalculateRowClearScore(Tet_State *restrict pState, uint8_t rows);

static uint8_t
ShouldActOnButtonPress(Tet_State *restrict pState, EButtons button, EButtonBehaviour behaviour);

static uint8_t
DoGameState(EButtons button, Tet_State *restrict pState);

static uint8_t
DoWaitForButtonPress(EButtons button, Tet_State *restrict pState, ETet_GameState nextState, uint8_t stateFrames);

static uint8_t
DoLevelSelect(EButtons button, Tet_State *restrict pState);

static void
TransitionGameState(Tet_State *restrict pState, ETet_GameState nextState);

static void
FillBag(Tet_State *restrict pState);

static void
ShuffleArray(uint8_t *restrict array, uint8_t n);

//////////////////
// Function Implementations

void
init_tet(Tet_State * pState, uint8_t Level, uint32_t TopScore )
{
  *pState = (Tet_State) {
    .m_Gravity = 60 + (Level * 2),
    .m_GravityFrames = 0,
    .m_Score = 0,
    .m_TopScore = TopScore,
    .m_PlayArea = { 0 },
    .m_Bag = { 0 },
    .m_BagIndex = 0,
    .m_Piece = { 0 },
    .m_Level = Level,
    .m_TotalRowsCleared = 0,
    .m_CurrentButton = eNone,
    .m_ButtonFrames = 0,
    .m_State = eInit,
    .m_StateFrames = 0
  };
  FillBag(pState);

  // Make sure first pieces are only
  // I, J, L, or T
  uint8_t *restrict bag = &(pState->m_Bag[0]);
  do {
    ShuffleArray(bag, BAG_COUNT);
  } while(bag[0] == 3 || bag[0] == 4 || bag[0] == 6);

  NewPiece(pState, &(pState->m_Piece));
  NewPiece(pState, &(pState->m_NextPiece));
}


uint8_t
GetTerominoPart(uint16_t Teromino, int16_t X, int16_t Y, ERotation Rotation)
{
  switch (Rotation) {
  case e0:
    return (Teromino >> (Y * 4 + X)) & 0x1;
  case e90:
    return (Teromino >> (12 + Y - (X * 4))) & 0x1;
  case e180:
    return (Teromino >> (15 - (Y * 4) - X)) & 01;
  case e270:
    return (Teromino >> (3 - Y + (X * 4))) & 0x1;
  }
  return 0;
}

static int16_t
DoesPieceFit(Tet_State *restrict pState, ERotation Rotation, int16_t X, int16_t Y)
{
  for (uint8_t y = 0; y < 4; ++y) {
    const int16_t pY = pState->m_Piece.m_Y + Y + y;

    for (uint8_t x = 0; x < 4; ++x) {
      const int16_t pX = pState->m_Piece.m_X + X + x;

      if (GetTerominoPart(pState->m_Piece.m_Teromino, x, y, Rotation)) {
        if (pY < 0 ||
            pY >= PLAYAREA_HEIGHT ||
            pX < 0 ||
            pX >= PLAYAREA_WIDTH ||
            pState->m_PlayArea[(pY * PLAYAREA_WIDTH) + pX] != 0) {
          return -1;
        }
      }
    }
  }
  return 0;
}

static void
LockPieceInBoard(Tet_State *restrict pState)
{
  for (uint8_t y = 0; y < 4; ++y) {
    const int16_t pY = pState->m_Piece.m_Y + y;

    for (uint8_t x = 0; x < 4; ++x) {
      const int16_t pX = pState->m_Piece.m_X + x;

      if (GetTerominoPart(pState->m_Piece.m_Teromino, x, y, pState->m_Piece.m_Rotation)) {
        pState->m_PlayArea[(pY * PLAYAREA_WIDTH) + pX] = 1;
      }
    }
  }
}

static void
NewPiece(Tet_State *restrict pState, Piece *restrict pPiece)
{
  if (pState->m_BagIndex >= BAG_COUNT) {
    ShuffleArray(pState->m_Bag, BAG_COUNT);
    pState->m_BagIndex = 0;
  }

 *pPiece = (Piece) {
    .m_Teromino = kTetromino[pState->m_Bag[pState->m_BagIndex++]],
    .m_Rotation = rand() % 4,
    .m_X        = (PLAYAREA_WIDTH / 2) - 1,
    .m_Y        = 0
  };
}

static void
NextPiece(Tet_State *restrict pState)
{
  memcpy(&(pState->m_Piece), &(pState->m_NextPiece), sizeof(Piece));
  NewPiece(pState, &(pState->m_NextPiece));
}

static uint8_t
CheckForLines(Tet_State *restrict pState, int16_t Y)
{
  int16_t found = 0;
  int16_t maxY = MIN(Y + 4, PLAYAREA_HEIGHT);
  for (int16_t y = Y; y < maxY; ++y) {
    int16_t line = 1;
    for(int16_t x = 0; x < PLAYAREA_WIDTH; ++x) {
      line &= pState->m_PlayArea[(y * PLAYAREA_WIDTH) + x] != 0;
    }

    if (line) {
      found += 1;
      for(int16_t x = 0; x < PLAYAREA_WIDTH; ++x) {
        pState->m_PlayArea[(y * PLAYAREA_WIDTH) + x] = 2;
      }
    }
  }
  return found;
}

static void
CleanUpLines(Tet_State *restrict pState)
{
  for (int16_t y = PLAYAREA_HEIGHT - 1; y >= 0; --y) {
    if (pState->m_PlayArea[y * PLAYAREA_WIDTH] == 2) {
      for (int16_t x = 0; x < PLAYAREA_WIDTH; ++x) {
        for (int16_t yy = y; yy >= 1; --yy) {
          pState->m_PlayArea[(yy * PLAYAREA_WIDTH) + x] =
            pState->m_PlayArea[ ((yy-1) * PLAYAREA_WIDTH) + x];
        }
        pState->m_PlayArea[x] = 0;
      }
      ++y; // process the line again as we moved it down
    }
  }
}


static uint32_t
CalculateRowClearScore(Tet_State *restrict pState, uint8_t rows)
{
  int16_t basePoints;
  switch (rows) {
  case 0:
    basePoints = 0;
    break;
  case 1:
    basePoints = 40;
    break;
  case 2:
    basePoints = 100;
    break;
  case 3:
    basePoints = 300;
    break;
  case 4:
  default:
    basePoints = 1200;
    break;
  }
  return basePoints * (pState->m_Level + 1);
}

static uint8_t
ShouldActOnButtonPress(Tet_State *restrict pState, EButtons button, EButtonBehaviour behaviour)
{
  switch (behaviour) {
  case eOneShot:
    if (pState->m_CurrentButton != button) {
      pState->m_CurrentButton = button;
      pState->m_ButtonFrames = 0;
      return 1;
    } else {
      return 0;
    }
  case eRepeatOnDelay:
    if (pState->m_CurrentButton == button){
      ++(pState->m_ButtonFrames);
        return pState->m_ButtonFrames >= 30 ? (30 - pState->m_ButtonFrames) % 5 == 0 : 0;
    } else {
      pState->m_CurrentButton = button;
      pState->m_ButtonFrames = 0;
      return 1;
    }
  case eRepeat:
    if (pState->m_CurrentButton == button) {
      ++(pState->m_ButtonFrames);
      return  pState->m_ButtonFrames % 5 == 0;
    } else {
      pState->m_CurrentButton = button;
      pState->m_ButtonFrames = 0;
      return 1;
    }
  }
  return 0;
}

static uint8_t
DoGameState(EButtons button, Tet_State *__restrict__ pState)
{
  if (pState->m_StateFrames > 0) {
    ++(pState->m_StateFrames);
    if (pState->m_StateFrames >= 61) {
      CleanUpLines(pState);
      pState->m_StateFrames = 0;
      NextPiece(pState);
      if (DoesPieceFit(pState,
                       pState->m_Piece.m_Rotation,
                       0, 0) < 0) {
        /*Game over!*/
        return -1;
      }
    }
    return 0;
  }

  switch(button) {
  case eNone:
    pState->m_CurrentButton = button;
    ++(pState->m_GravityFrames);
    if (pState->m_Gravity == pState->m_GravityFrames) {
      pState->m_GravityFrames = 0;

      if (DoesPieceFit(pState,
                       pState->m_Piece.m_Rotation,
                       0, 1)  == 0) {
        ++(pState->m_Piece.m_Y);
      } else {
        LockPieceInBoard(pState);
        uint8_t rowsCleared = CheckForLines(pState, pState->m_Piece.m_Y);
        if (rowsCleared > 0) {
          pState->m_Score += CalculateRowClearScore(pState, rowsCleared);
          pState->m_TotalRowsCleared +=rowsCleared;
          pState->m_StateFrames = 1;
          return 0;
        }

        NextPiece(pState);
        if (DoesPieceFit(pState,
                         pState->m_Piece.m_Rotation,
                         0, 0) < 0) {
          /*Game over!*/
          TransitionGameState(pState, eGameOver);
          return 0;
        }
      }
    }
    break;
  case eLeft:
    if (ShouldActOnButtonPress(pState, button, eRepeatOnDelay) &&
        DoesPieceFit(pState,
                     pState->m_Piece.m_Rotation,
                     -1, 0) == 0) {
      --(pState->m_Piece.m_X);
    }
    break;
  case eRight:
    if (ShouldActOnButtonPress(pState, button, eRepeatOnDelay) &&
        DoesPieceFit(pState,
                     pState->m_Piece.m_Rotation,
                     1, 0) == 0) {
      ++(pState->m_Piece.m_X);
    }
    break;
  case eDown:
    if (ShouldActOnButtonPress(pState, button, eRepeat) &&
        DoesPieceFit(pState,
                     pState->m_Piece.m_Rotation,
                     0, 1) == 0) {
      ++(pState->m_Piece.m_Y);
      if (pState->m_ButtonFrames > 0) {
        ++(pState->m_Score);
      }
    }
    break;
  case eA: //Rotate
    if (ShouldActOnButtonPress(pState, button, eOneShot)) {
      if (DoesPieceFit(pState,
                       (pState->m_Piece.m_Rotation + 1) % 4,
                       0, 0) == 0) {
        pState->m_Piece.m_Rotation = (pState->m_Piece.m_Rotation + 1) % 4;
      } else if (DoesPieceFit(pState,
                       (pState->m_Piece.m_Rotation + 1) % 4,
                       1, 0) == 0) {
        ++(pState->m_Piece.m_X);
        pState->m_Piece.m_Rotation = (pState->m_Piece.m_Rotation + 1) % 4;
      } else if (DoesPieceFit(pState,
                       (pState->m_Piece.m_Rotation + 1) % 4,
                       -1, 0) == 0) {
        --(pState->m_Piece.m_X);
        pState->m_Piece.m_Rotation = (pState->m_Piece.m_Rotation + 1) % 4;
      }
    }
    break;
  case eB:
    if (ShouldActOnButtonPress(pState, button, eOneShot)) {
      TransitionGameState(pState, ePaused);
    }
    break;
  default:
    pState->m_CurrentButton = button;
    break;
  }

  return 0;
}

static uint8_t
DoWaitForButtonPress(EButtons button, Tet_State *restrict pState, ETet_GameState nextState, uint8_t stateFrames)
{
  if (button != eNone) {
    if (ShouldActOnButtonPress(pState, button, eOneShot)) {
      TransitionGameState(pState, nextState);
    }
  } else {
    pState->m_CurrentButton = button;
    if (stateFrames > 0) {
      if (pState->m_StateFrames >= stateFrames) {
        TransitionGameState(pState, nextState);
      } else {
        ++(pState->m_StateFrames);
      }
    }
  }
  return 0;
}

static uint8_t
DoLevelSelect(EButtons button, Tet_State *restrict pState)
{
  switch (button) {
  case eUp:
    if (ShouldActOnButtonPress(pState, button, eRepeatOnDelay)) {
      pState->m_Level = MIN(pState->m_Level + 1, 29);
      pState->m_Gravity = 60 - (pState->m_Level * 2);
    }
    break;
  case eDown:
    if (ShouldActOnButtonPress(pState, button, eRepeatOnDelay)) {
      pState->m_Level = MIN(pState->m_Level - 1, 29);
      pState->m_Gravity = 60 - (pState->m_Level * 2);
    }
    break;
  case eA:
    TransitionGameState(pState, eGame);
    break;
  case eNone:
  case eB:
  case eLeft:
  case eRight:
    break;
  }
  return 0;
}

static void
TransitionGameState(Tet_State *restrict pState, ETet_GameState nextState)
{
  switch (pState->m_State)
  {
    case eInit:
      // Only goes to game
      pState->m_State = eLevelSelection;
      break;
    case eLevelSelection:
      // level select always goes to game
      pState->m_State = eGame;
      break;
    case eGame:
      // Can be pause or gameover
      // Treat other states as game over
      pState->m_State = (nextState == ePaused ? ePaused : eGameOver );
      break;
    case ePaused:
      // Treat all as Game
      pState->m_State = eGame;
      break;
    case eGameOver:
      // Treat all as Init but go straight to game
      init_tet(pState, pState->m_Level, pState->m_TopScore);
      pState->m_State = eGame;
      break;
  }
  pState->m_StateFrames = 0;
}

static void
FillBag(Tet_State *restrict pState)
{
  // If this works ill be surprised
  uint8_t *restrict bag = &(pState->m_Bag[0]);
  uint8_t i = 0;
  for (i = 0; i < BAG_COUNT; ++i) {
    bag[i] = i;
  }
}

static void
ShuffleArray(uint8_t *restrict array, uint8_t n)
{
  if (n > 1 ) {
    uint8_t i;
    for (i = 0; i < n - 1; ++i) {
      uint8_t j = i + rand() / (RAND_MAX / (n - i) + 1);
      uint8_t t = array[j];
      array[j] = array[i];
      array[i] = t;
    }
  }
}

uint8_t
DoGameTick(EButtons button, Tet_State *__restrict__ pState)
{
  switch (pState->m_State) {
  case eInit:
    return DoWaitForButtonPress(button, pState, eGame, 255);
  case eLevelSelection:
    return DoLevelSelect(button, pState);
  case eGame:
    return DoGameState(button, pState);
  case ePaused:
    return DoWaitForButtonPress(button, pState, eGame, 0);
  case eGameOver:
    return DoWaitForButtonPress(button, pState, eGame, 255);
  }
  return 0;
}
