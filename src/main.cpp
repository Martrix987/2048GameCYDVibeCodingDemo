#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include "game_logic.h"

namespace {
TFT_eSPI display;
constexpr uint8_t TOUCH_MOSI = 32, TOUCH_MISO = 39, TOUCH_CLK = 25, TOUCH_CS_PIN = 33;
uint16_t readTouchSpi(uint8_t command) {
  uint16_t value = 0;
  for (int bit = 7; bit >= 0; --bit) {
    digitalWrite(TOUCH_MOSI, command & (1 << bit));
    digitalWrite(TOUCH_CLK, LOW);
    delayMicroseconds(5);
    digitalWrite(TOUCH_CLK, HIGH);
    delayMicroseconds(5);
  }
  digitalWrite(TOUCH_MOSI, LOW);
  digitalWrite(TOUCH_CLK, LOW);
  for (int bit = 15; bit >= 0; --bit) {
    digitalWrite(TOUCH_CLK, HIGH);
    delayMicroseconds(5);
    digitalWrite(TOUCH_CLK, LOW);
    delayMicroseconds(5);
    value |= digitalRead(TOUCH_MISO) << bit;
  }
  return value >> 4;
}
bool touch(int16_t& x, int16_t& y) {
  constexpr uint8_t READ_X = 0x91, READ_Y = 0xD1, READ_Z1 = 0xB1, READ_Z2 = 0xC1;
  digitalWrite(TOUCH_CS_PIN, LOW);
  uint16_t z1 = readTouchSpi(READ_Z1);
  uint16_t z2 = readTouchSpi(READ_Z2);
  uint16_t pressure = z1 + 4095 - z2;
  if (pressure < 100) {
    digitalWrite(TOUCH_CS_PIN, HIGH);
    return false;
  }
  uint16_t rawX = readTouchSpi(READ_X);
  uint16_t rawY = readTouchSpi(READ_Y & ~1);
  digitalWrite(TOUCH_CS_PIN, HIGH);
  x = constrain(map(rawX, 200, 3800, 0, 240), 0, 239);
  y = constrain(map(rawY, 200, 3800, 0, 320), 0, 319);
  return true;
}
game2048::Game game; Preferences preferences;
constexpr int RED = 4, GREEN = 16, BLUE = 17, BUZZER = 26, GRID_X = 12, GRID_Y = 85, CELL = 46, GAP = 5;
constexpr uint32_t COMBO_WINDOW = 2000;
uint32_t lastMerge = 0, highScore = 0, touchStartMs = 0; uint8_t combo = 1; bool bombMode = false; int16_t startX = 0, startY = 0;
uint16_t color(uint8_t r, uint8_t g, uint8_t b) { return display.color565(r, g, b); }
void led(bool r, bool g, bool b) { digitalWrite(RED, r ? LOW : HIGH); digitalWrite(GREEN, g ? LOW : HIGH); digitalWrite(BLUE, b ? LOW : HIGH); }
uint16_t tileColor(uint16_t v) { if (!v) return color(55,55,70); if (v <= 64) return color(55,145,170); if (v <= 512) return color(210,120,50); return color(150,55,200); }
void drawGame() {
  uint16_t bg = game.highestTile() >= 1024 ? color(31,14,43) : game.highestTile() >= 128 ? color(42,26,31) : color(30,30,36);
  display.fillScreen(bg); display.setTextDatum(MC_DATUM); display.setTextColor(color(255,255,255), bg); display.setTextSize(2); display.drawString("2048 VIBE", 52, 20); display.setTextSize(1); display.drawString(("SCORE " + String(game.score())).c_str(), 170, 20); display.drawString(("BEST " + String(highScore)).c_str(), 170, 32);
  const char* labels[] = {"UNDO","BOMB","2x UP"}; const int uses[] = {game.undoUses(),game.bombUses(),game.multiplierUses()};
  for (int i=0;i<3;++i) { int x=8+i*78; display.fillRoundRect(x,55,70,22,4,bombMode&&i==1?color(180,40,40):color(70,70,90)); display.drawString((String(labels[i])+" "+String(uses[i])).c_str(),x+35,66); }
  for (uint8_t r=0;r<4;++r) for (uint8_t c=0;c<4;++c) { int x=GRID_X+c*(CELL+GAP), y=GRID_Y+r*(CELL+GAP); uint16_t v=game.board()[r][c]; display.fillRoundRect(x,y,CELL,CELL,5,tileColor(v)); if(v){display.setTextSize(v>=128?2:3); display.drawNumber(v,x+CELL/2,y+CELL/2);} }
  display.fillRect(12,310,216,8,color(45,45,55)); display.fillRect(12,310,min<uint16_t>(216,game.score()%217),8,color(40,190,180));
}
void tap(int16_t x,int16_t y) {
  if(!game.canMove()){game.reset(esp_random());bombMode=false;combo=1;lastMerge=0;drawGame();return;}
  if(y>=55&&y<=77){if(x<78&&game.undo()){bombMode=false;drawGame();}else if(x<156&&game.bombUses()){bombMode=true;drawGame();}else if(x>=156&&game.doubleHighest()){tone(BUZZER,700,80);drawGame();}return;}
  if(!bombMode||x<GRID_X||x>=GRID_X+4*(CELL+GAP)-GAP||y<GRID_Y||y>=GRID_Y+4*(CELL+GAP)-GAP)return;
  if(game.bomb((y-GRID_Y)/(CELL+GAP),(x-GRID_X)/(CELL+GAP))){bombMode=false;tone(BUZZER,220,90);drawGame();}
}
void swipe(int16_t x,int16_t y) {
  int16_t dx=x-startX,dy=y-startY;if(max(abs(dx),abs(dy))<28)return;
  auto direction=abs(dx)>abs(dy)?(dx>0?game2048::Direction::Right:game2048::Direction::Left):(dy>0?game2048::Direction::Down:game2048::Direction::Up);
  auto result=game.move(direction);if(!result.changed)return;
  if(result.mergeCount){uint32_t now=millis();combo=(now-lastMerge<=COMBO_WINDOW)?min<uint8_t>(4,combo+1):2;lastMerge=now;game.addComboBonus(combo,result.scoreDelta);tone(BUZZER,result.mergeValue>=1024?880:440,35);led(result.mergeValue>=1024,result.mergeValue>=128,true);}else{combo=1;tone(BUZZER,150,10);led(true,true,true);}
  if(game.score()>highScore){highScore=game.score();preferences.putUInt("high",highScore);} drawGame();
}
}
void setup(){Serial.begin(115200);pinMode(RED,OUTPUT);pinMode(GREEN,OUTPUT);pinMode(BLUE,OUTPUT);pinMode(BUZZER,OUTPUT);pinMode(TOUCH_MOSI,OUTPUT);pinMode(TOUCH_MISO,INPUT);pinMode(TOUCH_CLK,OUTPUT);pinMode(TOUCH_CS_PIN,OUTPUT);digitalWrite(TOUCH_CS_PIN,HIGH);digitalWrite(TOUCH_CLK,LOW);led(false,false,false);display.init();display.invertDisplay(true);display.setRotation(0);pinMode(TFT_BL,OUTPUT);digitalWrite(TFT_BL,TFT_BACKLIGHT_ON);preferences.begin("vibe2048",false);highScore=preferences.getUInt("high",0);game.reset(esp_random());drawGame();Serial.println("CYD 2048 Vibe Edition ready");}
void loop(){static uint32_t last=0;static bool active=false;static int16_t lastX=0,lastY=0;static bool moved=false;int16_t x=0,y=0;if(millis()-last>=40&&touch(x,y)){last=millis();lastX=x;lastY=y;if(!active){startX=x;startY=y;touchStartMs=millis();moved=false;active=true;}moved|=max(abs(x-startX),abs(y-startY))>=28;}else if(active){if(moved)swipe(lastX,lastY);else tap(startX,startY);active=false;}if(lastMerge&&millis()-lastMerge>COMBO_WINDOW)combo=1;if(!game.canMove())led(true,false,false);}
