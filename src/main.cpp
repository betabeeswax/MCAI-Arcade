#include <M5Cardputer.h>
#include <Arduino.h>
#include <vector>
#include <Preferences.h>

static constexpr int SW = 240, SH = 135;
static constexpr uint16_t BG = 0x0841, FG = 0xFFFF, ACC = 0x07FF, GOOD = 0x07E0, BAD = 0xF800, MUTED = 0x8410;
enum Mode { SNAKE, PONG, BREAKOUT, DODGE, REACTION, MATH, MEMORY, GUESS, TICTACTOE, TARGET, INVADERS, TETRIS, MINESWEEPER, FLAPPY, GAME2048, CHESS, CHECKERS, PACMAZE, RACING, WHACK };
enum Screen { MENU, GAME, RESULT };
struct Point { int x, y; };
struct Challenge { const char* name; Mode mode; int level; };

Challenge challenges[20];
Screen screen = MENU;
int selected = 0, score = 0, best[20] = {};
Preferences prefs;
int level = 1, page = 0;
uint32_t lastTick = 0, startedAt = 0, reactionAt = 0;
bool paused = false, won = false;
String message = "";
String subMessage = "";
Mode mode = SNAKE;

std::vector<Point> snake;
Point food{8,5};
int dirX = 1, dirY = 0;
float ballX=120, ballY=65, ballVX=2.2, ballVY=1.5;
float paddleX=50;
float brickPaddleX=103;
bool bricks[5][10];
int lives=3;
Point player{7,7}, target{10,5};
int reactionMs=0;
int mathA=0, mathB=0, mathAnswer=0;
int memorySeq[8], memoryLen=3, memoryPos=0, memoryInput[8];
bool showingSequence=false;
int secret=0, guessValue=5, tries=0;
char board[9];
int turn=0;
int targetX=120,targetY=65;
int gridX=0,gridY=0;
int taps=0;

int invX[4][7], invAlive[4][7], invDir=1, invShotX=-1, invShotY=-1, invPlayer=120;
int tetBoard[16][10], tetX=3,tetY=0,tetKind=0,tetRot=0;
const uint8_t tetShape[7][4][4][4]={
{{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},{{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},{{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},{{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}},
{{{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},{{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},{{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},{{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}},
{{{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},{{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},{{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},{{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}},
{{{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},{{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},{{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},{{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}}},
{{{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},{{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},{{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},{{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
{{{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},{{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},{{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},{{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}},
{{{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},{{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},{{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},{{0,1,0,0},{1,1,0,0},{1,0,0,0}}}};
bool mine[8][8], shown[8][8], flag[8][8]; int mx=0,my=0;
float bird=65,birdSpeed=0; int tile[4][4];
char chessBoard[8][8]; int bx=0,by=0,sx=-1,sy=-1; bool whiteMove=true;
char draughts[8][8]; int cx=0,cy=0,csx=-1,csy=-1; bool redMove=true;
int pacX=1,pacY=1,pacScore=0,carLane=1,carObs[3]={-1,-1,-1}, moleCol=3,moleRow=2,moleCount=0,moleAimX=3,moleAimY=2; bool pacDot[7][15];


const char* modeName(Mode m) {
  static const char* names[]={"SNAKE","PONG","BREAKOUT","DODGE","REACTION","MATH RUSH","MEMORY","NUMBER HUNT","TIC TAC TOE","TARGET TAP","SPACE INVADERS","TETRIS","MINESWEEPER","FLAPPY BIRD","2048","CHESS","CHECKERS","PAC-MAN MAZE","RACING","WHACK-A-MOLE"};
  return names[(int)m];
}
void makeChallenges() {
  for(int i=0;i<20;i++) challenges[i] = {modeName((Mode)i),(Mode)i,1};
}
void header(const char* title) {
  M5Cardputer.Display.fillScreen(BG);
  M5Cardputer.Display.setTextColor(FG,BG);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.drawString("MCAI ARCADE",6,4);
  M5Cardputer.Display.drawRightString(title,SW-6,4);
  M5Cardputer.Display.drawFastHLine(5,17,SW-10,ACC);
}
void footer(const char* s) {
  M5Cardputer.Display.drawFastHLine(5,SH-17,SW-10,MUTED);
  M5Cardputer.Display.setTextColor(MUTED,BG);
  M5Cardputer.Display.drawString(s,7,SH-13);
}
void menuDraw() {
  header("20 GAMES");
  M5Cardputer.Display.setTextColor(ACC,BG);
  M5Cardputer.Display.drawString("CHOOSE A GAME",8,23);
  int start=(selected/7)*7;
  for(int i=0;i<7;i++) {
    int n=start+i; if(n>=20) break;
    int y=39+i*12;
    if(n==selected) {
      M5Cardputer.Display.fillRoundRect(5,y-1,SW-10,11,2,0x2104);
      M5Cardputer.Display.setTextColor(ACC,0x2104);
    } else M5Cardputer.Display.setTextColor(FG,BG);
    char line[48]; snprintf(line,sizeof(line),"%02d  %-13s  LV %d",n+1,challenges[n].name,challenges[n].level);
    M5Cardputer.Display.drawString(line,9,y);
  }
  footer("W/S or arrows: move   ENTER: play   A/D: page");
}
void resetGame(int n) {
  selected=n; mode=challenges[n].mode; level=challenges[n].level;
  score=0; lives=3; paused=false; won=false; startedAt=millis(); lastTick=millis();
  randomSeed(esp_random());
  snake.clear(); snake.push_back({7,5}); snake.push_back({6,5}); snake.push_back({5,5});
  dirX=1;dirY=0;food={random(2,17),random(2,9)};
  ballX=120;ballY=65;ballVX=(random(0,2)?1:-1)*(1.7+level*.12);ballVY=1.4+level*.08;paddleX=50;brickPaddleX=103;
  for(int y=0;y<5;y++)for(int x=0;x<10;x++)bricks[y][x]=(y<2+level/3);
  player={7,7};target={random(1,18),random(1,10)};
  reactionMs=0;reactionAt=millis()+random(1200,3500);mathA=random(1,10+level*2);mathB=random(1,10+level*2);mathAnswer=mathA+mathB;
  memoryLen=min(3+level/2,8);memoryPos=0;showingSequence=true;
  for(int i=0;i<memoryLen;i++)memorySeq[i]=random(1,5);
  secret=random(1,51+level*5);guessValue=5;tries=0;
  for(int i=0;i<9;i++)board[i]=' ';turn=0;
  targetX=random(20,220);targetY=random(28,110);taps=0;

  for(int y=0;y<4;y++)for(int x=0;x<7;x++){invAlive[y][x]=1;invX[y][x]=30+x*25;}invDir=1;invShotX=invShotY=-1;invPlayer=120;
  memset(tetBoard,0,sizeof(tetBoard));tetX=3;tetY=0;tetKind=random(0,7);tetRot=0;
  memset(mine,0,sizeof(mine));memset(shown,0,sizeof(shown));memset(flag,0,sizeof(flag));mx=my=0;for(int n=0;n<10;){int x=random(0,8),y=random(0,8);if(!mine[y][x]){mine[y][x]=1;n++;}}
  bird=65;birdSpeed=0;memset(tile,0,sizeof(tile));tile[random(0,4)][random(0,4)]=2;tile[random(0,4)][random(0,4)]=2;
  const char* initChess[8]={"rnbqkbnr","pppppppp","........","........","........","........","PPPPPPPP","RNBQKBNR"};for(int y=0;y<8;y++)for(int x=0;x<8;x++)chessBoard[y][x]=initChess[y][x];bx=by=0;sx=sy=-1;whiteMove=true;
  memset(draughts,0,sizeof(draughts));for(int y=0;y<3;y++)for(int x=0;x<8;x++)if((x+y)%2)draughts[y][x]='r';for(int y=5;y<8;y++)for(int x=0;x<8;x++)if((x+y)%2)draughts[y][x]='b';cx=cy=0;csx=csy=-1;redMove=true;
  pacX=pacY=1;pacScore=0;memset(pacDot,1,sizeof(pacDot));for(int py=0;py<7;py++)for(int px=0;px<15;px++)if(!px||px==14||!py||py==6||((px%4==0)&&(py==2||py==4)))pacDot[py][px]=false;pacDot[1][1]=false;carLane=1;for(int i=0;i<3;i++)carObs[i]=-1;moleCol=random(0,8);moleRow=random(0,4);moleAimX=moleCol;moleAimY=moleRow;moleCount=0;

  screen=GAME;
}
void finish(bool win) {
  won=win; screen=RESULT;
  if(score>best[(int)mode]){best[(int)mode]=score;prefs.putInt(String("b"+String((int)mode)).c_str(),score);}
}
void drawSnake() {
  header("SNAKE");
  M5Cardputer.Display.setTextColor(FG,BG);
  M5Cardputer.Display.drawString("SCORE "+String(score)+"   BEST "+String(best[SNAKE]),8,22);
  for(auto p:snake) M5Cardputer.Display.fillRect(12+p.x*11,34+p.y*9,9,7,GOOD);
  M5Cardputer.Display.fillCircle(16+food.x*11,38+food.y*9,3,BAD);
  footer("WASD move   BACKSPACE pause");
}
void drawPong() {
  header("PONG");
  M5Cardputer.Display.drawString("SCORE "+String(score)+"   LIVES "+String(lives),8,22);
  M5Cardputer.Display.drawFastVLine(120,27,91,MUTED);
  M5Cardputer.Display.fillRect(8, (int)ballY-7,4,14,ACC);
  M5Cardputer.Display.fillRect(228,(int)paddleX,5,28,GOOD);
  M5Cardputer.Display.fillCircle((int)ballX,(int)ballY,3,FG);
  footer("W/S or arrows: move paddle");
}
void drawBreakout() {
  header("BREAKOUT");
  M5Cardputer.Display.drawString("SCORE "+String(score)+"   LIVES "+String(lives),8,22);
  for(int y=0;y<5;y++)for(int x=0;x<10;x++)if(bricks[y][x])M5Cardputer.Display.fillRect(5+x*23,30+y*9,20,6,(y%2)?ACC:GOOD);
  M5Cardputer.Display.fillRect((int)brickPaddleX,116,34,5,FG);
  M5Cardputer.Display.fillCircle((int)ballX,(int)ballY,3,BAD);
  footer("A/D or arrows: move");
}
void drawDodge() {
  header("DODGE");
  M5Cardputer.Display.drawString("SCORE "+String(score)+"   LEVEL "+String(level),8,22);
  M5Cardputer.Display.drawRect(5,29,230,91,MUTED);
  M5Cardputer.Display.fillRect(10+player.x*11,34+player.y*8,8,7,GOOD);
  for(int i=0;i<level+2;i++){int x=(i*37+(millis()/100*(i+1))%210);int y=32+(i*19)%80;M5Cardputer.Display.fillRect(x,y,5,5,BAD);}
  footer("WASD / arrows: dodge");
}
void drawGame() {
  switch(mode) {
    case SNAKE:drawSnake();break;
    case PONG:drawPong();break;
    case BREAKOUT:drawBreakout();break;
    case DODGE:drawDodge();break;
    case REACTION:
      header("REACTION TEST"); M5Cardputer.Display.setTextColor(FG,BG);
      M5Cardputer.Display.drawCenterString(reactionAt==0?"PRESS ENTER!":"WAIT FOR GREEN",120,55,2);
      if(reactionAt==0)M5Cardputer.Display.fillCircle(120,94,12,GOOD); else M5Cardputer.Display.fillCircle(120,94,12,BAD);
      footer("ENTER when green; early = fail");break;
    case MATH:
      header("MATH RUSH"); M5Cardputer.Display.drawCenterString(String(mathA)+" + "+String(mathB)+" = ?",120,45,2);
      M5Cardputer.Display.drawCenterString("Answer: "+String(guessValue),120,77,2);
      footer("1-9 type answer   ENTER submit");break;
    case MEMORY:
      header("MEMORY"); M5Cardputer.Display.drawCenterString(showingSequence?"MEMORIZE":"REPEAT THE SEQUENCE",120,34,1);
      if(showingSequence) {String s="";for(int i=0;i<memoryLen;i++)s+=String(memorySeq[i])+" ";M5Cardputer.Display.drawCenterString(s,120,67,2);}
      else M5Cardputer.Display.drawCenterString("STEP "+String(memoryPos+1)+" / "+String(memoryLen),120,67,2);
      footer("Type the numbers then ENTER");break;
    case GUESS:
      header("NUMBER HUNT");M5Cardputer.Display.drawCenterString("GUESS 1 - "+String(50+level*5),120,36,1);
      M5Cardputer.Display.drawCenterString(String(guessValue),120,58,3);
      M5Cardputer.Display.drawCenterString(message,120,94,1);footer("A/D adjust  ENTER guess");break;
    case TICTACTOE:
      header("TIC TAC TOE");M5Cardputer.Display.drawString("YOU: X   CPU: O",8,22);
      for(int y=0;y<3;y++)for(int x=0;x<3;x++){int i=y*3+x;M5Cardputer.Display.drawRect(69+x*34,35+y*27,30,23,MUTED);if(board[i]!=' ')M5Cardputer.Display.drawCenterString(String(board[i]),84+x*34,39+y*27,1);}
      M5Cardputer.Display.drawRect(68+(gridX%3)*34,34+(gridY%3)*27,32,25,ACC);
      footer("WASD select  ENTER place");break;

    case INVADERS:
      header("SPACE INVADERS");M5Cardputer.Display.drawString("SCORE "+String(score),8,22);
      for(int y=0;y<4;y++)for(int x=0;x<7;x++)if(invAlive[y][x])M5Cardputer.Display.fillRect(invX[y][x],32+y*13,13,8,GOOD);
      M5Cardputer.Display.fillTriangle(invPlayer,112,invPlayer-7,125,invPlayer+7,125,ACC);if(invShotY>=0)M5Cardputer.Display.fillRect(invShotX,invShotY,3,7,BAD);footer("A/D move  SPACE fire");break;
    case TETRIS:
      header("TETRIS");M5Cardputer.Display.drawString("SCORE "+String(score),8,22);
      for(int y=0;y<16;y++)for(int x=0;x<10;x++)if(tetBoard[y][x])M5Cardputer.Display.fillRect(25+x*9,27+y*6,8,5,ACC);
      for(int y=0;y<4;y++)for(int x=0;x<4;x++)if(tetShape[tetKind][tetRot][y][x]&&tetY+y<16)M5Cardputer.Display.fillRect(25+(tetX+x)*9,27+(tetY+y)*6,8,5,GOOD);footer("A/D move W rotate S drop");break;
    case MINESWEEPER:
      header("MINESWEEPER");for(int y=0;y<8;y++)for(int x=0;x<8;x++){int px=48+x*18,py=30+y*11;M5Cardputer.Display.drawRect(px,py,16,9,MUTED);if(shown[y][x]&&mine[y][x])M5Cardputer.Display.fillCircle(px+8,py+4,3,BAD);else if(flag[y][x])M5Cardputer.Display.drawCenterString("F",px+8,py,1);}
      M5Cardputer.Display.drawRect(47+mx*18,29+my*11,18,11,ACC);footer("WASD move ENTER reveal F flag");break;
    case FLAPPY:
      header("FLAPPY BIRD");M5Cardputer.Display.drawString("SCORE "+String(score),8,22);M5Cardputer.Display.fillCircle(65,(int)bird,5,GOOD);
      for(int i=0;i<2;i++){int x=150+i*100-(score*2%100),gap=52+i*23;M5Cardputer.Display.fillRect(x,27,14,gap-15,ACC);M5Cardputer.Display.fillRect(x,gap+25,14,SH-gap-42,ACC);}footer("SPACE / ENTER flap");break;
    case GAME2048:
      header("2048");M5Cardputer.Display.drawString("SCORE "+String(score),8,21);for(int y=0;y<4;y++)for(int x=0;x<4;x++){int px=57+x*32,py=31+y*22;M5Cardputer.Display.drawRoundRect(px,py,29,19,3,ACC);if(tile[y][x])M5Cardputer.Display.drawCenterString(String(tile[y][x]),px+14,py+5,1);}footer("WASD slide tiles");break;
    case CHESS:
      header("CHESS");M5Cardputer.Display.drawString(whiteMove?"WHITE TO MOVE":"BLACK TO MOVE",8,20);for(int y=0;y<8;y++)for(int x=0;x<8;x++){int px=48+x*12,py=28+y*12;M5Cardputer.Display.fillRect(px,py,12,12,((x+y)%2)?0x7BEF:0x39E7);if(chessBoard[y][x]!='.')M5Cardputer.Display.drawCenterString(String(chessBoard[y][x]),px+6,py+2,1);if(x==bx&&y==by)M5Cardputer.Display.drawRect(px,py,12,12,ACC);if(x==sx&&y==sy)M5Cardputer.Display.drawRect(px+2,py+2,8,8,GOOD);}footer("WASD select ENTER move");break;
    case CHECKERS:
      header("CHECKERS");for(int y=0;y<8;y++)for(int x=0;x<8;x++){int px=48+x*12,py=28+y*12;M5Cardputer.Display.fillRect(px,py,12,12,((x+y)%2)?0x7BEF:0x39E7);if(draughts[y][x])M5Cardputer.Display.fillCircle(px+6,py+6,4,draughts[y][x]=='r'?BAD:FG);if(x==cx&&y==cy)M5Cardputer.Display.drawRect(px,py,12,12,ACC);if(x==csx&&y==csy)M5Cardputer.Display.drawRect(px+2,py+2,8,8,GOOD);}footer("WASD select ENTER move");break;
    case PACMAZE:
      header("PAC-MAN MAZE");M5Cardputer.Display.drawString("DOTS "+String(pacScore),8,20);for(int y=0;y<7;y++)for(int x=0;x<15;x++){int px=20+x*13,py=30+y*12;bool wall=!x||x==14||!y||y==6||((x%4==0)&&(y==2||y==4));if(wall)M5Cardputer.Display.fillRect(px,py,12,11,ACC);else if(pacDot[y][x])M5Cardputer.Display.fillCircle(px+6,py+5,1,FG);}M5Cardputer.Display.fillCircle(26+pacX*13,35+pacY*12,5,GOOD);footer("WASD move");break;
    case RACING:
      header("RACING");M5Cardputer.Display.drawString("DISTANCE "+String(score),8,20);M5Cardputer.Display.drawFastVLine(80,28,92,MUTED);M5Cardputer.Display.drawFastVLine(160,28,92,MUTED);for(int i=0;i<3;i++)if(carObs[i]>=0)M5Cardputer.Display.fillRect(50+carObs[i]*45,35+i*27,20,14,BAD);M5Cardputer.Display.fillRoundRect(50+carLane*45,100,20,25,4,GOOD);footer("A/D change lanes");break;
    case WHACK:
      header("WHACK-A-MOLE");M5Cardputer.Display.drawString("HITS "+String(moleCount)+"/10",8,20);for(int y=0;y<4;y++)for(int x=0;x<8;x++)M5Cardputer.Display.drawCircle(25+x*27,34+y*20,6,MUTED);M5Cardputer.Display.fillCircle(25+moleAimX*27,34+moleAimY*20,5,GOOD);M5Cardputer.Display.drawRect(17+moleCol*27,26+moleRow*20,16,16,ACC);footer("WASD aim ENTER whack");break;
    case TARGET:
      header("TARGET TAP");M5Cardputer.Display.drawString("HITS "+String(taps)+"/"+String(10+level),8,22);
      M5Cardputer.Display.fillCircle(targetX,targetY,7,ACC);M5Cardputer.Display.drawCircle(targetX,targetY,10,FG);
      footer("Press ENTER when target appears");break;
  }
}
void tickGame() {
  uint32_t now=millis();
  if(now-lastTick< (mode==SNAKE?260-level*12:35))return;
  lastTick=now;
  if(mode==SNAKE) {
    Point h=snake.front();Point nh={h.x+dirX,h.y+dirY};
    if(nh.x<0||nh.x>=20||nh.y<0||nh.y>=10){finish(false);return;}
    for(auto p:snake)if(p.x==nh.x&&p.y==nh.y){finish(false);return;}
    snake.insert(snake.begin(),nh);
    if(nh.x==food.x&&nh.y==food.y){score+=10;food={random(0,20),random(0,10)};}else snake.pop_back();
  } else if(mode==PONG) {
    ballX+=ballVX;ballY+=ballVY;if(ballY<27||ballY>SH-19)ballVY=-ballVY;
    if(ballX<15){ballVX=abs(ballVX);score++;}
    if(ballX>222&&ballY>paddleX-4&&ballY<paddleX+32){ballVX=-abs(ballVX);ballVY+=random(-4,5)*.08;score+=5;}
    if(ballX>242){lives--;ballX=120;ballY=65;if(lives<=0)finish(false);}
  } else if(mode==BREAKOUT) {
    ballX+=ballVX;ballY+=ballVY;if(ballX<5||ballX>235)ballVX=-ballVX;if(ballY<25)ballVY=abs(ballVY);
    if(ballY>111&&ballX>brickPaddleX-3&&ballX<brickPaddleX+37){ballVY=-abs(ballVY);score+=2;}
    if(ballY>130){lives--;ballX=120;ballY=100;if(lives<=0)finish(false);}
    int bx=(int)ballX/23,by=((int)ballY-30)/9;if(bx>=0&&bx<10&&by>=0&&by<5&&bricks[by][bx]){bricks[by][bx]=false;ballVY=-ballVY;score+=10;}
    bool any=false;for(int y=0;y<5;y++)for(int x=0;x<10;x++)any|=bricks[y][x];if(!any)finish(true);
  } else if(mode==DODGE) {
    score++; if(score>=(60+level*12))finish(true);
    for(int i=0;i<level+2;i++){int x=(i*37+(millis()/100*(i+1))%210);int y=32+(i*19)%80;if(abs(x-(10+player.x*11))<7&&abs(y-(34+player.y*8))<7){finish(false);return;}}
  } else if(mode==REACTION) {
    if(reactionAt!=0 && now>=reactionAt)reactionAt=0;
    if(now-startedAt>12000&&reactionAt!=0)finish(false);
  } else if(mode==TARGET) {
    if(now-startedAt>25000)finish(false);

  } else if(mode==INVADERS) {
    static uint32_t invT=0;if(now-invT>350){invT=now;bool edge=false;for(int y=0;y<4;y++)for(int x=0;x<7;x++)if(invAlive[y][x]&&(invX[y][x]<8||invX[y][x]>218))edge=true;if(edge)invDir=-invDir;for(int y=0;y<4;y++)for(int x=0;x<7;x++)if(invAlive[y][x]){invX[y][x]+=invDir*4;if(invX[y][x]>invPlayer-7&&invX[y][x]<invPlayer+7){finish(false);return;}}}
    if(invShotY>=0){invShotY-=5;for(int y=0;y<4;y++)for(int x=0;x<7;x++)if(invAlive[y][x]&&invShotX>=invX[y][x]&&invShotX<=invX[y][x]+13&&invShotY>=32+y*13&&invShotY<=40+y*13){invAlive[y][x]=0;invShotY=-1;score+=10;}}
    bool any=false;for(int y=0;y<4;y++)for(int x=0;x<7;x++)any|=invAlive[y][x];if(!any)finish(true);
  } else if(mode==TETRIS) {
    auto hit=[&](int xx,int yy,int rr){for(int y=0;y<4;y++)for(int x=0;x<4;x++)if(tetShape[tetKind][rr][y][x]){int a=xx+x,b=yy+y;if(a<0||a>=10||b>=16||(b>=0&&tetBoard[b][a]))return true;}return false;};
    static uint32_t fall=0;if(now-fall>550-level*20){fall=now;if(!hit(tetX,tetY+1,tetRot))tetY++;else{for(int y=0;y<4;y++)for(int x=0;x<4;x++)if(tetShape[tetKind][tetRot][y][x]&&tetY+y>=0)tetBoard[tetY+y][tetX+x]=1;for(int y=15;y>=0;y--){bool full=true;for(int x=0;x<10;x++)full&=tetBoard[y][x];if(full){for(int yy=y;yy>0;yy--)for(int x=0;x<10;x++)tetBoard[yy][x]=tetBoard[yy-1][x];for(int x=0;x<10;x++)tetBoard[0][x]=0;score+=100;y++;}}tetX=3;tetY=0;tetKind=random(0,7);tetRot=0;if(hit(tetX,tetY,tetRot))finish(false);}}
  } else if(mode==FLAPPY) {birdSpeed+=0.22;bird+=birdSpeed;if(bird>120||bird<27)finish(false);for(int i=0;i<2;i++){int px=150+i*100-(score*2%100),gap=52+i*23;if(65+5>px&&65-5<px+14&&(bird-5<gap-15||bird+5>gap+25))finish(false);}static uint32_t flapScore=0;if(now-flapScore>700){flapScore=now;score++;}}
  else if(mode==RACING) {score++;for(int i=0;i<3;i++){if(carObs[i]<0)carObs[i]=random(0,3);else if(++carObs[i]>2)carObs[i]=-1;if(i==2&&carObs[i]==carLane)finish(false);}if(score>300)finish(true);}
  else if(mode==WHACK) {if(now-startedAt>30000)finish(false);else if(now-startedAt>750){moleCol=random(0,8);moleRow=random(0,4);moleAimX=moleCol;moleAimY=moleRow;startedAt=now;}}
}
void drawResult() {
  header("ROUND OVER");
  M5Cardputer.Display.setTextColor(won?GOOD:ACC,BG);
  M5Cardputer.Display.drawCenterString(won?"YOU WIN!":"GAME OVER",120,38,2);
  M5Cardputer.Display.setTextColor(FG,BG);
  M5Cardputer.Display.drawCenterString("SCORE  "+String(score),120,69,2);
  M5Cardputer.Display.drawCenterString("BEST   "+String(best[(int)mode]),120,91,1);
  footer("ENTER: replay   BACKSPACE: game list");
}
void keyAction(String k) {
  if(screen==MENU) {
    if(k=="w"||k=="W"||k=="UP")selected=(selected+19)%20;
    else if(k=="s"||k=="S"||k=="DOWN")selected=(selected+1)%20;
    else if(k=="a"||k=="A")selected=max(0,selected-7);
    else if(k=="d"||k=="D")selected=min(19,selected+7);
    else if(k=="ENTER"||k==" ")resetGame(selected);
    menuDraw();return;
  }
  if(screen==RESULT) {
    if(k=="ENTER"||k==" ")resetGame(selected);
    else if(k=="BACKSPACE"||k=="ESC")screen=MENU;
    if(screen==MENU)menuDraw();else drawResult();return;
  }
  if(k=="BACKSPACE"||k=="ESC") {screen=MENU;menuDraw();return;}
  if(k=="r"||k=="R"){resetGame(selected);return;}
  if(mode==SNAKE) {
    if((k=="w"||k=="W"||k=="UP")&&dirY==0){dirX=0;dirY=-1;}
    if((k=="s"||k=="S"||k=="DOWN")&&dirY==0){dirX=0;dirY=1;}
    if((k=="a"||k=="A"||k=="LEFT")&&dirX==0){dirX=-1;dirY=0;}
    if((k=="d"||k=="D"||k=="RIGHT")&&dirX==0){dirX=1;dirY=0;}
  } else if(mode==PONG||mode==BREAKOUT) {
    if(mode==PONG) {
      if(k=="w"||k=="W"||k=="UP")paddleX-=9;
      if(k=="s"||k=="S"||k=="DOWN")paddleX+=9;
      paddleX=constrain(paddleX,25,SH-45);
    } else {
      if(k=="a"||k=="A"||k=="LEFT")brickPaddleX-=10;
      if(k=="d"||k=="D"||k=="RIGHT")brickPaddleX+=10;
      brickPaddleX=constrain(brickPaddleX,3,SW-38);
    }
  } else if(mode==DODGE) {
    if(k=="w"||k=="W"||k=="UP")player.y=max(0,player.y-1);
    if(k=="s"||k=="S"||k=="DOWN")player.y=min(10,player.y+1);
    if(k=="a"||k=="A"||k=="LEFT")player.x=max(0,player.x-1);
    if(k=="d"||k=="D"||k=="RIGHT")player.x=min(19,player.x+1);
  } else if(mode==REACTION&&k=="ENTER") {
    if(reactionAt==0){reactionMs=millis()-startedAt;score=max(1,1000-reactionMs);finish(true);}
    else finish(false);
  } else if(mode==MATH) {
    if(k.length()==1&&isDigit(k[0]))guessValue=(guessValue*10+(k[0]-'0'))%10000;
    if(k=="BACKSPACE")guessValue/=10;
    if(k=="ENTER"){if(guessValue==mathAnswer){score+=10*level;finish(true);}else{message="Nope! "+String(mathAnswer);finish(false);}}
  } else if(mode==MEMORY&&!showingSequence) {
    if(k.length()==1&&isDigit(k[0])){memoryInput[memoryPos++]=k[0]-'0';if(memoryInput[memoryPos-1]!=memorySeq[memoryPos-1])finish(false);else if(memoryPos>=memoryLen){score=memoryLen*10;finish(true);}}
  } else if(mode==GUESS) {
    if(k=="a"||k=="A"||k=="LEFT")guessValue=max(1,guessValue-1);
    if(k=="d"||k=="D"||k=="RIGHT")guessValue=min(50+level*5,guessValue+1);
    if(k=="ENTER"){tries++;if(guessValue==secret){score=max(10,100-tries*5);finish(true);}else message=guessValue<secret?"TOO LOW":"TOO HIGH";}
  } else if(mode==TICTACTOE) {
    if(k=="w"||k=="W"||k=="UP")gridY=(gridY+2)%3;
    if(k=="s"||k=="S"||k=="DOWN")gridY=(gridY+1)%3;
    if(k=="a"||k=="A"||k=="LEFT")gridX=(gridX+2)%3;
    if(k=="d"||k=="D"||k=="RIGHT")gridX=(gridX+1)%3;
    if(k=="ENTER"){
      int i=gridY*3+gridX;
      if(board[i]==' '){
        board[i]='X';
        auto hasLine=[&](char p) {
          for(int a=0;a<3;a++) if(board[a*3]==p&&board[a*3+1]==p&&board[a*3+2]==p) return true;
          for(int a=0;a<3;a++) if(board[a]==p&&board[a+3]==p&&board[a+6]==p) return true;
          return (board[0]==p&&board[4]==p&&board[8]==p)||(board[2]==p&&board[4]==p&&board[6]==p);
        };
        if(hasLine('X')){score=100;finish(true);return;}
        int cpu=random(0,9),triesCpu=0;
        while(board[cpu]!=' '&&triesCpu++<9)cpu=(cpu+1)%9;
        if(triesCpu>=9){finish(false);return;}
        board[cpu]='O';
        if(hasLine('O')){finish(false);return;}
        bool empty=false;for(char c:board)empty|=c==' ';
        if(!empty)finish(false);
      }
    }

  } else if(mode==INVADERS) {
    if(k=="a"||k=="A"||k=="LEFT")invPlayer=max(10,invPlayer-10);if(k=="d"||k=="D"||k=="RIGHT")invPlayer=min(230,invPlayer+10);if((k==" "||k=="ENTER")&&invShotY<0){invShotX=invPlayer;invShotY=108;}
  } else if(mode==TETRIS) {
    auto hit=[&](int xx,int yy,int rr){for(int y=0;y<4;y++)for(int x=0;x<4;x++)if(tetShape[tetKind][rr][y][x]){int a=xx+x,b=yy+y;if(a<0||a>=10||b>=16||(b>=0&&tetBoard[b][a]))return true;}return false;};
    if((k=="a"||k=="A"||k=="LEFT")&&!hit(tetX-1,tetY,tetRot))tetX--;if((k=="d"||k=="D"||k=="RIGHT")&&!hit(tetX+1,tetY,tetRot))tetX++;if(k=="w"||k=="W"){int r=(tetRot+1)%4;if(!hit(tetX,tetY,r))tetRot=r;}if(k=="s"||k=="S")while(!hit(tetX,tetY+1,tetRot))tetY++;
  } else if(mode==MINESWEEPER) {
    if(k=="a"||k=="A"||k=="LEFT")mx=max(0,mx-1);if(k=="d"||k=="D"||k=="RIGHT")mx=min(7,mx+1);if(k=="w"||k=="W"||k=="UP")my=max(0,my-1);if(k=="s"||k=="S"||k=="DOWN")my=min(7,my+1);if(k=="f"||k=="F")flag[my][mx]=!flag[my][mx];if(k=="ENTER"&&!flag[my][mx]){shown[my][mx]=true;if(mine[my][mx])finish(false);else{bool clear=true;for(int y=0;y<8;y++)for(int x=0;x<8;x++)if(!mine[y][x]&&!shown[y][x])clear=false;if(clear){score=100;finish(true);}}}
  } else if(mode==FLAPPY&&(k==" "||k=="ENTER"))birdSpeed=-3.5;
  else if(mode==GAME2048) {
    int dx=0,dy=0;if(k=="a"||k=="A"||k=="LEFT")dx=-1;if(k=="d"||k=="D"||k=="RIGHT")dx=1;if(k=="w"||k=="W"||k=="UP")dy=-1;if(k=="s"||k=="S"||k=="DOWN")dy=1;
    if(dx||dy){for(int line=0;line<4;line++){int v[4],n=0;for(int j=0;j<4;j++){int x=dx<0?j:dx>0?3-j:line,y=dy<0?j:dy>0?3-j:line;if(tile[y][x])v[n++]=tile[y][x];}for(int j=0;j<n-1;j++)if(v[j]==v[j+1]){v[j]*=2;score+=v[j];for(int q=j+1;q<n-1;q++)v[q]=v[q+1];n--;}for(int j=0;j<4;j++){int x=dx<0?j:dx>0?3-j:line,y=dy<0?j:dy>0?3-j:line;tile[y][x]=j<n?v[j]:0;}}int x=random(0,4),y=random(0,4);for(int t=0;t<16&&tile[y][x];t++){x=random(0,4);y=random(0,4);}if(!tile[y][x])tile[y][x]=2;for(int y=0;y<4;y++)for(int x=0;x<4;x++)if(tile[y][x]>=2048)finish(true);}
  } else if(mode==CHESS) {
    if(k=="a"||k=="A"||k=="LEFT")bx=max(0,bx-1);if(k=="d"||k=="D"||k=="RIGHT")bx=min(7,bx+1);if(k=="w"||k=="W"||k=="UP")by=max(0,by-1);if(k=="s"||k=="S"||k=="DOWN")by=min(7,by+1);
    if(k=="ENTER"){char p=chessBoard[by][bx];if(sx<0){if(p!='.'&&((whiteMove&&isupper(p))||(!whiteMove&&islower(p)))){sx=bx;sy=by;}}else{char d=chessBoard[by][bx];if((d=='.'||(whiteMove&&islower(d))||(!whiteMove&&isupper(d)))&&(bx!=sx||by!=sy)){char q=tolower(chessBoard[sy][sx]);int dx=bx-sx,dy=by-sy;bool ok=true;if(q=='n')ok=(abs(dx)==1&&abs(dy)==2)||(abs(dx)==2&&abs(dy)==1);else if(q=='k')ok=abs(dx)<=1&&abs(dy)<=1;else if(q=='b')ok=abs(dx)==abs(dy);else if(q=='r')ok=dx==0||dy==0;else if(q=='q')ok=dx==0||dy==0||abs(dx)==abs(dy);else if(q=='p')ok=abs(dx)<=1&&(whiteMove?dy<0:dy>0);if(ok){if(tolower(d)=='k'){score=100;finish(true);return;}chessBoard[by][bx]=p;chessBoard[sy][sx]='.';whiteMove=!whiteMove;}}sx=sy=-1;}}
  } else if(mode==CHECKERS) {
    if(k=="a"||k=="A"||k=="LEFT")cx=max(0,cx-1);if(k=="d"||k=="D"||k=="RIGHT")cx=min(7,cx+1);if(k=="w"||k=="W"||k=="UP")cy=max(0,cy-1);if(k=="s"||k=="S"||k=="DOWN")cy=min(7,cy+1);
    if(k=="ENTER"){char p=draughts[cy][cx];if(csx<0){if(p==(redMove?'r':'b')){csx=cx;csy=cy;}}else{int dx=cx-csx,dy=cy-csy,fw=redMove?1:-1;if(abs(dx)==1&&dy==fw&&!draughts[cy][cx]){draughts[cy][cx]=draughts[csy][csx];draughts[csy][csx]=0;redMove=!redMove;}else if(abs(dx)==2&&dy==2*fw&&!draughts[cy][cx]&&draughts[csy+fw][csx+dx/2]&&draughts[csy+fw][csx+dx/2]!=(redMove?'r':'b')){draughts[cy][cx]=draughts[csy][csx];draughts[csy][csx]=0;draughts[csy+fw][csx+dx/2]=0;score+=10;redMove=!redMove;}csx=csy=-1;}}
  } else if(mode==PACMAZE) {
    int x=pacX,y=pacY;if(k=="a"||k=="A"||k=="LEFT")x--;if(k=="d"||k=="D"||k=="RIGHT")x++;if(k=="w"||k=="W"||k=="UP")y--;if(k=="s"||k=="S"||k=="DOWN")y++;bool wall=x<=0||x>=14||y<=0||y>=6||((x%4==0)&&(y==2||y==4));if(!wall){pacX=x;pacY=y;if(pacDot[y][x]){pacDot[y][x]=false;pacScore++;score+=5;}if(pacScore>=30)finish(true);}
  } else if(mode==RACING) {if(k=="a"||k=="A"||k=="LEFT")carLane=max(0,carLane-1);if(k=="d"||k=="D"||k=="RIGHT")carLane=min(2,carLane+1);}
  else if(mode==WHACK) {if(k=="a"||k=="A"||k=="LEFT")moleCol=max(0,moleCol-1);if(k=="d"||k=="D"||k=="RIGHT")moleCol=min(7,moleCol+1);if(k=="w"||k=="W"||k=="UP")moleRow=max(0,moleRow-1);if(k=="s"||k=="S"||k=="DOWN")moleRow=min(3,moleRow+1);if(k=="ENTER"){if(moleCol==moleAimX&&moleRow==moleAimY){moleCount++;score+=10;if(moleCount>=10)finish(true);}else{score=max(0,score-2);}moleCol=random(0,8);moleRow=random(0,4);moleAimX=moleCol;moleAimY=moleRow;}}
  else if(mode==TARGET&&k=="ENTER") {
    int dx=random(-75,76),dy=random(-35,36);targetX=constrain(targetX+dx,20,220);targetY=constrain(targetY+dy,32,108);
    taps++;score+=10;if(taps>=10+level)finish(true);
  }
  drawGame();
}
void setup() {
  auto cfg=M5.config(); M5Cardputer.begin(cfg,true);
  M5Cardputer.Display.setRotation(3);
  M5Cardputer.Display.setBrightness(100);
  prefs.begin("mcai-arcade",false);
  for(int i=0;i<20;i++)best[i]=prefs.getInt(String("b"+String(i)).c_str(),0);
  makeChallenges();menuDraw();
}
void loop() {
  M5Cardputer.update();
  if(M5Cardputer.Keyboard.isChange()&&M5Cardputer.Keyboard.isPressed()) {
    auto &keys=M5Cardputer.Keyboard.keysState();
    for(char key:keys.word) keyAction(String(key));
    if(keys.enter) keyAction("ENTER");
    if(keys.space) keyAction(" ");
  }
  if(screen==GAME) {
    if(mode==MEMORY&&showingSequence&&millis()-startedAt>1800+memoryLen*500){showingSequence=false;drawGame();}
    tickGame();
    static uint32_t paint=0;
    if(millis()-paint>80){paint=millis();if(mode==DODGE||mode==PONG||mode==BREAKOUT||mode==INVADERS||mode==TETRIS||mode==FLAPPY||mode==RACING||mode==WHACK)drawGame();}
  } else if(screen==RESULT) { static uint32_t r=0;if(millis()-r>500){r=millis();drawResult();}}
  delay(8);
}
