#include <cassert>
#include "game_logic.h"
int main(){game2048::Game game;game.reset(42);int occupied=0;for(int r=0;r<4;++r)for(int c=0;c<4;++c)occupied+=game.board()[r][c]!=0;assert(occupied==2);assert(game.undoUses()==3);auto result=game.move(game2048::Direction::Left);if(result.changed)assert(game.undo());assert(game.bomb(0,0)==false||game.bombUses()==1);if(game.highestTile()){auto before=game.highestTile();assert(game.doubleHighest());assert(game.highestTile()>=before*2);}return 0;}
