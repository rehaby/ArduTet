#ifdef __cplusplus
extern "C" {
#endif
  
#define PLAYAREA_WIDTH 12
#define PLAYAREA_HEIGHT 18
#define PLAYAREA_COUNT (PLAYAREA_WIDTH * PLAYAREA_HEIGHT)
#define BAG_COUNT 7

#define NO_UPDATE 0
#define PLAYAREA_UPDATED 1
#define PIECE_UPDATED 2
#define PLAYAREA_LINE 4

typedef enum {
  eNone,
  eLeft,
  eRight,
  eUp,
  eDown,
  eA,
  eB,
  eExit
} EButtons;

typedef enum {
  eInit,
  eLevelSelection,
  eGame,
  ePaused,
  eGameOver
} ETet_GameState;

typedef enum {
  e0,
  e90,
  e180,
  e270
} ERotation;

typedef struct {
  uint16_t m_Teromino;
  int16_t m_X;
  int16_t m_Y;
  ERotation m_Rotation;
} Piece;

typedef struct {
  Piece m_Piece;
  Piece m_NextPiece;
  uint8_t m_PlayArea[PLAYAREA_COUNT];
  uint8_t m_Bag[BAG_COUNT];
  uint8_t m_BagIndex;
  uint32_t m_Score;
  uint32_t m_TopScore;
  uint8_t m_Gravity;
  uint8_t m_GravityFrames;
  uint8_t m_Level;
  uint32_t m_TotalRowsCleared;
  EButtons m_CurrentButton;
  uint8_t m_ButtonFrames;
  ETet_GameState m_State;
  uint8_t m_StateFrames;
  uint8_t m_TopMostBlockInBoard;
} Tet_State;

uint8_t
GetTerominoPart(uint16_t Teromino, int16_t X, int16_t Y, ERotation Rotation);

void
init_tet(Tet_State *pState, uint8_t Level, uint32_t TopScore );

uint8_t 
DoGameTick(EButtons button, Tet_State * pState );

#ifdef __cplusplus
}
#endif
