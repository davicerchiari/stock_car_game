#include <reg51.h>
#include <intrins.h>
#include <stdlib.h>
#include <font_header.h>

#define Data_Port P3

sbit RS = P2^0;																	
sbit RW = P2^1;
sbit E = P2^2;
sbit CS1 = P2^3;
sbit CS2 = P2^4;
sbit RST = P2^5;

sbit COLUNA2 = P0^6;
sbit COLUNA0 = P0^4;
sbit LINHA1  = P0^1;

#define MAX_OBSTACLES 2
#define OBSTACLE_WIDTH 6
#define CAR_WIDTH 6
#define CAR_HEIGHT 1
#define LEFT_TRACK_BASE 10
#define RIGHT_TRACK_BASE 118
#define TRACK_WIDTH (RIGHT_TRACK_BASE - LEFT_TRACK_BASE)
#define OBSTACLE_GEN_DELAY 50
#define OBSTACLE_SPEED 7 

typedef struct {
    unsigned char x;
    unsigned char page;
    unsigned char active;
		unsigned char speed_counter;
} Obstacle;

Obstacle obstacles[2];
unsigned char obstacle_timer = 15;

char colisao = 0;
char empty_bitmap[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

char car_bitmap[6] = {0xC3, 0xFF, 0x66, 0x66, 0xFF, 0xC3};

char enemy_car[6] = {0xC3, 0xC3, 0x5A, 0x5A, 0xC3, 0xC3};

void delay(unsigned int k) {
    unsigned int i, j;
    for (i = 0; i < k; i++)
        for (j = 0; j < 112; j++);
}

void GLCD_Command(char Command) {
    Data_Port = Command;
    RS = 0; RW = 0; E = 1; _nop_(); E = 0; _nop_();
}

void GLCD_Data(char Data) {
    Data_Port = Data;
    RS = 1; RW = 0; E = 1; _nop_(); E = 0; _nop_();
}

void GLCD_Init()
{
	CS1 = 1; CS2 = 1;														
	RST = 1;																	
	delay(20);
	GLCD_Command(0x3E);
	GLCD_Command(0x40);
	GLCD_Command(0xB8);
	GLCD_Command(0xC0);
	GLCD_Command(0x3F);
}

void GLCD_ClearAll() {
    char page, col;
    for (page = 0; page < 8; page++) {
        CS1 = 0; CS2 = 1; GLCD_Command(0xB8 + page); GLCD_Command(0x40);
        for (col = 0; col < 64; col++) GLCD_Data(0x00);
        CS1 = 1; CS2 = 0; GLCD_Command(0xB8 + page); GLCD_Command(0x40);
        for (col = 0; col < 64; col++) GLCD_Data(0x00);
    }
    CS1 = 1; CS2 = 1; GLCD_Command(0xB8); GLCD_Command(0x40);
}

char ScanKey() {
    LINHA1 = 0;
    _nop_(); _nop_();
    
    if (!COLUNA0) {
        delay(20);
        if (!COLUNA0) return '4';
    }
    if (!COLUNA2) {
        delay(20);
        if (!COLUNA2) return '6';
    }
    return 0;
}

void GLCD_DisplayBitmap(const unsigned char *bitmap, int width, int page, int col) {
    char i;
    char new_col = col;
    
    char threshold = 3; 
    
    if (col >= (64 - width + threshold) && col < 64) {
        new_col = 64;
    }
    else if (col >= 64 && col < (64 + threshold)) {
        new_col = 64 - width;
    }
    
    if (new_col + width <= 64) {
        CS1 = 1; CS2 = 0; 
        GLCD_Command(0xB8 + page);
        GLCD_Command(0x40 + new_col);
        
        for(i = 0; i < width; i++) {
            GLCD_Data(bitmap[i]);
        }
    }
    else if (new_col >= 64) {
        CS1 = 0; CS2 = 1; 
        GLCD_Command(0xB8 + page);
        GLCD_Command(0x40 + (new_col - 64));
        
        for(i = 0; i < width; i++) {
            GLCD_Data(bitmap[i]);
        }
    }
    
    CS1 = 1; CS2 = 1;
}

const char track_offsets[8] = {0,0,0,0,0,0,0,0};

#define FIRST_OBSTACLE_PAGE 1 
#define LAST_PAGE 7        
#define LEFT_TRACK_START 32 
#define LEFT_TRACK_END 38      
#define RIGHT_TRACK_START 88  
#define RIGHT_TRACK_END 94     
#define TRACK_INNER_LEFT 38   
#define TRACK_INNER_RIGHT 88   
#define TRACK_HEIGHT 7        

void Generate_Obstacles() {
    unsigned char i;
    
    if (--obstacle_timer == 0) {
        obstacle_timer = OBSTACLE_GEN_DELAY + (rand() % 15);
        
        for (i = 0; i < MAX_OBSTACLES; i++) {
            if (!obstacles[i].active) {
                obstacles[i].x = TRACK_INNER_LEFT + 2 + (rand() % (TRACK_INNER_RIGHT - TRACK_INNER_LEFT - OBSTACLE_WIDTH - 4));
                
                if (obstacles[i].x < TRACK_INNER_LEFT + 2) {
                    obstacles[i].x = TRACK_INNER_LEFT + 2;
                }
                if (obstacles[i].x + OBSTACLE_WIDTH > TRACK_INNER_RIGHT - 2) {
                    obstacles[i].x = TRACK_INNER_RIGHT - OBSTACLE_WIDTH - 2;
                }
                
                obstacles[i].page = 1;
                obstacles[i].active = 1;
                obstacles[i].speed_counter = 0;
                break;
            }
        }
    }
}

void Update_Obstacles() {
    unsigned char i;
    
    for (i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            obstacles[i].speed_counter++;
            
            if (obstacles[i].speed_counter >= OBSTACLE_SPEED) {
                obstacles[i].speed_counter = 0;
                
                GLCD_DisplayBitmap(empty_bitmap, OBSTACLE_WIDTH, obstacles[i].page, obstacles[i].x);
                
                obstacles[i].page++;
                
                if (obstacles[i].page <= LAST_PAGE) {
                    GLCD_DisplayBitmap(enemy_car, OBSTACLE_WIDTH, obstacles[i].page, obstacles[i].x);
                } else {
                    obstacles[i].active = 0;
                }
            }
        }
    }
}

#define TRACK_SPEED 25
#define LEFT_TRACK_COL 32 
#define RIGHT_TRACK_COL 88

typedef struct {
    char page;                 
    unsigned char active;      
} TrackSegment;

TrackSegment track_segments[TRACK_HEIGHT];
unsigned char track_timer = 0;

const unsigned char track_bitmap[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

void Initialize_Track() {
		char i;
    for(i = 0; i < TRACK_HEIGHT; i++) {
        track_segments[i].page = i + 1; 
				track_segments[i].active = 1;
    }
}

#define MAX_TRACK_SHIFT 10

char left_track_offset = 0; 
char right_track_offset = 0;
char target_left_offset = 0;
char target_right_offset = 0; 

void Update_Track() {
    char last_top_page = -1;
    char i = 0;
    
    if(++track_timer >= TRACK_SPEED) {
        track_timer = 0;
        
        for(i = 0; i < TRACK_HEIGHT; i++) {
            if(track_segments[i].active) {
                GLCD_DisplayBitmap(empty_bitmap, 1, track_segments[i].page, 
                                 LEFT_TRACK_COL + left_track_offset);
                GLCD_DisplayBitmap(empty_bitmap, 1, track_segments[i].page, 
                                 RIGHT_TRACK_COL + right_track_offset);
            }
        }
        
        for(i = 0; i < TRACK_HEIGHT; i++) {
            track_segments[i].page++;
            
            if(track_segments[i].page > 7) {
                track_segments[i].page = 1;
                
                target_left_offset = (rand() % (2*MAX_TRACK_SHIFT + 1)) - MAX_TRACK_SHIFT;
                target_right_offset = (rand() % (2*MAX_TRACK_SHIFT + 1)) - MAX_TRACK_SHIFT;
                
                if((RIGHT_TRACK_COL + target_right_offset) - 
                   (LEFT_TRACK_COL + target_left_offset) != 
                   (RIGHT_TRACK_COL - LEFT_TRACK_COL)) {
                    target_right_offset = target_left_offset;
                }
            }
        }
        
        if(left_track_offset < target_left_offset) left_track_offset++;
        else if(left_track_offset > target_left_offset) left_track_offset--;
        
        if(right_track_offset < target_right_offset) right_track_offset++;
        else if(right_track_offset > target_right_offset) right_track_offset--;
        
        for(i = 0; i < TRACK_HEIGHT; i++) {
            GLCD_DisplayBitmap(track_bitmap, 1, track_segments[i].page, 
                             LEFT_TRACK_COL + left_track_offset);
            GLCD_DisplayBitmap(track_bitmap, 1, track_segments[i].page, 
                             RIGHT_TRACK_COL + right_track_offset);
        }
    }
}

int check_collision(int car_left) {
    unsigned char i;
		char page;
    char current_left_wall, current_right_wall;
    char car_top_page = 7 - CAR_HEIGHT + 1;
    
    for(i = 0; i < MAX_OBSTACLES; i++) {
        if(obstacles[i].active) {
            if(obstacles[i].page >= car_top_page && obstacles[i].page <= 7) {
                if(!(obstacles[i].x + OBSTACLE_WIDTH < car_left || 
                     obstacles[i].x > car_left + CAR_WIDTH)) {
                    return 1;
                }
            }
        }
    }
    
    for(page = 7; page >= car_top_page; page--) {
        current_left_wall = LEFT_TRACK_COL + left_track_offset + 6; 
        current_right_wall = RIGHT_TRACK_COL + right_track_offset - 6;
        
        if((car_left < current_left_wall) || 
           (car_left + CAR_WIDTH > current_right_wall)) {
            return 1;
        }
        
        if(page < 7) {
            int next_left_wall = LEFT_TRACK_COL + left_track_offset + 6;
            int next_right_wall = RIGHT_TRACK_COL + right_track_offset - 6;
            
            if(abs(current_left_wall - next_left_wall) > 2 || 
               abs(current_right_wall - next_right_wall) > 2) {
                return 1; 
            }
        }
    }
    return 0; 
}

void main() {   
    char car_position = 63;
    const char car_width = 6;
    int tempo_pressionado = 0;
		char botao_pressionado = 0;
		char key_pressed = 0;
		char tecla;
		char col;
		int score = 0;

    GLCD_Init();
    GLCD_ClearAll();

		//TELA INICIAL
    while (1) {
        tecla = ScanKey();
        if (tecla == '4') {
            tempo_pressionado += 100;
            botao_pressionado = 1;
        } else {
            if (botao_pressionado) {
                break;
            }
        }
        
        delay(100);
    }
		
		srand(tempo_pressionado);
		Initialize_Track();
			
    GLCD_DisplayBitmap(car_bitmap, car_width, 7, car_position);
	       
    while(1) {
				score++;
        key_pressed = ScanKey();
        if(key_pressed != 0) {
            GLCD_DisplayBitmap(empty_bitmap, CAR_WIDTH, 7, car_position);
            
            if(key_pressed == '4' && car_position > TRACK_INNER_LEFT) {
                car_position -= 6;
            }
            else if(key_pressed == '6' && car_position < TRACK_INNER_RIGHT - CAR_WIDTH) {
                car_position += 6;
            }
            
            GLCD_DisplayBitmap(car_bitmap, CAR_WIDTH, 7, car_position);
            delay(50);
            while(ScanKey() != 0) delay(10);
        }
				if (score % 20 == 0){
						CS1 = 0; CS2 = 1;
						GLCD_Command(0xB8);
						GLCD_Command(0x40 + 45);
						for (col = 0; col < 5; col++)
								GLCD_Data(font[(score/10)/100][col]);
						for (col = 0; col < 5; col++)
								GLCD_Data(font[((score/10) % 100)/10][col]);
						for (col = 0; col < 5; col++)
								GLCD_Data(font[(((score/10) % 100)%10)][col]);
				}
				Update_Track();
        Generate_Obstacles();
        Update_Obstacles();
        
        if(check_collision(car_position)) {
						GLCD_ClearAll();
            while(1);
        }
        delay(30);
    }
}