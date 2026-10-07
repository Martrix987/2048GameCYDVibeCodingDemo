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
void drawGame() {
  uint16_t bg = game.highestTile() >= 1024 ? color(31,14,43) : game.highestTile() >= 128 ? color(42,26,31) : color(30,30,36);
  display.fillScreen(bg); display.setTextDatum(MC_DATUM); display.setTextColor(color(255,255,255), bg); display.setTextSize(2); display.drawString("2048 VIBE", 52, 20); display.setTextSize(1); display.drawString(("SCORE " + String(game.score())).c_str(), 170, 20); display.drawString(("BEST " + String(highScore)).c_str(), 170, 32);
  const char* labels[] = {"UNDO","BOMB","2x UP"}; const int uses[] = {game.undoUses(),game.bombUses(),game.multiplierUses()};
  for (int i=0;i<3;++i) { int x=8+i*78; display.fillRoundRect(x,55,70,22,4,bombMode&&i==1?color(180,40,40):color(70,70,90)); display.drawString((String(labels[i])+" "+String(uses[i])).c_str(),x+35,66); }
  for (uint8_t r=0;r<4;++r) for (uint8_t c=0;c<4;++c) { int x=GRID_X+c*(CELL+GAP), y=GRID_Y+r*(CELL+GAP); uint16_t v=game.board()[r][c]; uint16_t tile=tileColor(v); display.fillRoundRect(x,y,CELL,CELL,5,tile); if(v){display.setTextDatum(MC_DATUM);display.setTextSize(v>=128?2:3);display.setTextColor(TFT_WHITE,tile);display.drawNumber(v,x+CELL/2,y+CELL/2);display.setTextSize(1);} }
  display.fillRect(12,310,216,8,color(45,45,55)); display.fillRect(12,310,min<uint16_t>(216,game.score()%217),8,color(40,190,180));
  if(gameFinished){uint16_t shade=color(15,15,20);display.fillRoundRect(18,135,204,70,8,shade);display.setTextDatum(MC_DATUM);display.setTextColor(TFT_WHITE,shade);display.setTextSize(2);display.drawString(game.highestTile()>=2048?"YOU WIN!":"GAME OVER",120,158);display.setTextSize(1);display.drawString("Tap to restart",120,182);}
}
void tap(int16_t x,int16_t y) {
  if(gameFinished){game.reset(esp_random());gameFinished=false;bombMode=false;combo=1;lastMerge=0;drawGame();return;}
  if(y>=55&&y<=77){if(x<78&&game.undo()){bombMode=false;drawGame();}else if(x<156&&game.bombUses()){bombMode=true;drawGame();}else if(x>=156&&game.doubleHighest()){tone(BUZZER,700,80);drawGame();}return;}
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
