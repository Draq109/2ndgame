#include <stdlib.h>
#include <string.h>

#include <stdlib.h>
#include <string.h>

// include NESLIB header
#include "neslib.h"

// include CC65 NES Header (PPU)
#include <nes.h>


#include "2ndGameBckg.h"

#define NES_MIRRORING 1

// link the pattern table into CHR ROM
//#link "chr_generic.s"

// BCD arithmetic support
#include "bcd.h"
//#link "bcd.c"

// VRAM update buffer
#include "vrambuf.h"
//#link "vrambuf.c"

//#link "famitone2.s"
void __fastcall__ famitone_update(void);
//#link "game2sound.s"
extern char game2sound_data[];

/*{pal:"nes",layout:"nes"}*/
const char BGPALETTE[16] = { 
  0x0F,			// screen color

  0x11,0x20,0x27,0x00,	// background palette 0
  0x01,0x20,0x2C,0x00,	// background palette 1
  0x00,0x10,0x20,0x00,	// background palette 2
  0x06,0x16,0x26,   	// background palette 3
};
/*const unsigned char player[]={ // Describes the MetaSprite of our player.
  // x-offset, y-offset, tile,   attributes
     0,        0,        0xD8,   0x03, // Tile 1 upper left
     0,        8,        0xD9,   0x03, // Tile 2 lower left
     8,        0,        0xDA,   0x00, // Tile 3 upper right
     8,        8,        0xDB,   0x02, // Tile 4 lower right
        128};*/

const unsigned char enemy[]={
     0,		0,	0xC4,	0x00,
     0,		8,	0xC5,	0x00,
     8,		0,	0xC6,	0x00,
     8,		8,	0xC7,	0x00,
  128
};
#define DEF_METASPRITE_2x2(name,code,pal)\
const unsigned char name[]={\
        0,      0,      (code)+0,   pal, \
        0,      8,      (code)+1,   pal, \
        8,      0,      (code)+2,   pal, \
        8,      8,      (code)+3,   pal, \
        128};

#define DEF_METASPRITE_2x2_FLIP(name,code,pal)\
const unsigned char name[]={\
        8,      0,      (code)+0,   (pal)|OAM_FLIP_H, \
        8,      8,      (code)+1,   (pal)|OAM_FLIP_H, \
        0,      0,      (code)+2,   (pal)|OAM_FLIP_H, \
        0,      8,      (code)+3,   (pal)|OAM_FLIP_H, \
        128};

DEF_METASPRITE_2x2(playerStand,0xD8,2);
DEF_METASPRITE_2x2(playerRRun1, 0xdc, 2);
DEF_METASPRITE_2x2(playerRRun2, 0xe0, 2);
DEF_METASPRITE_2x2(playerup, 0xec, 2);
DEF_METASPRITE_2x2(playerup2, 0xf0, 2);

DEF_METASPRITE_2x2(Enemyrun, 0xc4, 2);
DEF_METASPRITE_2x2(Enemyrun2, 0xc8, 2);


DEF_METASPRITE_2x2_FLIP(playerStandL,0xD8,2);
DEF_METASPRITE_2x2_FLIP(playerLRun1, 0xdc, 2);
DEF_METASPRITE_2x2_FLIP(playerLRun2, 0xe0, 2);
DEF_METASPRITE_2x2_FLIP(playerdown, 0xec, 2);
DEF_METASPRITE_2x2_FLIP(playerdown2, 0xf0, 2);

DEF_METASPRITE_2x2_FLIP(EnemyrunL, 0xc4, 2);
DEF_METASPRITE_2x2_FLIP(Enemyrun2L, 0xc8, 2);



#define TITLE_GAME_STATE 0
#define SPAWN_GAME_STATE 1
#define PLAY_GAME_STATE 2
#define NEW_LEVEL_STATE 3
#define GAME_OVER_STATE 4
#define RETRY_STATE 5
#define CH_COIN 0x18
#define CH_BLOCK 0xC2
#define CH_LADDER_L 0xd4
#define CH_LADDER_R 0xd5



static unsigned char controller, game_state=0,coins_collected = 0;
static unsigned char x,y,i,j=0;
static unsigned char xNpc[6],xCoins[5],xWall = 224;
static unsigned char yNpc[6],yCoins[5],yWall[2];
//bool gap = false;
static unsigned char flags[6] = {0,1,0,1,0,1},floorLevelY[4];
static unsigned char upL[2],upR[2],lowL[2],lowR[2];//four corners of our player, with index 0 being our x-axis and 1 y-axis
static byte runseq = 0;
static byte score = 0;
static char coins[18] = "Collected coins= ";
struct BlockGap{
unsigned char xStart,xEnd,yStart,yEnd;
};
struct BlockGap blockGaps[3];
//////////////////////////////
const unsigned char* const Running[16] = {
  playerStandL,playerLRun1,playerLRun2,playerStandL,
  playerStandL,playerLRun1,playerLRun2,playerStandL,
  playerStand,playerRRun1,playerRRun2,playerStand,
  playerStand,playerRRun1,playerRRun2,playerStand,
  
};

const unsigned char* const RunUpDown[16] = {

 playerup,playerup2,playerup2,playerup,
playerup,playerup2,playerup2,playerup,
  playerdown,playerdown2,playerdown2,playerdown,
playerdown,playerdown2,playerdown2,playerdown,

};


const unsigned char* const EnemyRunR[8] = {
	Enemyrun,Enemyrun2,Enemyrun2,Enemyrun,
	Enemyrun,Enemyrun2,Enemyrun2,Enemyrun,
};

const unsigned char* const EnemyRunL[8] = {
 	EnemyrunL,Enemyrun2L,EnemyrunL,Enemyrun2L,
 	 EnemyrunL,Enemyrun2L,EnemyrunL,Enemyrun2L
};


void playerMovement(char* i, char* x, char* y, byte* runseq, char *oam_id)
{

  bool pad = pad_poll(0);
  int stand = 0;
  
  upL[0] = *x;
  upL[1] = *y;
  upR[0] = *x + 16;
  upR[1] = *y;
  lowL[0] = *x;
  lowL[1] = *y + 16;
  lowR[0] = *x + 16;
  lowR[1] = *y + 16;
  // Move towards the Left
  if(*i&PAD_LEFT &&*x> 32) 
  {  
    if(pad)
    {
      pad = !pad;
      *runseq = *x & 7;
    *oam_id = oam_meta_spr(*x,*y,*oam_id,Running[*runseq]);
      stand = 2;
    }
    	if(*y <= 167 && *y >= 137)
      	{}
    	else if(*y <= 119 && *y >= 89)
    	{}
    	else if (*y <= 71 && *y >= 41)
    	{}
    	else
          *x-=1; 
   
  }
  // Move towards the Right
  if(*i&PAD_RIGHT&&*x<208) 
  {
    if(pad)
        {
          pad = !pad;
      *runseq = *x & 7;
  	*runseq +=8;
        *oam_id = oam_meta_spr(*x,*y,*oam_id,Running[*runseq]);
          stand = 3;
        }
      	if(*y <= 167 && *y >= 137)
      	{}
    	else if(*y <= 119 && *y >= 89)
    	{}
    	else if (*y <= 71 && *y >= 41)
    	{}
      else
    	*x+=1;
    
  }
  if(*i&PAD_RIGHT && coins_collected == 5 && *x >= 208 &&*x <= 232) 
  {
    if(*y == 184)
    	*x+=1;
    if(pad)
        {
          pad = !pad;
      *runseq = *x & 7;
  	*runseq +=8;
        *oam_id = oam_meta_spr(*x,*y,*oam_id,Running[*runseq]);
          stand = 3;
        }
  }
  // Move upwards
  if(*i&PAD_UP&&*y>24){ // because we are closing in on 0
       if(*y != 168 && *y != 120 && *y != 72)
         *y-=1;
  }
  if(*i&PAD_UP&&*y>24){ // because we are closing in on 0
       if((*x >= blockGaps[0].xStart - 4 && *x <= blockGaps[0].xStart + 4) && *y == 168)
         *y-=1;
       else if((*x >= blockGaps[1].xStart - 4 && *x <= blockGaps[1].xStart + 4) && *y == 120)
         *y-=1;
       else if((*x >= blockGaps[2].xStart - 4 && *x <= blockGaps[2].xStart + 4) && *y == 72)
         *y-=1;
    if(pad)
        {
          pad = !pad;
      	*runseq = *y & 7;
        *oam_id = oam_meta_spr(*x,*y,*oam_id,RunUpDown[*runseq]);
          stand = 1;
        }
  }
  
  // Move downwards
  if(*i&PAD_DOWN && *y < 184) // because we are closing in on 255
  {
    if(*y != 136 && *y != 88 && *y != 40)
    	*y+=1;
  }
  if(*i&PAD_DOWN && *y < 184)
  { 	
       if((*x >= blockGaps[0].xStart - 4 && *x <= blockGaps[0].xStart + 4) && *y == 136)
         *y+=1;
       else if((*x >= blockGaps[1].xStart - 4 && *x <= blockGaps[1].xStart + 4) && *y == 88)
         *y+=1;
       else if((*x >= blockGaps[2].xStart - 4 && *x <= blockGaps[2].xStart + 4) && *y == 40)
         *y+=1;
    if(pad)
        {
          pad = !pad;
      	*runseq = *y & 7;
          *runseq +=8;
        *oam_id = oam_meta_spr(*x,*y,*oam_id,RunUpDown[*runseq]);
      stand = 1;
        }
  }
  
  if(stand==0)
  {
    *runseq = *x & 7;
    *runseq +=8;
  *oam_id = oam_meta_spr(*x,*y,*oam_id,Running[*runseq]);
  }
  
};
void enemy_movement(char *oam_id)
{
  		// First Enemy
  		if(xNpc[0] == 24)
                  flags[0] = 1;
          	else if(xNpc[0] == 136)
                  flags[0] = 0;        
          	if(xNpc[0] > 24 && flags[0] == 0)
                {
                  xNpc[0] -= 1;
                 *oam_id = oam_meta_spr(xNpc[0],yNpc[0],*oam_id,EnemyRunL[xNpc[0]&7]);

                }
                 if(xNpc[0] < 136 && flags[0] == 1)
                 { 
                   xNpc[0] += 2;
                 *oam_id = oam_meta_spr(xNpc[0],yNpc[0],*oam_id,EnemyRunR[xNpc[0]&7]);
                 }
          	//Second Enemy
          	if(xNpc[1] == 92)
                  flags[1] = 1;
          	else if(xNpc[1] == 208)
                  flags[1] =0;
          	if(xNpc[1] > 92 && flags[1] == 0)
                {
                  xNpc[1] -= 1;
                 *oam_id = oam_meta_spr(xNpc[1],yNpc[1],*oam_id,EnemyRunL[xNpc[1]&7]);

                }
                 if(xNpc[1] < 208 && flags[1]== 1)
                 { 
                   xNpc[1] += 2;
                 *oam_id = oam_meta_spr(xNpc[1],yNpc[1],*oam_id,EnemyRunR[xNpc[1]&7]);
                 }
  		//Third Enemy
          	if(xNpc[2] == 92)
                  flags[2] =1;
          	else if(xNpc[2] == 208)
                  flags[2] =0;
          	if(xNpc[2] > 92 && flags[2] == 0)
                {
                  xNpc[2] -= 2;
                 *oam_id = oam_meta_spr(xNpc[2],yNpc[2],*oam_id,EnemyRunL[xNpc[2]&7]);

                }
                 if(xNpc[2] < 208 && flags[2] == 1)
                 { 
                   xNpc[2] += 2;
                   *oam_id = oam_meta_spr(xNpc[2],yNpc[2],*oam_id,EnemyRunR[xNpc[2]&7]);

                 }
  		//Fourth Enemy
  		if(xNpc[3] == 24)
                  flags[3] = 1;
          	else if(xNpc[3] == 136)
                  flags[3] = 0;        
          	if(xNpc[3] > 24 && flags[3] == 0)
                {
                  xNpc[3] -= 2;
                 *oam_id = oam_meta_spr(xNpc[3],yNpc[3],*oam_id,EnemyRunL[xNpc[3]&7]);

                }
                 if(xNpc[3] < 136 && flags[3] == 1)
                 {
                   xNpc[3] += 2;
                  *oam_id = oam_meta_spr(xNpc[3],yNpc[3],*oam_id,EnemyRunR[xNpc[3]&7]);

                 }
  
  		// Fifth Enemy
  		if(xNpc[4] == 24)
                  flags[4] = 1;
          	else if(xNpc[4] == 176)
                  flags[4] = 0;        
          	if(xNpc[4] > 24 && flags[4] == 0)
                {
                  xNpc[4] -= 2;
                  *oam_id = oam_meta_spr(xNpc[4],yNpc[4],*oam_id,EnemyRunL[xNpc[4]&7]);

                }
                 if(xNpc[4] < 176 && flags[4] == 1)
                 { 
                   xNpc[4] += 2;
                 *oam_id = oam_meta_spr(xNpc[4],yNpc[4],*oam_id,EnemyRunR[xNpc[4]&7]);

                 }
  		// Sixth Enemy
  		if(xNpc[5] == 56)
                  flags[5] = 1;
          	else if(xNpc[5] == 208)
                  flags[5] = 0;        
          	if(xNpc[5] > 56 && flags[5] == 0)
                {
                  xNpc[5] -= 2;
                  *oam_id = oam_meta_spr(xNpc[5],yNpc[5],*oam_id,EnemyRunL[xNpc[5]&7]);

                }
                 if(xNpc[5] < 208 && flags[5] == 1)
                 {
                   xNpc[5] += 2;
                *oam_id = oam_meta_spr(xNpc[5],yNpc[5],*oam_id,EnemyRunR[xNpc[5]&7]);

                 }
}


// setup PPU and tables
void setup_graphics() {
  // clear sprites
  oam_clear();
  // set palette colors
  pal_all(palette);
  //pal_bg(BGPALETTE);
}

void setMap(){
   // First Gap 
   floorLevelY[0] = 184;
   blockGaps[0].xStart = (rand() & 176) + 32;
   blockGaps[0].yStart = 160;
   blockGaps[0].xEnd = blockGaps[0].xStart + 16;
   blockGaps[0].yEnd = blockGaps[0].yStart - 16;
   floorLevelY[1] = blockGaps[0].yStart + 8;
  // Second Gap
  blockGaps[1].xStart = (rand() & 176) + 32;
   blockGaps[1].yStart = 112;
   blockGaps[1].xEnd = blockGaps[1].xStart + 16;
   blockGaps[1].yEnd = blockGaps[1].yStart - 16;
  // Third Gap
  blockGaps[2].xStart = (rand() & 176) + 32;
   blockGaps[2].yStart = 64;
   blockGaps[2].xEnd = blockGaps[2].xStart + 16;
   blockGaps[2].yEnd = blockGaps[2].yStart - 16;
  // Hallway walls
  yWall[0] = 183;
   yWall[1] = 191;
   xWall = 224;
  // Enemy Spawn Cordinates
  	xNpc[0] = 92;
  	yNpc[0] = 136;
  	xNpc[1] = 174;
  	yNpc[1] = 120;
  	xNpc[2] = 174;
  	yNpc[2] = 88;
  	xNpc[3] = 92;
  	yNpc[3] = 72;
  	xNpc[4] = 92;
  	yNpc[4] = 40;
  	xNpc[5] = 160;
  	yNpc[5] = 24;
}

void setCoins(){
j = 0;

   xCoins[j] = (rand() & 60) + 148;
   yCoins[j] = 140;
   j++;
  
   xCoins[j] = (rand() & 88) + 40;
   yCoins[j] = 124;
   j++;
  
   xCoins[j] = (rand() & 88) + 40;
   yCoins[j] = 88;
   j++;
  
   xCoins[j] = (rand() & 60) + 148;
   yCoins[j] = 74;
   j++;
  
   xCoins[j] = (rand() & 160) + 48;
   yCoins[j] = 32;
   
  return;

}
void makeMap(char *oam_id){
  // Makes our Gaps
  for(i = 0; i < 3; i++){
     *oam_id = oam_spr(blockGaps[i].xStart,blockGaps[i].yStart,CH_LADDER_L,3,*oam_id);
     *oam_id = oam_spr(blockGaps[i].xStart+8,blockGaps[i].yStart,CH_LADDER_R,3,*oam_id);
     *oam_id = oam_spr(blockGaps[i].xStart,blockGaps[i].yStart-8,CH_LADDER_L,3,*oam_id);
     *oam_id = oam_spr(blockGaps[i].xStart+8,blockGaps[i].yStart-8,CH_LADDER_R,3,*oam_id);
  }
  return;
}
void coinsCollision(char *x,char *y){
  if((*x-4 <= xCoins[0] && (*x + 8 >= xCoins[0] || *x + 16 >= xCoins[0])) && *y >= 128 && *y <= 140)
   {
	xCoins[0] = 255;
    	coins_collected++;
       score = bcd_add(score,1);
    
   }
  if((*x-4 <= xCoins[1] && (*x + 8 >= xCoins[1] || *x + 16 >= xCoins[1])) && *y >= 120 && *y <= 128)
   {
	xCoins[1] = 255;
    	coins_collected++;
       score = bcd_add(score,1);

    	
   }
  if((*x-4 <= xCoins[2] && (*x + 8 >= xCoins[2] || *x + 16 >= xCoins[2])) && *y >= 78 && *y <= 88)
   {
	xCoins[2] = 255;
    	coins_collected++;
       score = bcd_add(score,1);

    	
   }
  if((*x-4 <= xCoins[3] && (*x + 8 >= xCoins[3] || *x + 16 >= xCoins[3])) && *y >= 72 && *y <= 78)
   {
	xCoins[3] = 255;
    	coins_collected++;
       score = bcd_add(score,1);

    	
   }
  if((*x-4 <= xCoins[4] && (*x + 8 >= xCoins[4] || *x + 16 >= xCoins[4])) && *y >= 32 && *y <= 36)
   {
	xCoins[4] = 255;
    	coins_collected++;
      score = bcd_add(score,1);

    	
   }
}

////////////////////////////////////////////////////////////////////////
// Fade out/in Title screen

  void fade_out(){
byte x;
  for(x=4;x>=1;x--)
  {
	pal_bright(x);
   ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
   
  }
  pal_bright(x);

}

void fade_in(){

  byte x;
  for(x=0;x<=4;x++)
  {
  
  pal_bright(x);
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
    ppu_wait_frame();
   
  }

}

void TitleIn(){
 vram_adr(NTADR_A(0,0));
  pal_bg(palTitle);
   vram_unrle(TitleScreen);
  ppu_on_all();
  fade_in();
  
 while(!pad_trigger(0))
 {
 }
  fade_out();
  ppu_off();
}


void scrolling() {
  int x = 0;   // x scroll position
  int y = 0;   // y scroll position
  int dx = 1;  // y scroll direction
  // infinite loop
  while (1) {
    // wait for next frame
   // ppu_wait_frame();
    // update y variable
    split(x,0);
    x += dx;
    // change direction when hitting either edge of scroll area
    if (x >= 256) return;

    if (x == 0) dx = 16;
    // set scroll register
    //scroll(x, y);
  }
}

void enemyCollision()
{
   if((x-11 <= xNpc[0] && x + 4 >= xNpc[0]) && y >= 126 && y <= 140)
   {
	game_state = GAME_OVER_STATE;
   }
  if((x-11 <= xNpc[1] && x + 4 >= xNpc[1]) && y >= 112 && y <= 130)
   {
	game_state = GAME_OVER_STATE;
   }
  if((x-11 <= xNpc[2] && x + 4 >= xNpc[2]) && y >= 79 && y <= 94)
   {
	game_state = GAME_OVER_STATE;
   }
  if((x-11 <= xNpc[3] && x + 4 >= xNpc[3]) && y >= 65  && y <= 82)
   {
	game_state = GAME_OVER_STATE;
   }
  if((x-11 <= xNpc[4] && x + 4 >= xNpc[4]) && y >= 33 && y <= 45)
   {
	game_state = GAME_OVER_STATE;
   }
  if((x-11 <= xNpc[5] && x + 4 >= xNpc[5]) && y >= 24 && y <= 32)
   {
	game_state = GAME_OVER_STATE;
   }
 
}

void main(void)
{
 
  	char oam_id;
	pal_col(1,0x21);//blue color for text
	pal_col(17,0x30);//white color for sprite
  	
  	//vram_adr(NTADR_A(0,0));
  	//vram_unrle(n2ndGameBckg);
  	
  	setup_graphics();

	//ppu_on_all();//enable rendering
  	//fade_in();

	x=204;
	y=32;
  
  	
  	//setup sound
  	famitone_init(game2sound_data);
  	nmi_set_callback(famitone_update);
  
  	//put sprite

	while(1)
	{
        oam_id = 0;  	
	controller=pad_poll(0);
          if(game_state == TITLE_GAME_STATE)
          {
            TitleIn();
            vram_adr(NTADR_A(0,0));
  	    vram_unrle(n2ndGameBckg);
            vram_adr(NTADR_A(10,1));
           // count = '0';
            vram_write(coins,18);
           // vram_write(&count,1);
            
            
            vram_adr(NTADR_B(0,0));
            vram_unrle(n2ndGameBckg);
            ppu_on_all();
            fade_in();
            game_state = SPAWN_GAME_STATE;
            continue;
          }
          
          if(game_state == SPAWN_GAME_STATE)
          {
            	music_play(0);
            	coins_collected = 0;
            	x=32;
		y=184;
               	setCoins();
            	setMap(); // sets the gaps
            	pal_bg(palette);
            	ppu_on_all();
            	
            	game_state = PLAY_GAME_STATE;
            continue;
          }
          if(game_state == PLAY_GAME_STATE)
          {
            	// Character Sprites
                //oam_id = oam_meta_spr(x,y,oam_id,player);
          	//oam_id = oam_meta_spr(xNpc[0],yNpc[0],oam_id,enemy);
          	//oam_id = oam_meta_spr(xNpc[1],yNpc[1],oam_id,enemy);
            	//oam_id = oam_meta_spr(xNpc[2],yNpc[2],oam_id,enemy);
		//oam_id = oam_meta_spr(xNpc[3],yNpc[3],oam_id,enemy);
            	//oam_id = oam_meta_spr(xNpc[4],yNpc[4],oam_id,enemy);
		//oam_id = oam_meta_spr(xNpc[5],yNpc[5],oam_id,enemy);
          	// Collectable sprites
          	i=0;
      		oam_id = oam_spr(xCoins[i],yCoins[i],CH_COIN,2,oam_id);
            	i++;
          	oam_id = oam_spr(xCoins[i],yCoins[i],CH_COIN,2,oam_id);
            	i++;
          	oam_id = oam_spr(xCoins[i],yCoins[i],CH_COIN,2,oam_id);
            	i++;
          	oam_id = oam_spr(xCoins[i],yCoins[i],CH_COIN,2,oam_id);
            	i++;
            	oam_id = oam_spr(xCoins[i],yCoins[i],CH_COIN,2,oam_id);
            
            
            	//walls
            	oam_id = oam_spr(xWall,yWall[0],CH_BLOCK,palette[9],oam_id);
            	oam_id = oam_spr(xWall,yWall[1],CH_BLOCK,palette[9],oam_id);
            	oam_id = oam_spr(24,183,CH_BLOCK,palette[9],oam_id);
                oam_id = oam_spr(24,191,CH_BLOCK,palette[9],oam_id);
            
            	// Conditions  
                // When five coins are collected wall is removed for next area
            	if (coins_collected == 5)
                {
                  yWall[0] = 1;
                  yWall[1] = 1;
                  xWall = 255;
                }
            	// If player is at the hall, move to next area
            	if(y == 184 && x == 232)
                  game_state = NEW_LEVEL_STATE;
            	// Movement
          	playerMovement(&controller,&x,&y,&runseq,&oam_id);
            	enemy_movement(&oam_id);
            	coinsCollision(&x,&y);
           	enemyCollision();
            	
          	//score
                oam_id = oam_spr(216,7,'0'+(score &  0xf),2,oam_id);
		oam_id = oam_spr(208,7,'0'+(score >>4),2,oam_id);
            
   
            	// Map Blocks
            	
            	makeMap(&oam_id);
          
            	
           
            
          }
          if(game_state == NEW_LEVEL_STATE)
          {
            coins_collected = 0;
            oam_clear();
            oam_spr(1,22,0xa0,0,0);
            oam_id = oam_spr(208,7,'0'+(score &  0xf),2,oam_id);
	oam_id = oam_spr(200,7,'0'+(score >>4),2,oam_id);
            scrolling();
            oam_clear();
            x = 32;
            y = 184;
            game_state = SPAWN_GAME_STATE;
          }
          
           if(game_state == GAME_OVER_STATE)
          {
            fade_out();
            ppu_off();
            vram_adr(NTADR_A(0,0));
            vram_fill(0x00,32*30);
            vram_adr(NTADR_A(11,10));
            vram_write("Game Over",9);
            vram_adr(NTADR_A(11,20));
            vram_write("Try again?",10);
            fade_in();
            oam_clear();
            ppu_on_bg();
            score =0;
            game_state = RETRY_STATE;
            
          }
           if(game_state == RETRY_STATE)
          {
            music_stop();
           if(controller&PAD_START)
           {
             game_state = TITLE_GAME_STATE;
             fade_out();
             ppu_off();
             x = 32;
             y = 184;
           }
          }
          
          /*//Player/enemy Sprites
          	oam_id = oam_meta_spr(x,y,oam_id,player);
          	oam_id = oam_meta_spr(xNpc[0],yNpc[0],oam_id,enemy);
          	oam_id = oam_meta_spr(xNpc[1],yNpc[1],oam_id,enemy);

          	// Collectable sprites
          	spawnCoins();
          	
      		oam_id = oam_spr(50,18,0x18,0,oam_id);
          	oam_id_collectables[0] = oam_id;
          	
          	oam_id = oam_spr(175,18,0x18,0,oam_id);
          	oam_id_collectables[1] = oam_id;
          	oam_id = oam_spr(172,155,0x18,0,oam_id);
          	oam_id_collectables[2] = oam_id;
          	oam_id = oam_spr(24,95,0x18,0,oam_id);
		oam_id_collectables[3] = oam_id;          
          	oam_id = oam_spr(220,65,0x18,0,oam_id);
          	oam_id_collectables[4] = oam_id;
		//Player Sprite
          */
          	
          
          
          	
          	
		
          	// To recognize movement
          	//playerMovement(&controller,&x,&y,&runseq,&oam_id);
          	
		
		
		// Collision detection
          ppu_wait_frame();
	}
}