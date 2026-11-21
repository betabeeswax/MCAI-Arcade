#include <M5Cardputer.h>
#include <Arduino.h>
#include <vector>
#include <Preferences.h>

static constexpr int SW = 240, SH = 135;
static constexpr uint16_t BG = 0x0841, FG = 0xFFFF, ACC = 0x07FF, GOOD = 0x07E0, BAD = 0xF800, MUTED = 0x8410;
enum Mode { SNAKE, PONG, BREAKOUT, DODGE, REACTION, MATH, MEMORY, GUESS, TICTACTOE, TARGET };
enum Screen { MENU, GAME, RESULT };
struct Point { int x, y; };
struct Challenge { const char* name; Mode mode; int level; };

Challenge challenges[100];
Screen screen = MENU;
int selected = 0, score = 0, best[10] = {};
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

const char* modeName(Mode m) {
  switch(m) {
    case SNAKE:return "SNAKE"; case PONG:return "PONG"; case BREAKOUT:return "BREAKOUT";
    case DODGE:return "DODGE"; case REACTION:return "REACTION"; case MATH:return "MATH RUSH";
    case MEMORY:return "MEMORY"; case GUESS:return "NUMBER HUNT"; case TICTACTOE:return "TIC TAC TOE";
    default:return "TARGET TAP";
  }
}
void makeChallenges() {
  const char* names[10]={"Snake","Pong","Breakout","Dodge","Reaction","Math Rush","Memory","Number Hunt","Tic Tac Toe","Target Tap"};
  for(int i=0;i<100;i++) challenges[i] = {names[i/10],(Mode)(i/10),(i%10)+1};
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
  header("100 CHALLENGES");
  M5Cardputer.Display.setTextColor(ACC,BG);
  M5Cardputer.Display.drawString("CHOOSE A GAME",8,23);
  int start=(selected/7)*7;
  for(int i=0;i<7;i++) {
    int n=start+i; if(n>=100) break;
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
  }
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
    if(k=="w"||k=="W"||k=="UP")selected=(selected+99)%100;
    else if(k=="s"||k=="S"||k=="DOWN")selected=(selected+1)%100;
    else if(k=="a"||k=="A")selected=max(0,selected-7);
    else if(k=="d"||k=="D")selected=min(99,selected+7);
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
  } else if(mode==TARGET&&k=="ENTER") {
    int dx=random(-75,76),dy=random(-35,36);targetX=constrain(targetX+dx,20,220);targetY=constrain(targetY+dy,32,108);
    taps++;score+=10;if(taps>=10+level)finish(true);
  }
  drawGame();
}
void setup() {
  auto cfg=M5.config(); M5Cardputer.begin(cfg,true);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setBrightness(100);
  prefs.begin("mcai-arcade",false);
  for(int i=0;i<10;i++)best[i]=prefs.getInt(String("b"+String(i)).c_str(),0);
  makeChallenges();menuDraw();
}
void loop() {
  M5Cardputer.update();
  if(M5Cardputer.Keyboard.isChange()&&M5Cardputer.Keyboard.isPressed()) {
    auto keys=M5Cardputer.Keyboard.keys();
    for(auto key:keys) keyAction(String(key.word.c_str()));
  }
  if(screen==GAME) {
    if(mode==MEMORY&&showingSequence&&millis()-startedAt>1800+memoryLen*500){showingSequence=false;drawGame();}
    tickGame();
    static uint32_t paint=0;
    if(millis()-paint>80){paint=millis();if(mode==DODGE||mode==PONG||mode==BREAKOUT)drawGame();}
  } else if(screen==RESULT) { static uint32_t r=0;if(millis()-r>500){r=millis();drawResult();}}
  delay(8);
}
