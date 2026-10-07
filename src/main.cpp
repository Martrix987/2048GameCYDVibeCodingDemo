#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <XPT2046_Bitbang.h>
#include "game_logic.h"

namespace {
TFT_eSPI display;
constexpr uint8_t TOUCH_MOSI = 32, TOUCH_MISO = 39, TOUCH_CLK = 25, TOUCH_CS_PIN = 33;
XPT2046_Bitbang touchscreen(TOUCH_MOSI, TOUCH_MISO, TOUCH_CLK, TOUCH_CS_PIN, 240, 320);
bool touch(int16_t& x, int16_t& y, TouchPoint* details = nullptr) {
  TouchPoint point = touchscreen.getTouch();
  if (details) *details = point;
  if (!point.zRaw) return false;
  x = point.x;
  y = point.y;
  return true;
}
game2048::Game game; Preferences preferences;
constexpr int RED = 4, GREEN = 16, BLUE = 17, BUZZER = 26, GRID_X = 12, GRID_Y = 85, CELL = 46, GAP = 5;
constexpr uint32_t COMBO_WINDOW = 2000;
uint32_t lastMerge = 0, highScore = 0, touchStartMs = 0; uint8_t combo = 1; bool bombMode = false; int16_t startX = 0, startY = 0;
bool gameFinished = false;
uint16_t color(uint8_t r, uint8_t g, uint8_t b) { return display.color565(r, g, b); }
void led(bool r, bool g, bool b) { digitalWrite(RED, r ? LOW : HIGH); digitalWrite(GREEN, g ? LOW : HIGH); digitalWrite(BLUE, b ? LOW : HIGH); }
uint16_t tileColor(uint16_t v) { if (!v) return color(55,55,70); if (v <= 64) return color(55,145,170); if (v <= 512) return color(210,120,50); return color(150,55,200); }
const uint8_t glyphs[10][7] = {
  {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
  {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}, {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E},
  {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E},
  {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
  {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}
};
void drawDigit(uint8_t digit,int x,int y,uint16_t ink,uint16_t paper,uint8_t scale) {
  for(uint8_t row=0;row<7;++row) for(uint8_t col=0;col<5;++col)
    display.fillRect(x+col*scale,y+row*scale,scale,scale,(glyphs[digit][row]&(0x10>>col))?ink:paper);
}
void drawValue(uint16_t value,int centerX,int centerY,uint16_t ink,uint16_t paper) {
  char text[6]; snprintf(text,sizeof(text),"%u",value); uint8_t length=strlen(text), scale=3;
  int width=length*5*scale+(length-1)*scale, x=centerX-width/2, y=centerY-7*scale/2;
  for(uint8_t i=0;i<length;++i) drawDigit(text[i]-'0',x+i*(6*scale),y,ink,paper,scale);
}
void drawOverlayText(const char* text,int centerX,int y,uint16_t ink,uint16_t paper,uint8_t scale) {
  int width=strlen(text)*6*scale-scale, x=centerX-width/2;
  for(const char* p=text;*p;++p, x+=6*scale) {
    if(*p>='0'&&*p<='9') drawDigit(*p-'0',x,y,ink,paper,scale);
  }
}
void drawGame() {
  uint16_t bg = game.highestTile() >= 1024 ? color(31,14,43) : game.highestTile() >= 128 ? color(42,26,31) : color(30,30,36);
  display.fillScreen(bg); display.setTextDatum(MC_DATUM); display.setTextColor(TFT_WHITE,bg); display.setTextFont(2); display.setTextSize(1); display.drawString("2048 VIBE",52,18); display.drawString(("SCORE "+String(game.score())).c_str(),170,18); display.drawString(("BEST "+String(highScore)).c_str(),170,30);
  display.setTextFont(1); display.drawString("SWIPE TO MOVE",120,58);
  for (uint8_t r=0;r<4;++r) for (uint8_t c=0;c<4;++c) { int x=GRID_X+c*(CELL+GAP), y=GRID_Y+r*(CELL+GAP); uint16_t v=game.board()[r][c]; uint16_t tile=tileColor(v); display.fillRoundRect(x,y,CELL,CELL,5,tile); if(v) drawValue(v,x+CELL/2,y+CELL/2,TFT_WHITE,tile); }
  display.setTextDatum(TL_DATUM); display.setTextFont(1); display.setTextColor(TFT_WHITE,bg); display.drawString("COMBO",12,300); display.drawString(("X"+String(combo)).c_str(),205,300); display.fillRect(12,310,216,8,color(45,45,55)); display.fillRect(12,310,min<uint16_t>(216,game.score()%217),8,color(40,190,180));
  if(gameFinished){uint16_t shade=color(8,8,14);display.fillRect(0,0,240,320,shade);display.drawRect(8,72,224,176,TFT_WHITE);display.setTextDatum(MC_DATUM);display.setTextColor(TFT_WHITE,shade);display.setTextSize(3);display.drawString(game.highestTile()>=2048?"YOU WIN!":"GAME OVER",120,135);display.setTextSize(2);display.drawString("TAP TO RESTART",120,190);}
}
void tap(int16_t x,int16_t y) {
  if(gameFinished){game.reset(esp_random());gameFinished=false;bombMode=false;combo=1;lastMerge=0;drawGame();return;}
  if(!bombMode||x<GRID_X||x>=GRID_X+4*(CELL+GAP)-GAP||y<GRID_Y||y>=GRID_Y+4*(CELL+GAP)-GAP)return;
  if(game.bomb((y-GRID_Y)/(CELL+GAP),(x-GRID_X)/(CELL+GAP))){bombMode=false;tone(BUZZER,220,90);if(!game.canMove())gameFinished=true;drawGame();}
}
void swipe(int16_t x,int16_t y) {
  if(gameFinished)return;
  int16_t dx=x-startX,dy=y-startY;if(max(abs(dx),abs(dy))<28)return;
  auto touchDirection=abs(dx)>abs(dy)?(dx>0?game2048::Direction::Right:game2048::Direction::Left):(dy>0?game2048::Direction::Down:game2048::Direction::Up);
  auto direction=touchDirection;
  switch(touchDirection){
    case game2048::Direction::Right: direction=game2048::Direction::Up; break;
    case game2048::Direction::Left: direction=game2048::Direction::Down; break;
    case game2048::Direction::Down: direction=game2048::Direction::Right; break;
    case game2048::Direction::Up: direction=game2048::Direction::Left; break;
  }
  auto result=game.move(direction);if(!result.changed)return;
  if(result.mergeCount){uint32_t now=millis();combo=(now-lastMerge<=COMBO_WINDOW)?min<uint8_t>(4,combo+1):2;lastMerge=now;game.addComboBonus(combo,result.scoreDelta);tone(BUZZER,result.mergeValue>=1024?880:440,35);led(result.mergeValue>=1024,result.mergeValue>=128,true);}else{combo=1;tone(BUZZER,150,10);led(true,true,true);}
  if(game.highestTile()>=2048||!game.canMove())gameFinished=true;
  if(game.score()>highScore){highScore=game.score();preferences.putUInt("high",highScore);} drawGame();
}
}
void setup(){Serial.begin(115200);led(false,false,false);display.init();display.invertDisplay(true);display.setRotation(0);pinMode(TFT_BL,OUTPUT);digitalWrite(TFT_BL,TFT_BACKLIGHT_ON);touchscreen.begin();preferences.begin("vibe2048",false);highScore=preferences.getUInt("high",0);game.reset(esp_random());drawGame();Serial.println("CYD 2048 Vibe Edition ready");}
void loop(){static uint32_t last=0,lastTouchLog=0,lastIdleLog=0;static bool active=false;static int16_t lastX=0,lastY=0;static bool moved=false;int16_t x=0,y=0;TouchPoint point{};uint32_t now=millis();if(now-last>=40){last=now;bool pressed=touch(x,y,&point);if(pressed){if(!active){startX=x;startY=y;touchStartMs=now;moved=false;active=true;Serial.printf("[TOUCH DOWN] mapped=(%d,%d) raw=(%u,%u) pressure=%u\n",x,y,point.xRaw,point.yRaw,point.zRaw);}else if(now-lastTouchLog>=150){Serial.printf("[TOUCH MOVE] mapped=(%d,%d) raw=(%u,%u) pressure=%u\n",x,y,point.xRaw,point.yRaw,point.zRaw);lastTouchLog=now;}lastX=x;lastY=y;moved|=max(abs(x-startX),abs(y-startY))>=28;}else{if(active){Serial.printf("[TOUCH UP] mapped=(%d,%d) duration=%lums action=%s\n",lastX,lastY,(unsigned long)(now-touchStartMs),moved?"swipe":"tap");if(moved)swipe(lastX,lastY);else tap(startX,startY);active=false;}else if(now-lastIdleLog>=2000){Serial.println("[TOUCH IDLE] no touch detected");lastIdleLog=now;}}}if(lastMerge&&millis()-lastMerge>COMBO_WINDOW)combo=1;if(gameFinished)led(game.highestTile()<2048,false,false);}
