#include <stdio.h>
#include <stdlib.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_audio.h>



//the resolution can be changed
#define SCREEN_WIDTH 1024 
#define SCREEN_HEIGHT 640 
#define FPS 60.0
#define PLAYER_FPS 14.0

#define IMAGE_BLOCK_HEIGHT 32.0
#define IMAGE_BLOCK_WIDTH 32.0
#define SCALE_MULTIPLIER 1.4 
#define DEFAULT_FONT_SIZE 40.0

#define TEMPO_OF_MUSIC 1.2 //the tempo after the %90 of time is finished
#define ENVOIRMENT_CHANGE_MULTIPLIER 2 //If it increases the creatures and the gravity will slow down (time depends on PLAYER_FPS)
#define FINISH_TIME_AFTER_FAIL 3 //The game will wait 3 seconds after the miner is dead (in seconds)

#define LEVEL_QUANTITY 10 //max 99
#define START_MERITS 999  //max 999
#define WAIT_FOR_LEVEL_JUMP 1 //in seconds

typedef enum { wall, brickWall, miner, spider, monster, earth, rock, diamond, door, water, empty, explodeEmpty, explodeDiamond }blockType;

//for storing the place of the region in the image
typedef struct IMAGE_BLOCK_t {
	int x;       //WARNING !! These are not pixel based cordinates 
	int y;       //these are the cordinates of picture assuming every block is 1X1
	struct IMAGE_BLOCK_t * next;
}IMAGE_BLOCK;

typedef struct MAP_CELL_t {
	
	int type;
	int mapWidth;		//Width and Height will be stored in the first element of array
	int mapHeight; 		// example:  map[0][0].mapWidth 
	IMAGE_BLOCK * animation;  // For the explosion animations only
	int mark; 			//For some functions
	int spiderLastMove; //Stores the last move of spider 
	int pathMark;		//marks for path finding function 
	
					//Node properties(for the findShortestPath function)
	int nodeMark; 	//used to mark the nodes
	int nodeUpdated; // for avoiding updating more than once
	int nodeNumber;
}MAP_CELL;

//For storing the levels
typedef struct LEVEL_t {
	int number;
	int levelTime;		
	int waterRiseTime;
}LEVEL;

//Functions declared details are below for the functions
IMAGE_BLOCK * createListOfImagePos(int startX, int startY, int endX, int endY, int copies, int loopOrNot);
MAP_CELL ** createMapFromFile(const char * mapFileName, int * diamondObjective, int * openDoorX, int * openDoorY);
void drawMap(MAP_CELL ** map, IMAGE_BLOCK ** animationArr, ALLEGRO_BITMAP * bit);
void moveToNextAnimation(IMAGE_BLOCK ** anmArr, int count);
void findPositionOfMiner(MAP_CELL **map, int * x, int *y);
int moveCheck(MAP_CELL ** map, int dir, int * occupiedDiamonds);
void gravityUpdate(MAP_CELL** map);
void waterUpdate(MAP_CELL ** map);
int markExplosion(MAP_CELL ** map, int minerX, int minerY, IMAGE_BLOCK * anmHead, IMAGE_BLOCK * minerExp);
void anmExplosion(MAP_CELL ** map, ALLEGRO_BITMAP * sprites);
void creatureUpdate(MAP_CELL ** map);
void refreshUpdateStatus(MAP_CELL ** map);
int findShortestPathToMiner(MAP_CELL** map, int x, int y);
int updateGraphNodes(MAP_CELL ** map);
void refreshAllNodeMarks(MAP_CELL ** map, int mode);
void unmarkPath(MAP_CELL ** map);
void levelFlow(ALLEGRO_DISPLAY * display, const char * spritesFileName, LEVEL levels[], int startMerits, int levelQuantity);
int playLevel(ALLEGRO_DISPLAY * display, const char * levelName, const  char * spritesFileName, double waterRise,
	int levelTime, int livesRemaining, int levelNumber);
const char * indicatorCreator(char letter, int num);
const char * levelNameCreator(int num);


int main()
{
	if (!al_init()) {
		printf("Could not initialize allegro");
	}
	//Creating display
	ALLEGRO_DISPLAY * display = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);

	//Some controls and initializations
	if (!display) {
		printf("Failed to create display");
	}
	if (!al_init_font_addon()) {
		printf("Failed to initialize font addon");
	}
	if (!al_init_ttf_addon()) {
		printf("Failed to initialize tff addon");
	}
	if (!al_init_image_addon()) {
		printf("Failed to initialize image addon");
	}
	if (!al_install_audio()) {
		printf("Failed to initialize audio addon");
	}
	if (!al_init_acodec_addon()) {
		printf("Failed to initialize audio codec addon");
	}
	//Setting window position
	al_set_window_position(display, 0, 0);

	//Loading the level properties
	//If some level properties have to change it must be changed here and be compiled again
	//The properties : Level Time , Level Number , Water Rise Time ,The directory of sprites file
	const char * spritesFileName = "sprites.png";
	LEVEL levels[LEVEL_QUANTITY] = { { 1, 990 ,5 } , { 1, 990 ,5 } ,{ 1, 990 ,5 },{ 1, 990 ,5 },{ 1, 990 ,5 },{ 1, 990 ,5 },
	{ 1, 990 ,5 },{ 1, 990 ,5 },{ 1, 990 ,5 },{ 1, 990 ,5 }	};

	//Funtions that orginizes the level flow
	levelFlow(display, spritesFileName, levels, START_MERITS, LEVEL_QUANTITY);

	al_destroy_display(display);

	return 0;
}


//returns a looped list of image blocks cropped of the image and assumes the image size is 1 pixel
//pre : start points must be upper than end point also it must be on its left
// last parameter is the time any image will be saved to the list (this is for lowering the fps)
IMAGE_BLOCK * createListOfImagePos(int startX, int startY, int endX, int endY, int copies, int loopOrNot) {

	//Some adress allocations and variable declarations for the loop
	IMAGE_BLOCK * head = (IMAGE_BLOCK*)malloc(sizeof(IMAGE_BLOCK));
	IMAGE_BLOCK * iter = head;
	int curX = startX, curY = startY;
	head->next = NULL;
	iter->x = curX;
	iter->y = curY;
	curX++;

	int i;
	//This loop turns for every left upper part of a block on the picture
	for (; curY < endY; curY++) {
		for (; curX < endX; curX++) {
			//Writes the same position x times(x is variable copies here)
			for (i = 0; i<copies; i++) {
				iter->next = (IMAGE_BLOCK*)malloc(sizeof(IMAGE_BLOCK));
				iter = iter->next;
				iter->x = curX;
				iter->y = curY;
				iter->next = NULL;
			}
		}
		curX = startX;
	}
	//If its a loop connect the tail to the head
	if (loopOrNot)
		iter->next = head;
	//If not connect null
	else
		iter->next = NULL;
	//return the head of the list
	return head;
}

//Creates a map from a level file and reads its properties like level time and objective score 
//Returns the map array
MAP_CELL ** createMapFromFile(const char * mapFileName, int * diamondObjective, int * openDoorX, int * openDoorY) {

	FILE * fp = fopen(mapFileName, "r");
	int  i, j, mapWidth, mapHeight;
	
	//Read the properties
	fscanf(fp, "%d,%d[width,height]-%d[score objective]-%d,%d[open door index]", &mapWidth, &mapHeight,
		diamondObjective, openDoorX, openDoorY);
	
	MAP_CELL **map;
	//Allocate memory
	map = (MAP_CELL **)malloc(sizeof(MAP_CELL *) * (mapWidth));
	for (i = 0; i < mapWidth; i++) {
		map[i] = (MAP_CELL*)malloc(sizeof(MAP_CELL)*(mapHeight));
	}


	char c; //the value that will be read
	//Load the map width and height to the arrays first element
	map[0][0].mapWidth = mapWidth;
	map[0][0].mapHeight = mapHeight;
	
	//Preparing the i and j for x and y cordinates
	i = 0, j = 0;
	//Go until the file has end
	for (c = getc(fp); c != EOF; c = getc(fp)) {
		//If the value you read is connected with the elements in your map(example:miner,spider)
		//Assign their type to the current cordinates
		if (c == 'M') {
			map[i][j].type = miner;
		}
		else if (c == '.') {
			map[i][j].type = earth;
		}
		else if (c == '1') {
			map[i][j].type = wall;
		}
		else if (c == 'S') {
			map[i][j].type = spider;
		}
		else if (c == 'm') {
			map[i][j].type = monster;
		}
		else if (c == ' ') {
			map[i][j].type = empty;
		}
		else if (c == 'O') {
			map[i][j].type = rock;
		}
		else if (c == '#') {
			map[i][j].type = brickWall;
		}
		else if (c == 'W') {
			map[i][j].type = water;
		}
		else if (c == 'D') {
			map[i][j].type = door;
		}

		else if (c == '^') {
			map[i][j].type = diamond;
		}

		//When you read a comma 
		//Assign the default attributes for the current cordinates
		//After that increment the cordinates 
		if (c == ',') {
			map[i][j].animation = NULL;
			map[i][j].mark = 0;
			map[i][j].spiderLastMove = ALLEGRO_KEY_UP;
			map[i][j].pathMark = 0;
			map[i][j].nodeMark = 0;
			map[i][j].nodeUpdated = 0;
			map[i][j].nodeNumber = -1;
			i++;
			if (i == mapWidth) {
				i = 0;
				j++;

			}
		}

	}

	fclose(fp);

	return map;
}


//Draws the map 
void drawMap(MAP_CELL ** map, IMAGE_BLOCK ** animationArr, ALLEGRO_BITMAP * bit) {
	//for cordinates
	int i, j;

	//This attribute of map is stored in the first element
	int mapWidth = map[0][0].mapWidth;
	int mapHeight = map[0][0].mapHeight;

	//Traverse the map
	for (j = 0; j < mapHeight; j++) {
		for (i = 0; i < mapWidth; i++) {
			//Draw the type 
			if (map[i][j].type == wall) {
				al_draw_bitmap_region(bit, (animationArr[wall])->x*IMAGE_BLOCK_WIDTH, (animationArr[wall])->y*IMAGE_BLOCK_HEIGHT
					, IMAGE_BLOCK_WIDTH, IMAGE_BLOCK_HEIGHT,
					i*IMAGE_BLOCK_WIDTH, j*IMAGE_BLOCK_HEIGHT, 0);
			}

			else if (map[i][j].type == brickWall) {

				al_draw_bitmap_region(bit, (animationArr[brickWall])->x*IMAGE_BLOCK_WIDTH, (animationArr[brickWall])->y*IMAGE_BLOCK_HEIGHT
					, IMAGE_BLOCK_WIDTH, IMAGE_BLOCK_HEIGHT,
					i*IMAGE_BLOCK_WIDTH, j*IMAGE_BLOCK_HEIGHT, 0);
			}

			else if (map[i][j].type == spider) {

				al_draw_bitmap_region(bit, (animationArr[spider])->x*IMAGE_BLOCK_WIDTH, (animationArr[spider])->y*IMAGE_BLOCK_HEIGHT
					, IMAGE_BLOCK_WIDTH, IMAGE_BLOCK_HEIGHT,
					i*IMAGE_BLOCK_WIDTH, j*IMAGE_BLOCK_HEIGHT, 0);
			}
			else if (map[i][j].type == monster) {

				al_draw_bitmap_region(bit, (animationArr[monster])->x*IMAGE_BLOCK_WIDTH, (animationArr[monster])->y*IMAGE_BLOCK_HEIGHT
					, IMAGE_BLOCK_WIDTH, IMAGE_BLOCK_HEIGHT,
					i*IMAGE_BLOCK_WIDTH, j*IMAGE_BLOCK_HEIGHT, 0);
			}
			else if (map[i][j].type == earth) {

				al_draw_bitmap_region(bit, (animationArr[earth])->x*IMAGE_BLOCK_WIDTH, (animationArr[earth])->y*IMAGE_BLOCK_HEIGHT
					, IMAGE_BLOCK_WIDTH, IMAGE_BLOCK_HEIGHT,
					i*IMAGE_BLOCK_WIDTH, j*IMAGE_BLOCK_HEIGHT, 0);
			}
			else if (map[i][j].type == rock) {

				al_draw_bitmap_region(bit, (animationArr[rock])->x*IMAGE_BLOCK_WIDTH, (animationArr[rock])->y*IMAGE_BLOCK_HEIGHT
					, IMAGE_BLOCK_WIDTH, IMAGE_BLOCK_HEIGHT,
					i*IMAGE_BLOCK_WIDTH, j*IMAGE_BLOCK_HEIGHT, 0);
			}
			else if (map[i][j].type == diamond) {

				al_draw_bitmap_region(bit, (animationArr[diamond])->x*IMAGE_BLOCK_WIDTH, (animationArr[diamond])->y*IMAGE_BLOCK_HEIGHT
					, IMAGE_BLOCK_WIDTH, IMAGE_BLOCK_HEIGHT,
					i*IMAGE_BLOCK_WIDTH, j*IMAGE_BLOCK_HEIGHT, 0);
			}
			else if (map[i][j].type == door) {
				al_draw_bitmap_region(bit, (animationArr[door])->x*IMAGE_BLOCK_WIDTH, (animationArr[door])->y*IMAGE_BLOCK_HEIGHT
					, IMAGE_BLOCK_WIDTH, IMAGE_BLOCK_HEIGHT,
					i*IMAGE_BLOCK_WIDTH, j*IMAGE_BLOCK_HEIGHT, 0);
			}
			else if (map[i][j].type == water) {

				al_draw_bitmap_region(bit, (animationArr[water])->x*IMAGE_BLOCK_WIDTH, (animationArr[water])->y*IMAGE_BLOCK_HEIGHT
					, IMAGE_BLOCK_WIDTH, IMAGE_BLOCK_HEIGHT,
					i*IMAGE_BLOCK_WIDTH, j*IMAGE_BLOCK_HEIGHT, 0);
			}
		}

	}
}

//Given an position linked list array it will assign the next adress as their head 
//So there will be no need to move to next adress one by one
void moveToNextAnimation(IMAGE_BLOCK ** anmArr, int count) {
	int i;
	for (i = 0; i < count; i++) {
		if ((anmArr[i])->next)
			//Assign the next adress
			(anmArr[i]) = (anmArr[i]->next);
	}
}
//Returns the position of miner 
//If there is no miner returns 0,0
void findPositionOfMiner(MAP_CELL **map, int * x, int *y) {
	int mapWidth = map[0][0].mapWidth;
	int mapHeight = map[0][0].mapHeight;
	int i, j;
	*x = 0, *y = 0;
	for (j = 0; j < mapHeight; j++) {
		for (i = 0; i < mapWidth; i++) {
			//If you find the miner assign the cordinates
			if (map[i][j].type == miner) {
				*x = i;
				*y = j;
			}
		}
	}
	
		
}

//Checks if the miner can go to that position or not 
//Returns 1 if it can  and 0 if it cannot
int moveCheck(MAP_CELL ** map, int dir, int * occupiedDiamonds) {
	//Some variables initialized
	int minerX, minerY, result = 0, dirUnitX = 0, dirUnitY = 0;
	int mapWidth = map[0][0].mapWidth;
	int mapHeight = map[0][0].mapHeight;

	findPositionOfMiner(map, &minerX, &minerY);

	//Assign the dirUnitX values depending on the move direction
	if (dir == ALLEGRO_KEY_DOWN) dirUnitY = 1;
	else if (dir == ALLEGRO_KEY_UP) dirUnitY = -1;
	else if (dir == ALLEGRO_KEY_RIGHT) dirUnitX = 1;
	else if (dir == ALLEGRO_KEY_LEFT) dirUnitX = -1;
	//If it's in the map
	if (minerX > 0 && minerX < mapWidth && minerY < mapHeight && minerY > 0) {
		//If there is earth on the way
		if (map[minerX + dirUnitX][minerY + dirUnitY].type == earth) {

			map[minerX + dirUnitX][minerY + dirUnitY].type = miner;
			map[minerX][minerY].type = empty;
			result = 1;
		}
		//If there is diamond
		else if (map[minerX + dirUnitX][minerY + dirUnitY].type == diamond) {
			map[minerX + dirUnitX][minerY + dirUnitY].type = miner;
			map[minerX][minerY].type = empty;
			(*occupiedDiamonds)++;
			result = 1;
		}
		//Or there is a rock and its back is empty
		else if (dirUnitY == 0 && map[minerX + dirUnitX][minerY + dirUnitY].type == rock
			&& map[minerX + dirUnitX * 2][minerY + dirUnitY].type == empty) {
			map[minerX + dirUnitX][minerY + dirUnitY].type = miner;
			map[minerX][minerY].type = empty;
			map[minerX + dirUnitX * 2][minerY + dirUnitY].type = rock;
			result = 1;
		}
		//Or its just empty
		else if (map[minerX + dirUnitX][minerY + dirUnitY].type == empty) {
			map[minerX + dirUnitX][minerY + dirUnitY].type = miner;
			map[minerX][minerY].type = empty;
			result = 1;
		}

		//Else return 0
		else
			result = 0;
	}

	

	return result;
}

//Updates the enviorment if a rock needs to fall it will fall 
void gravityUpdate(MAP_CELL** map) {
	int i, j;
	int mapWidth = map[0][0].mapWidth;
	int mapHeight = map[0][0].mapHeight;
	for (j = mapHeight; j > 0; j--) {
		for (i = 0; i < mapWidth; i++) {
			if (map[i][j].type == rock && map[i][j + 1].type == empty) { //No need to check if j+1 exists
				map[i][j].type = empty;									//because it will exist since the design of  map will be that way
				map[i][j + 1].type = rock;
			}
		}
	}
}

//Every time it is called the water will expand
void waterUpdate(MAP_CELL ** map) {
	int x, y;
	int mapWidth = map[0][0].mapWidth;
	int mapHeight = map[0][0].mapHeight;

	for (y = 1; y < mapHeight - 1; y++) {
		for (x = 1; x < mapWidth - 1; x++) {
			//If there is a water and its not updated
			if (map[x][y].mark == 0 && map[x][y].type == water) {
				map[x][y].mark = 1;
				//Expand toward the appropiate cells
				if (map[x][y - 1].mark == 0 && (map[x][y - 1].type == empty || map[x][y - 1].type == earth)) {
					map[x][y - 1].type = water;
					map[x][y - 1].mark = 1;
				}
				if (map[x][y + 1].mark == 0 && (map[x][y + 1].type == empty || map[x][y + 1].type == earth)) {
					map[x][y + 1].type = water;
					map[x][y + 1].mark = 1;
				}
				if (map[x + 1][y].mark == 0 && (map[x + 1][y].type == empty || map[x + 1][y].type == earth)) {
					map[x + 1][y].type = water;
					map[x + 1][y].mark = 1;
				}
				if (map[x - 1][y].mark == 0 && (map[x - 1][y].type == empty || map[x - 1][y].type == earth)) {
					map[x - 1][y].type = water;
					map[x - 1][y].mark = 1;
				}
			}
		}
	}
	//Reset the update status
	refreshUpdateStatus(map);
}

//if there is the conditions for an explosion this function will make it happen and update the map
//with the enum exlode after the explosion it will be marked explode empty or explode diamond
int markExplosion(MAP_CELL ** map, int minerX, int minerY, IMAGE_BLOCK * anmHead, IMAGE_BLOCK * minerExp) {
	int mapWidth = map[0][0].mapWidth;
	int mapHeight = map[0][0].mapHeight;

	//if the creature dies
	int x, y;
	for (y = 0; y < mapHeight; y++) {
		for (x = 0; x < mapWidth; x++) {

			if ((map[x][y].type == spider || map[x][y].type == monster) && map[x][y - 1].type == rock) {
				//if its a spider

				if (map[x][y].type == spider) {
					map[x][y].type = explodeEmpty;
					//the 8 adjacent earth cell turning into diamond
					if (map[x][y - 1].type == earth) {
						map[x][y - 1].type = explodeDiamond;
						map[x][y - 1].animation = anmHead;
					}
					if (map[x][y + 1].type == earth) {
						map[x][y + 1].type = explodeDiamond;
						map[x][y + 1].animation = anmHead;
					}
					if (map[x + 1][y].type == earth) {
						map[x + 1][y].type = explodeDiamond;
						map[x + 1][y].animation = anmHead;
					}
					if (map[x - 1][y].type == earth) {
						map[x - 1][y].type = explodeDiamond;
						map[x - 1][y].animation = anmHead;
					}
					if (map[x + 1][y + 1].type == earth) {
						map[x + 1][y + 1].type = explodeDiamond;
						map[x + 1][y + 1].animation = anmHead;
					}
					if (map[x + 1][y - 1].type == earth) {
						map[x + 1][y - 1].type = explodeDiamond;
						map[x + 1][y - 1].animation = anmHead;
					}
					if (map[x - 1][y + 1].type == earth) {
						map[x - 1][y + 1].type = explodeDiamond;
						map[x - 1][y + 1].animation = anmHead;
					}
					if (map[x - 1][y - 1].type == earth) {
						map[x - 1][y - 1].type = explodeDiamond;
						map[x - 1][y - 1].animation = anmHead;
					}

				}
				//is its a monster
				else if (map[x][y].type == monster) {
					map[x][y].type = explodeEmpty;
					//its adjacent 12 cells including empty cells
					if (map[x][y - 1].type == earth || map[x][y - 1].type == empty || map[x][y - 1].type == rock) {
						map[x][y - 1].type = explodeDiamond;
						map[x][y - 1].animation = anmHead;
					}
					if (map[x][y + 1].type == earth || map[x][y + 1].type == empty || map[x][y + 1].type == rock) {
						map[x][y + 1].type = explodeDiamond;
						map[x][y + 1].animation = anmHead;
					}
					if (map[x + 1][y].type == earth || map[x + 1][y].type == empty || map[x + 1][y].type == rock) {
						map[x + 1][y].type = explodeDiamond;
						map[x + 1][y].animation = anmHead;
					}
					if (map[x - 1][y].type == earth || map[x - 1][y].type == empty || map[x - 1][y].type == rock) {
						map[x - 1][y].type = explodeDiamond;
						map[x - 1][y].animation = anmHead;
					}
					if (map[x + 1][y + 1].type == earth || map[x + 1][y + 1].type == empty || map[x + 1][y + 1].type == rock) {
						map[x + 1][y + 1].type = explodeDiamond;
						map[x + 1][y + 1].animation = anmHead;
					}
					if (map[x + 1][y - 1].type == earth || map[x + 1][y - 1].type == empty || map[x + 1][y - 1].type == rock) {
						map[x + 1][y - 1].type = explodeDiamond;
						map[x + 1][y - 1].animation = anmHead;
					}
					if (map[x - 1][y + 1].type == earth || map[x - 1][y + 1].type == empty || map[x - 1][y + 1].type == rock) {
						map[x - 1][y + 1].type = explodeDiamond;
						map[x - 1][y + 1].animation = anmHead;
					}
					if (map[x - 1][y - 1].type == earth || map[x - 1][y - 1].type == empty || map[x - 1][y - 1].type == rock) {
						map[x - 1][y - 1].type = explodeDiamond;
						map[x - 1][y - 1].animation = anmHead;
					}
					//must check if it's beyond limit
					if (x - 2 > 0 && (map[x - 2][y].type == earth || map[x - 2][y].type == empty || map[x - 2][y].type == rock)) {
						map[x - 2][y].type = explodeDiamond;
						map[x - 2][y].animation = anmHead;
					}
					if (y - 2 > 0 && (map[x][y - 2].type == earth || map[x][y - 2].type == empty || map[x][y - 2].type == rock)) {
						map[x][y - 2].type = explodeDiamond;
						map[x][y - 2].animation = anmHead;
					}
					if (y + 2 < mapHeight && (map[x][y + 2].type == earth || map[x][y + 2].type == empty || map[x][y + 2].type == rock)) {
						map[x][y + 2].type = explodeDiamond;
						map[x][y + 2].animation = anmHead;
					}
					if (x + 2 < mapWidth && (map[x + 2][y].type == earth || map[x + 2][y].type == empty || map[x + 2][y].type == rock)) {
						map[x + 2][y].type = explodeDiamond;
						map[x + 2][y].animation = anmHead;
					}
				}
			}
		}
	}




	//if the miner dies 
	//for the miner to die its adjacent cells must be occupied by spider or monster

	//WARNING UPDATE THIS CODE
	if (map[minerX + 1][minerY].type == spider || map[minerX + 1][minerY].type == monster) {
		map[minerX][minerY].type = explodeEmpty;
		map[minerX][minerY].animation = minerExp;
		return 0;
	}
	else if (map[minerX - 1][minerY].type == spider || map[minerX - 1][minerY].type == monster) {
		map[minerX][minerY].type = explodeEmpty;
		map[minerX][minerY].animation = minerExp;
		return 0;
	}
	else if (map[minerX][minerY - 1].type == spider || map[minerX][minerY - 1].type == monster) {
		map[minerX][minerY].type = explodeEmpty;
		map[minerX][minerY].animation = minerExp;
		return 0;
	}
	else if (map[minerX][minerY + 1].type == spider || map[minerX][minerY + 1].type == monster) {
		map[minerX][minerY].type = explodeEmpty;
		map[minerX][minerY].animation = minerExp;
		return 0;
	}
	return 1;
}

//animates the explotions in the marked areas if it is over then 
//the marks will be cleared by diamond or empty cells
void anmExplosion(MAP_CELL ** map, ALLEGRO_BITMAP * sprites) {

	int x, y;
	int mapWidth = map[0][0].mapWidth;
	int mapHeight = map[0][0].mapHeight;
	//Traverse the map
	for (y = 0; y < mapHeight; y++) {
		for (x = 0; x < mapWidth; x++) {
			//If there is an explosion type
			if (map[x][y].type == explodeDiamond || map[x][y].type == explodeEmpty) {
				//If the animation is not over
				if (map[x][y].animation != NULL) {
					//Draw the frame of it
					al_draw_bitmap_region(sprites, map[x][y].animation->x*IMAGE_BLOCK_WIDTH, map[x][y].animation->y*IMAGE_BLOCK_HEIGHT,
						IMAGE_BLOCK_WIDTH, IMAGE_BLOCK_HEIGHT, x*IMAGE_BLOCK_WIDTH, y*IMAGE_BLOCK_HEIGHT, 0);

					map[x][y].animation = map[x][y].animation->next;

				}
				//If its over then put diamond or empty cell after it
				else if (map[x][y].animation == NULL) {
					if (map[x][y].type == explodeDiamond) {
						map[x][y].type = diamond;
					}
					else if (map[x][y].type == explodeEmpty) {
						map[x][y].type = empty;
					}
				}
			}
		}
	}
}


//Updates the behavior of the creatures
void creatureUpdate(MAP_CELL ** map) {
	int x, y;
	int mapWidth = map[0][0].mapWidth;
	int mapHeight = map[0][0].mapHeight;


	for (y = 0; y < mapHeight; y++) {
		for (x = 0; x < mapWidth; x++) {
			//if it's a spider which moves always to the available space
			if (map[x][y].type == spider && map[x][y].mark == 0) {
				//if its right is empty
				if (map[x][y].spiderLastMove == ALLEGRO_KEY_UP) {
					if (map[x + 1][y].type == empty) {
						map[x + 1][y].type = spider;
						map[x][y].type = empty;
						map[x + 1][y].mark = 1;
						map[x + 1][y].spiderLastMove = ALLEGRO_KEY_RIGHT;
					}
					else 
						map[x][y].spiderLastMove = ALLEGRO_KEY_RIGHT;
				}
				else if (map[x][y].spiderLastMove == ALLEGRO_KEY_RIGHT) {
					if (map[x][y + 1].type == empty) {
						map[x][y + 1].type = spider;
						map[x][y].type = empty;
						map[x][y + 1].mark = 1;
						map[x][y + 1].spiderLastMove = ALLEGRO_KEY_DOWN;
					}
					else
						map[x][y].spiderLastMove = ALLEGRO_KEY_DOWN;
				}
				else if (map[x][y].spiderLastMove == ALLEGRO_KEY_DOWN) {
					if (map[x - 1][y].type == empty) {
						map[x - 1][y].type = spider;
						map[x][y].type = empty;
						map[x - 1][y].mark = 1;
						map[x - 1][y].spiderLastMove = ALLEGRO_KEY_LEFT;
					}
					else 
						map[x][y].spiderLastMove = ALLEGRO_KEY_LEFT;
				}
				else if (map[x][y].spiderLastMove == ALLEGRO_KEY_LEFT) {
					if (map[x][y - 1].type == empty) {
						map[x][y - 1].type = spider;
						map[x][y].type = empty;
						map[x][y - 1].mark = 1;
						map[x][y - 1].spiderLastMove = ALLEGRO_KEY_UP;
					}
					else
						map[x][y].spiderLastMove = ALLEGRO_KEY_UP;
				}
			}

			//if it's a monster which moves to the shortest path toward miner
			else if (map[x][y].type == monster && map[x][y].mark == 0) {
				//If there is a shortest path go towards it
				if (findShortestPathToMiner(map, x, y) == 1) {
					if (map[x + 1][y].pathMark == 1) {
						map[x + 1][y].type = monster;
						map[x + 1][y].mark = 1;
						map[x][y].type = empty;
					}
					else if (map[x - 1][y].pathMark == 1) {
						map[x - 1][y].type = monster;
						map[x - 1][y].mark = 1;
						map[x][y].type = empty;
					}
					else if (map[x][y - 1].pathMark == 1) {
						map[x][y - 1].type = monster;
						map[x][y - 1].mark = 1;
						map[x][y].type = empty;
					}
					else if (map[x][y + 1].pathMark == 1) {
						map[x][y + 1].type = monster;
						map[x][y + 1].mark = 1;
						map[x][y].type = empty;
					}
					unmarkPath(map);
				}

			}

		}
	}
	//resets the attributes
	refreshUpdateStatus(map);
}

//Resets the map attribute waterUpdate and creatureUpdate to 0
void refreshUpdateStatus(MAP_CELL ** map) {

	int x, y;
	int mapWidth = map[0][0].mapWidth;
	int mapHeight = map[0][0].mapHeight;

	for (y = 0; y < mapHeight; y++) {
		for (x = 0; x < mapWidth; x++) {
			//When you find it reset
			if (map[x][y].mark == 1) {
				map[x][y].mark = 0;
			}
		}
	}
}

//Finds shortest past with a recursive algorithm
//If there is no path returns 0
//If there is it returns 1 and the shortest path will have the value ALLEGRO_KEY_UP/DOWN/RIGHT/LEFT
int findShortestPathToMiner(MAP_CELL** map, int x, int y) {
	//first refresh the mark of nodes
	refreshAllNodeMarks(map, 1);
	refreshAllNodeMarks(map, 2);
	
	//Turn it to a graph
	int sourceNode,i = x, j = y;
	int mapWidth = map[0][0].mapWidth;
	int mapHeight = map[0][0].mapHeight;
	
	//First must find the location ofthe nodes and the count
	
	//Lets mark the first node manually
	map[x][y].nodeMark = 1;
	int updatedNodes = 1;
	int graphNodeCount = 0;
	//marks all the nodes from source and also returns the node count
	while (updatedNodes) {
		graphNodeCount += updatedNodes;
		updatedNodes = updateGraphNodes(map);
		
	};
	
	
	
	//return 0 if there is no path to miner
	int minerX, minerY;
	findPositionOfMiner(map, &minerX, &minerY);
	//If there is no node then there is no path to miner
	if ( (minerX == 0 &&  minerY == 0 )|| map[minerX][minerY].nodeMark != 1) {
		return 0;
	}
	

	//Create the adjacency matrix and initialize them to zero
	int** adjacency = (int **)calloc(graphNodeCount, sizeof(int*));
	for (i = 0; i < graphNodeCount; i++) {
		adjacency[i] = (int *)calloc(graphNodeCount, sizeof(int));
	}

	//Let's number the nodes and fill the adjancency matrix
	int graphNodeNumber = 0;
	for (j = 0; j < mapHeight; j++) {
		for (i = 0; i < mapWidth; i++) {
			if (map[i][j].nodeMark == 1) {
				//assign the number value
				
				map[i][j].nodeNumber = graphNodeNumber;
		
				graphNodeNumber++;
				
			}
		}
	}
	sourceNode = map[x][y].nodeNumber;
	
	//The adjancency matrix works like this:
	// if adjancency[i][j] is equal to 1 it means they are adjacent, 0 if they are not
	graphNodeNumber = 0;
	for (j = 0; j < mapHeight; j++) {
		for (i = 0; i < mapWidth; i++) {
			if (map[i][j].nodeMark == 1) {
				if ( map[i + 1][j].nodeMark == 1) {
					adjacency[graphNodeNumber][map[i + 1][j].nodeNumber] = 1;
				}
				if ( map[i - 1][j].nodeMark == 1) {
					adjacency[graphNodeNumber][map[i - 1][j].nodeNumber] = 1;
				}
				if ( map[i][j - 1].nodeMark == 1) {
					adjacency[graphNodeNumber][map[i][j - 1].nodeNumber] = 1;
				}
				if ( map[i][j + 1].nodeMark == 1) {
					adjacency[graphNodeNumber][map[i][j + 1].nodeNumber] = 1;
				}
				graphNodeNumber++;
			}
		}
	}
	
	//now the ajcacency matrix is filled
	//Let's apply Djikstra's algorithm

	//vertexCost will store the cost between to vertex
	int** vertexCost = (int **)calloc(graphNodeCount, sizeof(int*));
	for (i = 0; i < graphNodeCount; i++) {
		vertexCost[i] = (int *)calloc(graphNodeCount, sizeof(int));
	}
	//the distance to source
	int * vertexDistance = (int *)calloc(graphNodeCount, sizeof(int));
	//if visited equals 1
	int * isVisited = (int *)calloc(graphNodeCount, sizeof(int));
	//Stores the previous node that has the shortest path to it
	int * prevNode = (int *)calloc(graphNodeCount, sizeof(int));
	//Count : number of nodes seen so far
	int count, minDist, nextNode;
	//We will take the total cell number as infinity 
	int infinity = mapHeight * mapWidth;
	//We also have sourceNode and graphNodeCount from above
	
	//Note: Sometimes I write node or vertex they mean the same thing here

	//initializing them

	for (i = 0; i < graphNodeCount; i++) {
		for (j = 0; j < graphNodeCount; j++) {
			if (adjacency[i][j] == 0)
				vertexCost[i][j] = infinity;
			else
				vertexCost[i][j] = adjacency[i][j];
		}
	}

	for (i = 0; i < graphNodeCount; i++) {
		vertexDistance[i] = vertexCost[sourceNode][i];
		prevNode[i] = sourceNode;
		isVisited[i] = 0;
	}
	//The source node has distance 0
	vertexDistance[sourceNode] = 0;
	isVisited[sourceNode] = 1;
	count = 1;

	while (count < graphNodeCount-1) {
		minDist = infinity;
		
		for (i = 0; i < graphNodeCount; i++) {
			if ((vertexDistance[i] < minDist) && !isVisited[i]) {
				minDist = vertexDistance[i];
				nextNode = i;
			}
		}
		
		isVisited[nextNode] = 1;
		//If a better path exists through nextNode
		for (i = 0; i < graphNodeCount; i++) {
			if (!isVisited[i] && minDist + vertexCost[nextNode][i] < vertexDistance[i]) {
			
				vertexDistance[i] = minDist + vertexCost[nextNode][i];
				prevNode[i] = nextNode;
			}
		}
		count++;
	}


	//The target vertex number
	int targetNode = map[minerX][minerY].nodeNumber;
	int currentNode = targetNode;
	//Find the node next to the source
	while (sourceNode != prevNode[currentNode]) {
		
		currentNode = prevNode[currentNode];
	}
	
	//find the cell with this node number
	for (j = 0; j < mapHeight; j++) {
		for (i = 0; i < mapWidth; i++) {
			if (map[i][j].nodeNumber == currentNode) {
				map[i][j].pathMark = 1;
				
			}
		}
	}


	//refresh the nodes
	refreshAllNodeMarks(map, 1);
	
	return 1;
}

//Returns the number of marked new nodes in the graph
int updateGraphNodes(MAP_CELL ** map){
	//The variables for loop
	int i , j;
	int mapWidth = map[0][0].mapWidth;
	int mapHeight = map[0][0].mapHeight;
	
	//variables for the function 
	int newNodeCount = 0;


	for (j = 0; j < mapHeight; j++) {
		for (i = 0; i < mapWidth; i++) {
			//If the cell is not updated and a node
			if (map[i][j].nodeMark == 1 && map[i][j].nodeUpdated == 0) {
				//Mark the near and cells
				if (map[i+1][j].nodeMark == 0 && (map[i+1][j].type == empty || map[i + 1][j].type == miner) && map[i+1][j].nodeUpdated == 0) {
					map[i+1][j].nodeMark = 1;
					map[i+1][j].nodeUpdated = 1;
					newNodeCount++;
				}
				if (map[i][j+1].nodeMark == 0 && (map[i][j+1].type == empty || map[i][j + 1].type == miner) && map[i][j+1].nodeUpdated == 0) {
					map[i][j+1].nodeMark = 1;
					map[i][j+1].nodeUpdated = 1;
					newNodeCount++;
				}
				if (map[i-1][j].nodeMark == 0 && (map[i-1][j].type == empty || map[i - 1][j].type == miner) && map[i-1][j].nodeUpdated == 0) {
					map[i-1][j].nodeMark = 1;
					map[i-1][j].nodeUpdated = 1;
					newNodeCount++;
				}
				if (map[i][j-1].nodeMark == 0 && (map[i][j-1].type == empty || map[i][j - 1].type == miner) && map[i][j-1].nodeUpdated == 0) {
					map[i][j-1].nodeMark = 1;
					map[i][j-1].nodeUpdated = 1;
					newNodeCount++;
				}
			}
		}
	}
	//refresh the node marks
	refreshAllNodeMarks(map, 2);
	return newNodeCount;
}

//Refreshs the marks related to nodes
//If the second parameter is;
// 1 it will refresh the nodeMark and the nodeNumber,
// 2 it will refresh the nodeUpdated
void refreshAllNodeMarks(MAP_CELL ** map,int mode) {
	//loop variables
	int mapWidth = map[0][0].mapWidth, mapHeight = map[0][0].mapHeight , i ,j;
	//the loop
	for (j = 0; j < mapHeight; j++) {
		for (i = 0; i < mapWidth; i++) {
			if(map[i][j].nodeMark == 1 && mode == 1){
				map[i][j].nodeMark = 0;
				map[i][j].nodeNumber = -1;
			}
			if (map[i][j].nodeUpdated == 1 && mode == 2) {
				map[i][j].nodeUpdated = 0;
			}
		}
	}
}

//Unmarks the path mark attributes if they are 1
void unmarkPath(MAP_CELL ** map) {
	int x, y;
	int mapWidth = map[0][0].mapWidth;
	int mapHeight = map[0][0].mapHeight;

	for (y = 0; y < mapHeight; y++) {
		for (x = 0; x < mapWidth; x++) {
			if (map[x][y].pathMark == 1) {
				map[x][y].pathMark = 0;
			}
		}
	}
}


//Returns 1 if the level is sucessfuly finished
//Returns 0 if the level has to start again
//Returns -1 if the user wants to quit
//returns 2 if user wants to jump to the next level
//returns -2 if user wants to jumt to previous level
//Warning the levelTime and livesRemaining parameters are at most 3 digits
int playLevel(ALLEGRO_DISPLAY * display, const char * levelName, const char * spritesFileName, double waterRise,
	int  levelTime, int livesRemaining, int levelNumber) {



	int diamondObjective, openDoorX, openDoorY;
	//creating the map array it will later be destoyed
	MAP_CELL **map = createMapFromFile(levelName, &diamondObjective, &openDoorX, &openDoorY);

	//for the game loop
	int done = 0;
	int minerX, minerY, occupiedDiamonds = 0, minerStatus = 1;
	int levelTimeRemaining = levelTime;

	//return variable
	int levelResult = 0;

	//finding position of miner also assuming there is one
	findPositionOfMiner(map, &minerX, &minerY);

	//installing keyboard
	al_install_keyboard();
	
	ALLEGRO_BITMAP * sprites = al_load_bitmap(spritesFileName);

	//timers and their helping variables
	ALLEGRO_TIMER * timer = al_create_timer(1.0 / FPS);
	ALLEGRO_TIMER * playerTimer = al_create_timer(1.0 / PLAYER_FPS);
	ALLEGRO_TIMER * waterTimer = al_create_timer(waterRise);
	ALLEGRO_TIMER * levelTimer = al_create_timer(1.0);
	int updateEnvoirmentCountdown = ENVOIRMENT_CHANGE_MULTIPLIER;
	int timeToFinishLevelAfterFail = FINISH_TIME_AFTER_FAIL;
	int haveSetTheFailCountdown = 0;

	//registering event queues
	ALLEGRO_EVENT_QUEUE * eventQueue = al_create_event_queue();
	al_register_event_source(eventQueue, al_get_timer_event_source(playerTimer));
	al_register_event_source(eventQueue, al_get_timer_event_source(timer));
	al_register_event_source(eventQueue, al_get_timer_event_source(waterTimer));
	al_register_event_source(eventQueue, al_get_timer_event_source(levelTimer));
	al_register_event_source(eventQueue, al_get_display_event_source(display));
	al_register_event_source(eventQueue, al_get_keyboard_event_source());

	//loading the font
	ALLEGRO_FONT * font = al_load_ttf_font("font.ttf", DEFAULT_FONT_SIZE, 0);

	//for camera and controls
	ALLEGRO_EVENT events;
	ALLEGRO_KEYBOARD_STATE keyState;
	ALLEGRO_TRANSFORM camera;

	//Drawing variables

	//Every object in the game
	IMAGE_BLOCK * rightMovementImageList = createListOfImagePos(0, 5, 8, 6, 1, 1);
	IMAGE_BLOCK * leftMovementImageList = createListOfImagePos(0, 4, 8, 5, 1, 1);
	IMAGE_BLOCK * stillMovementImageList = createListOfImagePos(0, 1, 8, 4, 3, 1);
	IMAGE_BLOCK * explosionAnmImageList = createListOfImagePos(0, 0, 5, 1, 8, 0);
	IMAGE_BLOCK * metalWallImageList = createListOfImagePos(1, 6, 2, 7, 1, 1);
	IMAGE_BLOCK * brickWallImageList = createListOfImagePos(3, 6, 4, 7, 1, 1);
	IMAGE_BLOCK * openDoorImageList = createListOfImagePos(2, 6, 3, 7, 1, 1);
	IMAGE_BLOCK * rockImageList = createListOfImagePos(0, 7, 1, 8, 1, 1);
	IMAGE_BLOCK * earthImageList = createListOfImagePos(1, 7, 2, 8, 1, 1);
	IMAGE_BLOCK * diamondAnmImageList = createListOfImagePos(2, 7, 8, 8, 4, 0);
	IMAGE_BLOCK * waterImageList = createListOfImagePos(0, 8, 8, 9, 2, 1);
	IMAGE_BLOCK * spiderImageList = createListOfImagePos(0, 9, 8, 10, 3, 1); //It seems more like a firefly but lets call it spider
	IMAGE_BLOCK * monsterImageList = createListOfImagePos(0, 11, 8, 12, 3, 1);
	IMAGE_BLOCK * diamondImageList = createListOfImagePos(0, 10, 8, 11, 3, 1);


	IMAGE_BLOCK * movement = stillMovementImageList;
	IMAGE_BLOCK * explosionAnmImageListHead = explosionAnmImageList;

	//array for most of the images position on the sprite file
	IMAGE_BLOCK *anmArr[] = { metalWallImageList,brickWallImageList,stillMovementImageList,spiderImageList,
		monsterImageList,earthImageList,rockImageList,diamondImageList,openDoorImageList,waterImageList,diamondAnmImageList,
		rightMovementImageList,leftMovementImageList };



	//for the background music
	//some pointers will be destroyed in the end

	ALLEGRO_SAMPLE * music = al_load_sample("music.wav");
	if (!music) {
		printf("Problem with loading the music sample\n");
	}
	al_reserve_samples(1);
	//the tempo will increase in the last %10 of the time
	int  tempoChanged = 0;
	ALLEGRO_SAMPLE_ID musicId;
	al_play_sample(music, 1.0, 0, 1.0, ALLEGRO_PLAYMODE_LOOP, &musicId);



	//starting the timers
	al_start_timer(playerTimer);
	al_start_timer(timer);
	al_start_timer(waterTimer);
	al_start_timer(levelTimer);




	//for the game flow and pause dynamics
	int pause = 0;
	int timeWhenPressedForNextLevel = 0;
	int redraw = 1;

	//Game loop has started
	while (!done) {
		
		al_hold_bitmap_drawing(1);
		al_wait_for_event(eventQueue, &events);
		al_get_keyboard_state(&keyState);

		//if the display is closed quit game
		if (events.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
			done = 1;
			levelResult = -1;
		}


		// pressed P so user wants to pause
		if (al_key_down(&keyState, ALLEGRO_KEY_P)) {
			pause = 1;
			al_stop_timer(playerTimer);
			al_stop_timer(timer);
			al_stop_timer(waterTimer);
			al_stop_timer(levelTimer);

		}
		//R for the user to resume
		else if (al_key_down(&keyState, ALLEGRO_KEY_R)) {
			pause = 0;
			al_resume_timer(playerTimer);
			al_resume_timer(timer);
			al_resume_timer(waterTimer);
			al_resume_timer(levelTimer);
		}
		//Pressed A ,user wants to go to the previous level 
		if (al_key_down(&keyState, ALLEGRO_KEY_A)) {
			timeWhenPressedForNextLevel = levelTimeRemaining;
			levelResult = -2;
		}
		//Pressed S ,user wants to go to the next level 
		if (al_key_down(&keyState, ALLEGRO_KEY_S)) {
			timeWhenPressedForNextLevel = levelTimeRemaining;
			levelResult = 2;
		}
		if (timeWhenPressedForNextLevel - levelTimeRemaining == WAIT_FOR_LEVEL_JUMP) {
			done = 1;
		}

		//If its not in pause
		if (!pause) {
			if (events.type == ALLEGRO_EVENT_TIMER) {
				
				//If there is an event to draw
				redraw = 1;	

				//if its been a second
				//when its timer ticks the water will expand
				if (events.timer.source == waterTimer) {
					waterUpdate(map);
				}

				if (events.timer.source == levelTimer) {
					//updated the time remaining
					levelTimeRemaining--;
					//if the time is up the level is over
					if (levelTimeRemaining == 0) {
						done = 1;
						levelResult = 0;
					}

					if (levelTimeRemaining <= levelTime * 0.1 && !tempoChanged) {
						//stop the old sample and start the new one
						al_stop_sample(&musicId);
						al_play_sample(music, 1.0, 0, TEMPO_OF_MUSIC, ALLEGRO_PLAYMODE_LOOP, &musicId);
						tempoChanged = 1;

					}
					//The following 3 if's works together for finishing the level after miners death
					if (!minerStatus && !haveSetTheFailCountdown) {
						//the miner is dead now 
						haveSetTheFailCountdown = 1;
					}
					if (haveSetTheFailCountdown) {
						timeToFinishLevelAfterFail--;
					}
					if (timeToFinishLevelAfterFail == 0) {
						//stop the loop and fail the level
						done = 1;
						levelResult = 0;
					}

				}

				if (events.timer.source == playerTimer) {

					//if miner  lives
					if (minerStatus) {

						//When player presses up
						if (al_key_down(&keyState, ALLEGRO_KEY_UP)) {
							//checks if the move is available if it is changes the position
							if (moveCheck(map, ALLEGRO_KEY_UP, &occupiedDiamonds))
								minerY--;
							//movement now has the position of the right movement part of the sprites
							//later the positions will be used for the drawing
							movement = rightMovementImageList;

						}
						//right
						else if (al_key_down(&keyState, ALLEGRO_KEY_RIGHT)) {
							if (moveCheck(map, ALLEGRO_KEY_RIGHT, &occupiedDiamonds))
								minerX++;
							movement = rightMovementImageList;

						}
						//down
						else if (al_key_down(&keyState, ALLEGRO_KEY_DOWN)) {
							if (moveCheck(map, ALLEGRO_KEY_DOWN, &occupiedDiamonds))
								minerY++;
							movement = leftMovementImageList;

						}
						//left
						else if (al_key_down(&keyState, ALLEGRO_KEY_LEFT)) {
							if (moveCheck(map, ALLEGRO_KEY_LEFT, &occupiedDiamonds))
								minerX--;
							movement = leftMovementImageList;

						}
						//still
						else {
							movement = stillMovementImageList;
						}

						updateEnvoirmentCountdown--;
						//when the countdown is over it updates the envoirment
						if (updateEnvoirmentCountdown == 0) {
							//updates the gravity of rocks and the behavior of the creatures
							gravityUpdate(map);
							creatureUpdate(map);
							updateEnvoirmentCountdown = ENVOIRMENT_CHANGE_MULTIPLIER;
						}
					}
				}
				//draws the explosions 
				anmExplosion(map, sprites);
			}




		}
		
		//Camera Effects
		al_identity_transform(&camera);
		al_translate_transform(&camera, -(minerX*IMAGE_BLOCK_WIDTH + IMAGE_BLOCK_WIDTH / 2), -(minerY*IMAGE_BLOCK_HEIGHT + IMAGE_BLOCK_HEIGHT / 2));
		al_scale_transform(&camera, 1.4, 1.4);
		al_translate_transform(&camera, (minerX*IMAGE_BLOCK_WIDTH + IMAGE_BLOCK_WIDTH / 2) - (-(SCREEN_WIDTH*0.5) + minerX * IMAGE_BLOCK_WIDTH + IMAGE_BLOCK_WIDTH / 2),
			(minerY*IMAGE_BLOCK_HEIGHT + IMAGE_BLOCK_HEIGHT / 2) - (-(SCREEN_HEIGHT*0.5) + minerY * IMAGE_BLOCK_HEIGHT + IMAGE_BLOCK_HEIGHT / 2));
		al_use_transform(&camera);





		//Drawing the miner if he is present
		if (minerStatus) {
			minerStatus = markExplosion(map, minerX, minerY, diamondAnmImageList, explosionAnmImageListHead);
			//to the next animation
			leftMovementImageList = leftMovementImageList->next;
			stillMovementImageList = stillMovementImageList->next;
			rightMovementImageList = rightMovementImageList->next;
			//draw the miner
			al_draw_bitmap_region(sprites, movement->x*IMAGE_BLOCK_WIDTH, movement->y*IMAGE_BLOCK_HEIGHT,
				IMAGE_BLOCK_WIDTH, IMAGE_BLOCK_HEIGHT, minerX * IMAGE_BLOCK_WIDTH, minerY * IMAGE_BLOCK_HEIGHT, 0);


			//If the miner is near the door
			if (map[minerX + 1][minerY].type == door || map[minerX - 1][minerY].type == door || map[minerX][minerY + 1].type == door || map[minerX][minerY - 1].type == door) {
				done = 1;
				levelResult = 1;
			}
		}
		//For caution if the miner isn't marked exploded
		int a, b;
		findPositionOfMiner(map, &a, &b);
		if (a == 0 && b == 0)
			minerStatus = 0;

		//if the objective has met open the door
		if (diamondObjective == occupiedDiamonds) {
			map[openDoorX][openDoorY].type = door;
		}

		//draw the map
		drawMap(map, anmArr, sprites);
		//to the next animation
		moveToNextAnimation(anmArr, 13);


		//For the indicators on the top of the screen (the mod operation below is just for caution)
		char Time = 'T';
		const char * timeIndicator = indicatorCreator(Time, levelTimeRemaining % 1000);
		char Merits = 'M';
		const char * meritIndicator = indicatorCreator(Merits, livesRemaining % 1000);
		char Diamonds = 'D';
		const char * diamondIndicator = indicatorCreator(Diamonds, occupiedDiamonds % 1000);
		char Level = 'L';
		const char * levelIndicator = indicatorCreator(Level, levelNumber % 1000);

		//draw the time left
		al_draw_text(font, al_map_rgb(255, 255, 255), (minerX*IMAGE_BLOCK_WIDTH - SCREEN_WIDTH / (2 * SCALE_MULTIPLIER) + IMAGE_BLOCK_WIDTH * 1),
			(minerY*IMAGE_BLOCK_WIDTH - SCREEN_HEIGHT / (2 * SCALE_MULTIPLIER) + IMAGE_BLOCK_HEIGHT * 1), 0, timeIndicator);
		//draw the diamonds
		al_draw_text(font, al_map_rgb(255, 255, 255), (minerX*IMAGE_BLOCK_WIDTH - SCREEN_WIDTH / (2 * SCALE_MULTIPLIER) + IMAGE_BLOCK_WIDTH * 5),
			(minerY*IMAGE_BLOCK_WIDTH - SCREEN_HEIGHT / (2 * SCALE_MULTIPLIER) + IMAGE_BLOCK_HEIGHT * 1), 0, diamondIndicator);
		//draw the merits
		al_draw_text(font, al_map_rgb(255, 255, 255), (minerX*IMAGE_BLOCK_WIDTH - SCREEN_WIDTH / (2 * SCALE_MULTIPLIER) + IMAGE_BLOCK_WIDTH * 9),
			(minerY*IMAGE_BLOCK_WIDTH - SCREEN_HEIGHT / (2 * SCALE_MULTIPLIER) + IMAGE_BLOCK_HEIGHT * 1), 0, meritIndicator);
		//draw the level
		al_draw_text(font, al_map_rgb(255, 255, 255), (minerX*IMAGE_BLOCK_WIDTH - SCREEN_WIDTH / (2 * SCALE_MULTIPLIER) + IMAGE_BLOCK_WIDTH * 13),
			(minerY*IMAGE_BLOCK_WIDTH - SCREEN_HEIGHT / (2 * SCALE_MULTIPLIER) + IMAGE_BLOCK_HEIGHT * 1), 0, levelIndicator);

		//if paused
		if (pause) {
			//write a text if paused
			al_draw_text(font, al_map_rgb(255, 255, 255),
				(minerX*IMAGE_BLOCK_WIDTH - SCREEN_WIDTH / (2 * SCALE_MULTIPLIER) + IMAGE_BLOCK_WIDTH * 1),
				(minerY*IMAGE_BLOCK_WIDTH - SCREEN_HEIGHT / (2 * SCALE_MULTIPLIER) + IMAGE_BLOCK_HEIGHT * 3),
				0, "Paused");
		}

		al_hold_bitmap_drawing(0);
		if(redraw && al_is_event_queue_empty(eventQueue)){
			
			al_flip_display();
			al_clear_to_color(al_map_rgb(0, 0, 0));
		}

	}
	//Destroying the adresses allocated
	free(map);
	free(rightMovementImageList);
	free(leftMovementImageList);
	free(stillMovementImageList);
	free(explosionAnmImageList);
	free(metalWallImageList);
	free(brickWallImageList);
	free(openDoorImageList);
	free(rockImageList);
	free(earthImageList);
	free(diamondAnmImageList);
	free(waterImageList);
	free(spiderImageList);
	free(monsterImageList);
	free(diamondImageList);
	al_destroy_timer(timer);
	al_destroy_timer(playerTimer);
	al_destroy_timer(waterTimer);
	al_destroy_timer(levelTimer);
	al_destroy_bitmap(sprites);
	al_destroy_event_queue(eventQueue);
	al_destroy_sample(music);

	return levelResult;
}

//For the gui takes a letter the numerical value and returns a cont string
// The format is like this T:100 or L:999 always a letter and a 3 digit integer
//Warning!!! Assumes the integer is 3 digit at most
const char * indicatorCreator(char letter, int num) {

	char * string = (char *)calloc(sizeof(char), 6);
	//marking null
	string[5] = 0;
	string[0] = letter;
	//58 is : in ascii
	string[1] = 58;
	//taking the digits one by one 
	string[2] = num / 100 + 48;
	string[3] = (num / 10) % 10 + 48;
	string[4] = (num % 10) + 48;

	const char * result = string;

	return result;
}
//For the playLevel functions parameter
//The format is like level02 or level99 
//Warning !! Assumes the num is a 2 digit int 
const char * levelNameCreator(int num) {
	char * string = (char *)calloc(sizeof(char), 8);
	//marking the null
	string[7] = '\0';
	//writing level...
	string[0] = 'l';
	string[1] = 'e';
	string[2] = 'v';
	string[3] = 'e';
	string[4] = 'l';
	//taking the digits
	string[5] = (num % 100) / 10 + 48;
	string[6] = (num % 100) % 10 + 48;

	const char * result = string;

	return result;
}


//For the flow of levels will star agian if the level has failed and finish when the level has finished
void levelFlow(ALLEGRO_DISPLAY * display, const char * spritesFileName, LEVEL levels[], int startMerits, int levelQuantity) {
	//Variables for the loop
	int currentLevel = 1, done = 0, levelStatus = 0;
	int meritsRemaining = startMerits;


	while (!done) {
		//play the level and get the result
		levelStatus = playLevel(display, levelNameCreator(currentLevel), spritesFileName, levels[currentLevel - 1].waterRiseTime
			, levels[currentLevel - 1].levelTime, meritsRemaining, currentLevel);


		//If the level has sucessfuly finished 
		if (levelStatus == 1) {
			currentLevel++;
			//If all the levels has finished
			if (currentLevel == levelQuantity + 1) {
				done = 1;
			}
		}

		//If the miner has died or the time is up take one merit away
		else if (levelStatus == 0) {
			meritsRemaining--;
		}
		//If the user wants to quit
		else if (levelStatus == -1) {
			done = 1;
		}
		//If user wants to go to the next level
		else if (levelStatus == 2 && currentLevel != levelQuantity) {
			currentLevel++;
		}
		//If user wants to go to the previous level
		else if (levelStatus == -2 && currentLevel != 1) {
			currentLevel--;
		}
		//If no merit remains
		if (meritsRemaining == 0) {
			done = 1;
		}
	}
}

