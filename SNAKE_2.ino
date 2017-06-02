#include "Arduino.h"
#include "Keypad.h"
#include "HT1632.h"
//#include <StandardCplusplus.h>
//#include <list>
#include <EEPROM.h>

#define DATA 2
#define WR   3
#define CS   4
#define CS2  5

//r2

//Prototypes
typedef struct {
	char x,y;
}Coordinate;

typedef struct
{
	Coordinate* buffer;
	int size;
	int tail;
	int head;
}ring;

bool isInSnake(Coordinate Q);
bool isOutOfBounds(Coordinate Q);
int push(Coordinate co, ring *r);
Coordinate front(ring *r);
Coordinate pop(ring *r);
void Main_Thread();
void mainMenu();
void gameInProgress();
void highScores();
void nextLevel();
//void SnakeDirection_Thread();
void GetKey_Thread(int ct);

#define LEFT_LIMIT 0
#define RIGHT_LIMIT 15
#define UP_LIMIT 23
#define DOWN_LIMIT 0

char upper = 16;
char lower = -1;
char y_upper = 24;
char y_lower = -1;





//key bindings
typedef enum {BTN_1, BTN_2, BTN_3, BTN_4, BTN_5, BTN_6, BTN_7, BTN_8, BTN_9, BTN_STAR, BTN_0, BTN_POUND} buttons;

//State Values from State Chart
//1. Snake Direction State
typedef enum {
	UP,
	DOWN,
	LEFT,
	RIGHT
} Direction_st;

//2. Snake State
typedef enum {
	PAUSED,
	WAIT,
	MOVE,
	EAT,
	FRUIT
} Snake_st;

//3. HighScores States
typedef enum {
	GAMEOVER,
	DISPLAY_HIGH_SCORES,
	ENTER_HIGH_SCORES
} Highscores_st;

//4. TOPLEVEL STATE
typedef enum {
	MAIN_MENU,
	GAME_INPROGRESS,
	HIGH_SCORES
} Toplevel_st;

//5. GAMEPAD STATE
typedef enum {
	PAD_UP = BTN_2,
	PAD_DOWN = BTN_8,
	PAD_LEFT = BTN_4,
	PAD_RIGHT = BTN_6,
	PAD_PAUSE = BTN_STAR,
	PAD_NONE = 0
} Gamepad_st;

typedef enum{
	HIGH_SCORES_SELECTED,
	NEW_GAME_SELECTED,
	SET_SPEED_SELECTED
}MenuSelection_st;


//Level State
typedef enum{
		LEVEL1,
		LEVEL2,
		LEVEL3,
		LEVEL4
}Level_st;


//initial states
Direction_st direction_st = RIGHT;
Snake_st snake_st = WAIT;
Highscores_st highscores_st = GAMEOVER;
Toplevel_st toplevel_st = MAIN_MENU;
Gamepad_st gamepad_st = PAD_NONE;
MenuSelection_st menuSelection_st = NEW_GAME_SELECTED;
unsigned int score = 45;
unsigned int size = 0;
Level_st level_st = LEVEL1;

// use this line for single matrix
HT1632LEDMatrix matrix = HT1632LEDMatrix(DATA, WR, CS);
// use this line for two matrices!
//HT1632LEDMatrix matrix = HT1632LEDMatrix(DATA, WR, CS, CS2);


const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
	{BTN_1,BTN_2,BTN_3},
	{BTN_4,BTN_5,BTN_6},
	{BTN_7,BTN_8,BTN_9},
	{BTN_STAR,BTN_0,BTN_POUND}
};

char title[6][24] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0},
	{0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0},
	{0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0},
	{0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0},
	{0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0},
};

using namespace std;




//list<Coordinate> snake;




char directionx = 0;
char directiony = 0;

byte rowPins[ROWS] = {9,10,11,12}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6,8,7}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );


int ct = 0;
Coordinate fruit = {.x=10,.y=10};
Coordinate direction = {.x=0,.y=1};

typedef struct
{
	char d,x,y;
}dir;

dir sd[4][12];
//Snake_st pauseFsd[5][2];

int highScore=5;
int h_i = 0;
char temp[4] = " ";
char HIGHSCORES_Names[3][4] = {"AAA","BBB","CCC"};
short HIGHSCORES_Values[3] = {3,2,1};




//Ring Code


int push(Coordinate co, ring *r){
	if (r->tail + 1 % r->size == r->head )
	{
		return -1;
		
	} 
	else
	{
		
		r->tail = (r->tail +1)%(r->size);
		r->buffer[r->tail] = co;
		return 1;
	}
	
	
}

Coordinate front(ring *r){
	return r->buffer[r->tail]; //
}



Coordinate pop(ring *r){
	Coordinate temp;
	if (r->tail == r->head)
	{
		temp.x = 0;
		temp.y = 0;
		return temp;
	}
	else{
		
		r->head = (r->head +1) % r->size;
		temp = r->buffer[r->head];
		return temp;
	}
}

//End of Ring Code




ring snake;

void setup(){
	int i =0;
	char key =0;
	
	
	snake.buffer = (Coordinate*)malloc(sizeof(Coordinate)*385);
	snake.tail = 0;
	snake.head = 0;
	snake.size = 385;
	
	
	//test = (char*)malloc(sizeof(char)*400);
	//test[0] = 20;
	//test[1] = 21;
	//test[399] = 21;
	//list<char> test = new list<char>;
	
	EEPROM.get(0,HIGHSCORES_Names[0]);
	EEPROM.get(4,HIGHSCORES_Names[1]);
	EEPROM.get(8,HIGHSCORES_Names[2]);
	EEPROM.get(12,HIGHSCORES_Values[0]);
	EEPROM.get(14,HIGHSCORES_Values[1]);
	EEPROM.get(16,HIGHSCORES_Values[2]);

	Serial.begin(9600);
	//Ring Buffer Unit Tests 
	ring rng1;
	rng1.tail = 0;
	rng1.head = 0;
	rng1.size = 20;
	rng1.buffer = (Coordinate*)malloc(sizeof(Coordinate)*20);
	int res = 0;
	Coordinate t;
	Serial.print("Push:");
	res = push((Coordinate){10,2},&rng1);
	res = push((Coordinate){10,3},&rng1);
	res = push((Coordinate){10,4},&rng1);
	Serial.println(res);
	t = front(&rng1);
	Serial.print("Front: ");
	Serial.print((int)t.x);
	Serial.print(",");
	Serial.println((int)t.y);
	t = pop(&rng1);
	Serial.print("Popping: ");
	Serial.print((int)t.x);
	Serial.print(",");
	Serial.println((int)t.y);
	t = pop(&rng1);
	Serial.print("Popping: ");
	Serial.print((int)t.x);
	Serial.print(",");
	Serial.println((int)t.y);
	t = pop(&rng1);
	Serial.print("Popping: ");
	Serial.print((int)t.x);
	Serial.print(",");
	Serial.println((int)t.y);	
	
		t = pop(&rng1);
		Serial.print("Popping: ");
		Serial.print((int)t.x);
		Serial.print(",");
		Serial.println((int)t.y);
	
	//End of ring buffer tests
	
	
	//Serial.println("Begin Setup");
	
	//Initialize Snake Direction Finite State Machine
	dir up_dir = {.d = UP, .x = -1, .y = 0};
	dir down_dir = {.d = DOWN, .x = 1, .y = 0};
	dir left_dir = {.d = LEFT, .x = 0, .y = -1};
	dir right_dir = {.d = RIGHT, .x = 0, .y = 1};
	
	for(int j=0; j<12;j++){
		sd[UP][j]=up_dir;
		sd[DOWN][j]=down_dir;
		sd[LEFT][j]=left_dir;
		sd[RIGHT][j]=right_dir;
	}
	
	//sd[UP][PAD_UP] = up_dir;
	//sd[UP][PAD_DOWN] = up_dir;
	sd[UP][PAD_RIGHT] = right_dir;
	sd[UP][PAD_LEFT] = left_dir;
	//sd[DOWN][PAD_UP] = down_dir ;
	//sd[DOWN][PAD_DOWN] = down_dir;
	sd[DOWN][PAD_RIGHT] = right_dir;
	sd[DOWN][PAD_LEFT] = left_dir;
	sd[LEFT][PAD_UP] = up_dir;
	sd[LEFT][PAD_DOWN] = down_dir;
	//sd[LEFT][PAD_RIGHT] = left_dir;
	//sd[LEFT][PAD_LEFT] = left_dir;
	sd[RIGHT][PAD_UP] = up_dir;
	sd[RIGHT][PAD_DOWN] = down_dir;
	//sd[RIGHT][PAD_LEFT] = right_dir;
	//sd[RIGHT][PAD_RIGHT] = right_dir;

	
	//pauseFsd[PAUSED][PAD_PAUSE] = WAIT;
	//pauseFsd[WAIT][PAD_PAUSE] = PAUSED;
	//pauseFsd[EAT][PAD_PAUSE] = PAUSED;
	//pauseFsd[MOVE][PAD_PAUSE] = PAUSED;
	//pauseFsd[FRUIT][PAD_PAUSE] = PAUSED;
	
	//PAD_UP = BTN_2,
	//PAD_DOWN = BTN_8,
	//PAD_LEFT = BTN_4,
	//PAD_RIGHT = BTN_6,
	//PAD_PAUSE = BTN_STAR,
	
	matrix.begin(HT1632_COMMON_16NMOS);
	matrix.fillScreen();
	//delay(500);
	matrix.clearScreen();
	//snake.push_front(Coordinate(10,2));
	//snake.push_front(Coordinate(10,3));
	//snake.push_front(Coordinate(10,4));
	
	i = 0;
	while(i<10){
		matrix.clearScreen();
		for (uint8_t y=0; y<24; y++) {
			for (uint8_t x=0; x<6; x++) {
				if (title[x][y] == 1){
					matrix.setPixel(y, x+10-i);
					
				}
			}
		}
		matrix.writeScreen();
		delay(100);
		i++;
	}
	// blink!
	matrix.blink(true);
	delay(2000);
	matrix.blink(false);
	
	// Adjust the brightness down
	for (int8_t i=15; i>=0; i--) {
		matrix.setBrightness(i);
		char key = keypad.getKey();
		if (key != NO_KEY){
			////Serial.println(key);
		}
		delay(100);
	}
	// then back up
	for (uint8_t i=0; i<16; i++) {
		matrix.setBrightness(i);
		char key = keypad.getKey();
		
		if (key != NO_KEY){
			////Serial.println(key);
		}
		delay(100);
	}

	//for (uint8_t y=0; y<matrix.height(); y++) {
		//for (uint8_t x=0; x< matrix.width(); x++) {
			//matrix.clrPixel(x, y);
			//matrix.writeScreen();
		//}
	//}

	matrix.setPixel(fruit.y, fruit.x);
	matrix.writeScreen();
	//delay(300);
	matrix.setTextColor(1);
	matrix.setTextSize(1);
	i = 0;
	//while (i<50)
	//{
		//String test = "SNAKE\nby\nALI\nSULE\nIMAN";
		//matrix.setCursor(0,(i)*-1);
		//matrix.print(test);
		//matrix.writeScreen();
		//delay(100);
	
		//i++;
		//
	//}
	//Serial.println("End Of Setup");
	//matrix.print("ENDOFSETUP");
	//matrix.writeScreen();
	
	fruit.x = 10;
	fruit.y = 10;
	matrix.clearScreen();
	
}



void Main_Thread(){
	//Serial.println("starmainthread1");
	//matrix.println("MainTHR");
	//TOP LEVEL
	//while(1){
	switch(toplevel_st){
		case MAIN_MENU:
		mainMenu();
		break;
		case GAME_INPROGRESS:
		gameInProgress();
		break;
		case HIGH_SCORES:
		highScores();
		break;
		
	}
	//}
}

void reset(){
	direction_st = RIGHT;
	direction.x = 0;
	direction.y = 1;
	upper = 15;
	lower = 0;
	snake_st = WAIT;
	highscores_st = GAMEOVER;
	level_st = LEVEL4;
	//toplevel_st = MAIN_MENU;
	gamepad_st = PAD_NONE;
	menuSelection_st = NEW_GAME_SELECTED;
	score = 0;
	fruit.x = 10;
	fruit.y = 10;
	matrix.setPixel(fruit.y, fruit.x);
}

void mainMenu(){
	//matrix.print("Main MEnu");
	//Serial.println("Main Menu");
	//Serial.println(gamepad_st);
	if(gamepad_st != PAD_NONE){
		//Serial.println("KEY Present");
		if(gamepad_st == PAD_UP){
			matrix.clearScreen();
			matrix.setCursor(0,0);
			menuSelection_st = (MenuSelection_st)(((unsigned char)menuSelection_st+1)%2);
			//Serial.println("High Scores Selected");
			gamepad_st = PAD_NONE;
		}
		if (gamepad_st == PAD_DOWN)
		{
			matrix.clearScreen();
			matrix.setCursor(0,0);
			menuSelection_st = (MenuSelection_st)(((unsigned char)menuSelection_st-1)%2*-1);
			//Serial.print("NEW\n GAME SELECTED");
			
			gamepad_st = PAD_NONE;
		}
		if(gamepad_st == PAD_PAUSE){
			matrix.clearScreen();
			if(menuSelection_st == HIGH_SCORES_SELECTED){
				toplevel_st = HIGH_SCORES;
				highscores_st = DISPLAY_HIGH_SCORES;
				gamepad_st = PAD_NONE;
				h_i = 0;
				return;
			}
			if(menuSelection_st == NEW_GAME_SELECTED ){
				reset();
				toplevel_st = GAME_INPROGRESS;
				gamepad_st = PAD_NONE;
				size = 3;
				int i =0;
				snake.head = 0;
				snake.tail = 0;
				y_upper = 24;
				y_lower = -1;
				upper = 16;
				lower = -1;

				
				
				push((Coordinate){10,2},&snake);
				push((Coordinate){10,3},&snake);
				//push((Coordinate){10,4},&snake);	
				
				return;
			}
			else{
				//TODO
				

				
				return;
			}
			
			
		}

	}
	if(menuSelection_st == HIGH_SCORES_SELECTED){
		//Serial.println("High Scores Selected");
		
		matrix.setCursor(0,0);
		matrix.print("VIEW\nHISCORES");
		matrix.writeScreen();
	}
	else if(menuSelection_st == NEW_GAME_SELECTED){
		//Serial.println("New Game Selected");
		
		matrix.setCursor(0,0);
		matrix.print("NEW\nGAME");
		matrix.writeScreen();
	}
	else{
		//Serial.println("New Game Selected");
		matrix.setCursor(0,0);
		matrix.print("!???\n");
		matrix.print(menuSelection_st);
		
		matrix.writeScreen();
	}
	//if(menuSelection_st == SET_SPEED_SELECTED){
	////Serial.println("New Game Selected");
	//matrix.print("SET\nSPEED");
	//matrix.setCursor(0,0);
	//matrix.writeScreen();
	//}
	
	
}

void nextLevel(){
	Serial.println("Level Change");
	level_st = (Level_st)(((unsigned char)level_st+1)%4);
	int i =0;
		y_upper = 24;
		y_lower = -1;
		upper = 16;
		lower = -1;
			if (level_st == LEVEL3)
			{
				matrix.fillScreen();
			}
			else{
				matrix.clearScreen();
			}
		i=0;
		while(i < (size-2) ){pop(&snake);i++;}//.pop_back()
		size = 3;

		i =0;
		while(i<10){
			//for (list<Coordinate>::iterator it = snake.begin(); it != snake.end(); it++)
			for (int it = snake.head; it != (snake.tail+1)%snake.size; it = (it+1)%snake.size)
			{
				//matrix.clrPixel(it->y, it->x);
				matrix.setPixel(snake.buffer[it].y, snake.buffer[it].x);
			}
			matrix.writeScreen();
			delay(50);
			for (int it = snake.head; it != (snake.tail+1)%snake.size; it = (it+1)%snake.size)
			{
				//matrix.clrPixel(it->y, it->x);
				matrix.clrPixel(snake.buffer[it].y, snake.buffer[it].x);
			}
			delay(50);
			//if (level_st == LEVEL3)
			//{
				//matrix.fillScreen();
			//}
			//else{
				//matrix.clearScreen();
			//}
			
			//
			matrix.writeScreen();
			delay(60);
			i++;
		}
		
		
		
}



void gameInProgress(){
	//Serial.println("+ Game in PRogresss");
	////Serial.print("direction x:");
	////Serial.println(direction.x);
	////Serial.print("direction.y");
	////Serial.print(direction.y);
	////Serial.println(snake_st);
	//delay(1000);
	int i = 0;
	Coordinate T = {.x=0,.y=0};
	Coordinate co = {.x=0,.y=0};
	
	if (snake_st == PAUSED){

		if(gamepad_st == PAD_PAUSE){	snake_st = WAIT;
			if(level_st == LEVEL3){
				matrix.fillScreen();
			}else{
				matrix.clearScreen();
				
			}
			
			////Serial.println("Paused");
			
		gamepad_st = (Gamepad_st)NO_KEY;}
	}
	else{
		if(gamepad_st == PAD_PAUSE)	{
			snake_st = PAUSED;
			matrix.clearScreen();
			matrix.setCursor(0,0);
			matrix.print("Paused\n");
			matrix.print(score);
			matrix.writeScreen();
			gamepad_st = (Gamepad_st)NO_KEY;
		}
		////Serial.println("unPaused")
		
	}
	
	
	
	if (snake_st == WAIT)snake_st = MOVE;
	
	while(snake_st != WAIT && snake_st != PAUSED){
		switch (snake_st)
		{
			case MOVE:
			////Serial.println("MOVE");

			//Get next position
			//BMARK
			//Common Code
			T.x = front(&snake).x+direction.x;
			T.y =front(&snake).y+direction.y;
			//Check for GameOver
			if( isInSnake(T) || isOutOfBounds(T)){
				////Serial.println("GAMEOVER");
				toplevel_st = HIGH_SCORES;
				
				highscores_st = GAMEOVER; //************
				return;
			}
			push(T,&snake);
			//snake.push_front(T);
			
			switch(level_st){
				case LEVEL1:
				case LEVEL2:
					i =0;
					for (int it = snake.head; it != (snake.tail+1)%snake.size; it = (it+1)%snake.size)
					{
						matrix.setPixel(snake.buffer[it].y, snake.buffer[it].x);

					}
					matrix.setPixel(fruit.y, fruit.x);
					
					if(front(&snake).y == fruit.y && front(&snake).x == fruit.x){
						snake_st = FRUIT;
						return;
					}
					co = snake.buffer[snake.head]; //snake back
					matrix.clrPixel(co.y, co.x);
					
					for (int j =0; j<=lower;j++)
					{
											
						for(i=0;i<24;i++){
							matrix.setPixel(i,upper+j);
							matrix.setPixel(i,lower-j);
						}
					}
				break;
				
				case LEVEL3:
					for (int it = snake.head;it != (snake.tail+1)%snake.size; it = (it+1)%snake.size)
					{
						//matrix.clrPixel(it->y, it->x);
						matrix.clrPixel(snake.buffer[it].y, snake.buffer[it].x);
					}
					matrix.clrPixel(fruit.y, fruit.x);
					
					if(front(&snake).y == fruit.y && front(&snake).x == fruit.x){
						snake_st = FRUIT;
						return;
					}
					co = snake.buffer[snake.head]; //snake back
					matrix.setPixel(co.y, co.x);
					
					for (int j =0; j<=y_lower;j++)
					{
						for(i=0;i<16;i++){
							matrix.clrPixel(y_upper+j,i);
							matrix.clrPixel(y_lower-j,i);
						}
					}
				break;
				
				case LEVEL4:
					i =0;
					for (int it = snake.head; it != (snake.tail+1)%snake.size; it = (it+1)%snake.size)
					{
						matrix.setPixel(snake.buffer[it].y, snake.buffer[it].x);

					}
					matrix.setPixel(fruit.y, fruit.x);
									
					if(front(&snake).y == fruit.y && front(&snake).x == fruit.x){
						snake_st = FRUIT;
						return;
					}
					co = snake.buffer[snake.head]; //snake back
					matrix.clrPixel(co.y, co.x);
									
					for (int j =0; j<=lower;j++)
					{
										
						for(i=0;i<24;i++){
							matrix.setPixel(i,upper+j);
							matrix.setPixel(i,lower-j);
						}
					}
					for (int j =0; j<=y_lower;j++)
					{
						for(i=0;i<16;i++){
							matrix.setPixel(y_upper+j,i);
							matrix.setPixel(y_lower-j,i);
						}
					}
				break;
	
			}		

				pop(&snake);
				snake_st = WAIT;
				
			
			break; //break for move
			//case EAT: //Flagged for remove
			//break;
			case FRUIT:
			score++;
			size++;
			Serial.println("Size:");
			Serial.println(size);

			
			if(size%5 == 4){
				switch(level_st){
					case LEVEL1:
						if(size%10 == 9){
							nextLevel();
						}
					break;
				
					case LEVEL2:
						upper--;
						lower++;
						if(size%45 == 44){
							nextLevel();
						}
					break;
				
					case LEVEL3:
						y_upper-=2;
						y_lower+=2;
						if(size%33 == 32){
							nextLevel();
						}
					break;
				
					case LEVEL4:
						upper--;
						lower++;
						y_upper-=2;
						y_lower+=2;
						if(size%33 == 32){
							nextLevel();
						}
					break;
				
				}
			}
			

			
			if(size%5 == 3 || (upper - lower)<5){
				fruit.x = random(lower+4,upper-4);
				fruit.y = random(y_lower+4,y_upper-4);
			}
			else{
				fruit.x = random(lower+1,upper-1);
				fruit.y = random(y_lower+1,y_upper-1);
			}
			i = 0;
			while((isInSnake(fruit)|| isOutOfBounds(fruit) ) && i <24){
				fruit.x = random(lower+2+(i%2),upper-2);
				fruit.y = random(0,24-i);
				i++;
			}
			i = 0;
			while((isOutOfBounds(fruit) ) && i <24){
				fruit.x = lower+1;
				fruit.y = 12;
				i++;
			}
			if(score == 44){ //Freezing around 75ee
				fruit.x = random(lower+2,upper-2);
				fruit.y = random(0,24-i);
			}
			
			matrix.setPixel(fruit.y, fruit.x);
			snake_st = WAIT;
			break;
			
		}
		
	}

	
	matrix.writeScreen();
	////Serial.println("- Game in PRogresss");
	
}


void highScores(){
	
	//while(1){
	switch(highscores_st){
		case GAMEOVER:
		matrix.clearScreen();
		matrix.setCursor(0,0);
		matrix.print("GAME\nOVER");
		matrix.writeScreen();
		delay(2000);
		matrix.clearScreen();
		matrix.setCursor(0,0);
		matrix.print("SCOR\n");
		matrix.print(score);
		matrix.writeScreen();
		delay(2000);
		matrix.clearScreen();
		if (score > HIGHSCORES_Values[2]){
			highscores_st = ENTER_HIGH_SCORES;
			matrix.setCursor(0,0);
			temp[0] = '_';
			temp[1] = ' ';
			temp[2] = ' ';
			temp[3] = ' ';
			//temp[4] = 0;
			h_i = 0;
			gamepad_st = PAD_NONE;
		}
		else{
			highscores_st = DISPLAY_HIGH_SCORES;
			h_i = 0;
			
		}
		break;
		case DISPLAY_HIGH_SCORES:
		
		if(h_i > -50){
			matrix.clearScreen();
			matrix.setCursor(0,h_i);
			matrix.println(HIGHSCORES_Names[0]);
			matrix.println(HIGHSCORES_Values[0]);
			matrix.println(HIGHSCORES_Names[1]);
			matrix.println(HIGHSCORES_Values[1]);
			matrix.println(HIGHSCORES_Names[2]);
			matrix.println(HIGHSCORES_Values[2]);
			matrix.writeScreen();
			if(gamepad_st == BTN_POUND){
				HIGHSCORES_Names[0][0] = 'A';
				HIGHSCORES_Names[0][1] = 'A';
				HIGHSCORES_Names[0][2] = 'A';
				HIGHSCORES_Names[1][0] = 'B';
				HIGHSCORES_Names[1][1] = 'B';
				HIGHSCORES_Names[1][2] = 'B';
				HIGHSCORES_Names[2][0] = 'C';
				HIGHSCORES_Names[2][1] = 'C';
				HIGHSCORES_Names[2][2] = 'C';
				HIGHSCORES_Values[0] = 3;
				HIGHSCORES_Values[1] = 2;
				HIGHSCORES_Values[2] = 1;
				
				EEPROM.put(0,HIGHSCORES_Names[0]);
				EEPROM.put(4,HIGHSCORES_Names[1]);
				EEPROM.put(8,HIGHSCORES_Names[2]);
				EEPROM.put(12,HIGHSCORES_Values[0]);
				EEPROM.put(14,HIGHSCORES_Values[1]);
				EEPROM.put(16,HIGHSCORES_Values[2]);
				
			}
			if(gamepad_st == PAD_PAUSE){toplevel_st = MAIN_MENU; gamepad_st=PAD_NONE;matrix.clearScreen();return;}
			h_i--;
			//delay(100);
		}
		else{
			toplevel_st = MAIN_MENU;
		}
		return;
		break;
		matrix.clearScreen();
		case ENTER_HIGH_SCORES:
		if (gamepad_st == PAD_UP)(temp[h_i]++);
		if (gamepad_st == PAD_DOWN)temp[h_i]--;
		if (gamepad_st == PAD_PAUSE) {
			h_i++;
			if (h_i < 3)
			{
				temp[h_i] = '_';
				
			}
			
			
		}
		if (gamepad_st != NO_KEY) gamepad_st = (Gamepad_st)NO_KEY;
		
		matrix.clearScreen();
		matrix.setCursor(0,0);
		//temp[4] = '\0';
		matrix.print(temp[0]);
		matrix.print(temp[1]);
		matrix.print(temp[2]);
		matrix.print("\n");
		//matrix.setCursor(0,0);
		matrix.print(score);
		matrix.writeScreen();

		
		if(h_i == 3){
			highscores_st = DISPLAY_HIGH_SCORES;
			//for(h_i = 2;h_i>=0;h_i--){
			if(score > HIGHSCORES_Values[0]){
				
				HIGHSCORES_Values[2] = HIGHSCORES_Values[1];
				HIGHSCORES_Names[2][0] = HIGHSCORES_Names[1][0];
				HIGHSCORES_Names[2][1] =HIGHSCORES_Names[1][1];
				HIGHSCORES_Names[2][2] = HIGHSCORES_Names[1][2];
				
				HIGHSCORES_Values[1] = HIGHSCORES_Values[0];
				HIGHSCORES_Names[1][0] = HIGHSCORES_Names[0][0];
				HIGHSCORES_Names[1][1] = HIGHSCORES_Names[0][1];
				HIGHSCORES_Names[1][2] = HIGHSCORES_Names[0][2];
				
				HIGHSCORES_Values[0] = score;
				HIGHSCORES_Names[0][0] = temp[0];
				HIGHSCORES_Names[0][1] = temp[1];
				HIGHSCORES_Names[0][2] = temp[2];
				
				EEPROM.put(0,HIGHSCORES_Names[0]);
				EEPROM.put(4,HIGHSCORES_Names[1]);
				EEPROM.put(8,HIGHSCORES_Names[2]);
				EEPROM.put(12,HIGHSCORES_Values[0]);
				EEPROM.put(14,HIGHSCORES_Values[1]);
				EEPROM.put(16,HIGHSCORES_Values[2]);
				
				
				
				
				return;
			}
			if(score > HIGHSCORES_Values[1]){
				
				HIGHSCORES_Values[2] = HIGHSCORES_Values[1];
				HIGHSCORES_Names[2][0] = HIGHSCORES_Names[1][0];
				HIGHSCORES_Names[2][1] =HIGHSCORES_Names[1][1];
				HIGHSCORES_Names[2][2] = HIGHSCORES_Names[1][2];
				
				HIGHSCORES_Values[1] = score;
				HIGHSCORES_Names[1][0] = temp[0];
				HIGHSCORES_Names[1][1] = temp[1];
				HIGHSCORES_Names[1][2] = temp[2];
				
				EEPROM.put(0,HIGHSCORES_Names[0]);
				EEPROM.put(4,HIGHSCORES_Names[1]);
				EEPROM.put(8,HIGHSCORES_Names[2]);
				EEPROM.put(12,HIGHSCORES_Values[0]);
				EEPROM.put(14,HIGHSCORES_Values[1]);
				EEPROM.put(16,HIGHSCORES_Values[2]);
				
				return;
			}
			if(score > HIGHSCORES_Values[2]){
				
				HIGHSCORES_Values[2] = score;
				HIGHSCORES_Names[2][0] = temp[0];
				HIGHSCORES_Names[2][1] = temp[1];
				HIGHSCORES_Names[2][2] = temp[2];
				
				EEPROM.put(0,HIGHSCORES_Names[0]);
				EEPROM.put(4,HIGHSCORES_Names[1]);
				EEPROM.put(8,HIGHSCORES_Names[2]);
				EEPROM.put(12,HIGHSCORES_Values[0]);
				EEPROM.put(14,HIGHSCORES_Values[1]);
				EEPROM.put(16,HIGHSCORES_Values[2]);
				
				return;
			}
			//}
		}
		break;
	}
	//}
}


bool isInSnake(Coordinate Q){
	for (int it = snake.head; it != (snake.tail+1)%snake.size; it = (it+1)%snake.size)
	{
		
		if(snake.buffer[it].y == Q.y && snake.buffer[it].x == Q.x) return true;
	}
	return false;
	
}

bool isOutOfBounds(Coordinate Q){
	//if (Q.x < LEFT_LIMIT || Q.x > RIGHT_LIMIT ) return true;
	if (Q.x <= lower || Q.x >= upper ) return true;
	if (Q.y >= y_upper || Q.y <= y_lower ) return true;
	return false;
}


void GetKey_Thread(int ct){
	int i =0;
	char key = keypad.getKey();
	if (key != NO_KEY){
		////Serial.print("GamePad State = ");
		gamepad_st = (Gamepad_st)key;
		////Serial.println(gamepad_st);
	}
	direction_st = (Direction_st)sd[direction_st][gamepad_st].d;
	direction.x = sd[direction_st][gamepad_st].x;
	direction.y = sd[direction_st][gamepad_st].y;

	
}








void loop(){

	if(ct % 4000 == 0){
		Main_Thread();
		
	}
	GetKey_Thread(ct);
	
	ct++;
	
}

