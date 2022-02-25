
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>
#include <string.h>

// include NESLIB header
#include "neslib.h"

// include CC65 NES Header (PPU)
#include <nes.h>


#include "2ndGameBckg.h"

// link the pattern table into CHR ROM
//#link "chr_generic.s"

// BCD arithmetic support
#include "bcd.h"
//#link "bcd.c"

// VRAM update buffer
#include "vrambuf.h"
//#link "vrambuf.c"

/*{pal:"nes",layout:"nes"}*/
const char BGPALETTE[32] = { 
  0x0F,			// screen color

  0x11,0x30,0x27,0x00,	// background palette 0
  0x1C,0x20,0x2C,0x00,	// background palette 1
  0x00,0x10,0x20,0x00,	// background palette 2
  0x06,0x16,0x26,0x00,   // background palette 3
};
const unsigned char player[]={ // Describes the MetaSprite of our player.
  // x-offset, y-offset, tile,   attributes
     0,        0,        0xD8,   0x03, // Tile 1 upper left
     0,        8,        0xD9,   0x03, // Tile 2 lower left
     8,        0,        0xDA,   0x00, // Tile 3 upper right
     8,        8,        0xDB,   0x02, // Tile 4 lower right
        128};

const unsigned char enemy[]={
     0,		0,	0xC4,	0x00,
     0,		8,	0xC5,	0x00,
     8,		0,	0xC6,	0x00,
     8,		8,	0xC7,	0x00,
  128
};

typedef struct{
 int xStart; // at what x coordinate theres a structure;
 int xEnd; // at what x coordinate that structure ends;
 int xTop; // at what x coordinate theres a structure above you;
 int xTopEnd; // a what x coordinate that structure ends 
 int xBot; // at what x coordinate theres a structure below you
 int xBotEnd; // at what x cordinate that structure ends
 
} CollisionByRow;
/*const CollisionByRow collision[12]={
  {160,168,56,80,16,224},
  {160,168,56,80,16,224},
  {160,168,56,80,16,224},
  {160,168,56,80,16,224},
  {160,168,56,80,16,224},
  {160,168,56,80,16,224},
  {160,168,56,80,16,224},
  {160,168,56,80,16,224},
  {160,168,56,80,16,224},
  {160,168,56,80,16,224},
  {160,168,56,80,16,224},
  {160,168,56,80,16,224},
  };*/



const unsigned char mapCol[25][28]={
// 1 2 3 4 5 6 7 8 9 A B C D E F G H I J K L M N O P Q R S
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},//1
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},//2
  {0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},//3
  {0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},//4
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,1,1},//5
  {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},//6
  {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0},//7
  {0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0},//8
  {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0},//9
  {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},//10 = A
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},//11 = B
  {1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},//12 = C
  {1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0},//13 = D
  {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0},//14 = F
  {0,0,0,0,1,1,1,0,0,0,1,0,0,0,0,0,0,0,1,1,1,0,0,0,0,1,0,0},//15 = G
  {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0},//16 = H
  {1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1},//17 = I
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0},//18 = J
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0},//19 = K
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},//20 = L
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,1,0,0},//21 = M
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0},//22 = N
  {0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,1,0,0,1,1,1,0,0,0,0,1,0,0},//23 = O
  {0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0},//24 = P
  {0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0},//25 = Q
  

};

void playerMovement(char* i, char* x, char* y)
{
  int row,column;
  
  
  if(*i&PAD_LEFT &&*x> 16 && mapCol[row][column] == 0) 
    *x-=1;
  if(*i&PAD_RIGHT&&*x<224 && mapCol[row][column] == 0) 
    *x+=1;
  if(*i&PAD_UP   &&*y> 16 && mapCol[row][column] == 0) 
    *y-=1;
  if(*i&PAD_DOWN &&*y<200 && mapCol[row][column] == 0) 
    *y+=1;
};


// setup PPU and tables
void setup_graphics() {
  // clear sprites
  oam_clear();
  // set palette colors
  pal_all(palette);
}

static unsigned char controller;
static unsigned char x,y;
static unsigned int xNpc[2];
static unsigned int yNpc[2];
static unsigned int flag = 0,flag1 = 0;

void main(void)
{
  	char oam_id = 0;
	pal_col(1,0x21);//blue color for text
	pal_col(17,0x30);//white color for sprite
  	
  	vram_unrle(n2ndGameBckg);
  	setup_graphics();

	ppu_on_all();//enable rendering

	x=16;
	y=200;
  
  	xNpc[0] = 175;
  	yNpc[0] = 60;
  	xNpc[1] = 80;
  	yNpc[1] = 192;
  	//put sprite

	while(1)
	{
          	oam_id = 0;
		controller=pad_poll(0);
          	
          	if(xNpc[0] == 111)
                  flag =1;
          	else if(xNpc[0] == 175)
                  flag =0;
          
          	if(xNpc[0] > 111 && flag == 0)
                {
                  xNpc[0] -=1;
                }
                 if(xNpc[0] <175 && flag ==1)
                  xNpc[0] +=1;
          
          
          if(xNpc[1] == 80)
                  flag1 =1;
          	else if(xNpc[1] == 136)
                  flag1 =0;
          
          	if(xNpc[1] > 80 && flag1 == 0)
                {
                  xNpc[1] -=1;
                }
                 if(xNpc[1] <136 && flag1 ==1)
                  xNpc[1] +=1;
          
          
          
          	// Collectable sprites
          	oam_id = oam_meta_spr(x,y,oam_id,player);
          	oam_id = oam_meta_spr(xNpc[0],yNpc[0],oam_id,enemy);
          	oam_id = oam_meta_spr(xNpc[1],yNpc[1],oam_id,enemy);

          	
      		oam_id = oam_spr(50,18,0x18,0,oam_id);
          	oam_id = oam_spr(175,18,0x18,0,oam_id);
          	oam_id = oam_spr(172,155,0x18,0,oam_id);
          	oam_id = oam_spr(172,155,0x18,0,oam_id);
          	oam_id = oam_spr(24,95,0x18,0,oam_id);
          	oam_id = oam_spr(220,65,0x18,0,oam_id);
		//Player Sprite
          
          	
          
          
          	
          	
		
          	// To recognize movement
          	playerMovement(&controller,&x,&y);
          	
		
		
		// Collision detection
          ppu_wait_frame();
	}
}
