/*	Name & E-mail: Brittany Seto (bseto001@ucr.edu)
 *	Lab Section: 22
 *	Assignment: Lab #10  Mini-project
 *	Exercise Description: Othello
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
//#include <usart_ATmega1284.h>
#include <usart.h>

void sleep(unsigned int x)
{
	1000*x; 
}


unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
	return (b ? x | (0x01 << k) : x & ~(0x01 << k));
}
unsigned char GetBit(unsigned char x, unsigned char k) {
	return ((x & (0x01 << k)) != 0);
}

// ====================
// TIMER STUFF
// ====================

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

typedef struct task{
	int state;
	unsigned long period;
	unsigned long elapsedTime;
	int (*TickFct)(int);
}task;

task tasks[5];
const unsigned short tasksNum = 5;
const unsigned long tasksPeriodGCD = 1;
const unsigned long periodDisplay = 1;
const unsigned long periodMove = 100;
const unsigned long periodChangePlayer = 100; 
const unsigned long periodStartReset = 200; 
const unsigned long periodSend = 100; 


void TimerISR()
{
	unsigned char p;
	for(p = 0; p < tasksNum; ++p)
	{
		if(tasks[p].elapsedTime >= tasks[p].period)
		{
			tasks[p].state = tasks[p].TickFct(tasks[p].state);
			tasks[p].elapsedTime = 0;
		}
		tasks[p].elapsedTime += tasksPeriodGCD;
	}
}


// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
	TimerISR(); // Call the ISR that the user uses
	_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

//=============== VARIABLES =================
unsigned char column_val = 0x01; //(UP/DOWN)
unsigned char column_sel = 0x01; 
unsigned char column_sel_p1 = ~(0x01); //(LEFT/RIGHT)
unsigned char column_sel_p2 = ~(0x01 << 1); 

unsigned char p1_score = 0;
unsigned char p2_score = 0;
unsigned char player = 1;
unsigned char select = 0;
unsigned char x = 0;
unsigned char y = 0;
unsigned char tempPlayer;
unsigned char reset=0;

//for display
unsigned int i = 0;
unsigned int j = 0;

//============ 2D array ===========
static unsigned char LED_matrix[8][8] = 
{ 
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,1,2,0,0,0},
	{0,0,0,2,1,0,0,0},
	{0,0,0,0,0,0,0,0},	
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0}
};


//====================== USART ==========================
void clear_board()
{
	for(unsigned char a = 0; a < 8; a++)
	{
		for(unsigned char b = 0; b < 8; b++)
		{
			LED_matrix[a][b] = 0; 
		}
	}
}

void initialize_board()
{
	LED_matrix[3][3] = 1;
	LED_matrix[3][4] = 2;
	LED_matrix[4][3] = 2;
	LED_matrix[4][4] = 1;
}

void end_screen()
{
	if(p1_score > p2_score)
	{
		for(unsigned char a = 0; a < 8; a++)
		{
			for(unsigned char b = 0; b < 8; b++)
			{
				LED_matrix[a][b] = 1;
			}
		}
	}
	else if(p2_score > p1_score)
	{
		for(unsigned char a = 0; a < 8; a++)
		{
			for(unsigned char b = 0; b < 8; b++)
			{
				LED_matrix[a][b] = 2;
			}
		}
	}
}


void getScore()
{
	for(unsigned char a = 0; a < 8; a++)
	{
		for(unsigned char b = 0; b < 8; b++)
		{
			if(LED_matrix[a][b] == 1)
			{
				p1_score++;
			}
			else if(LED_matrix[a][b] == 2)
			{
				p2_score++;
			}
		}
	} 
}

unsigned char board_full()
{
	if((p1_score + p2_score) == 36)
	{
		return 1; 
	}
	return 0; 
}

unsigned char getOtherPlayer(unsigned char prev_player)
{
	if(player == 1)
	{
		return 2; 
	}
	else if(player == 2)
	{
		return 1; 
	}
}

//============================= FLIP FUNCTIONS ===============================
//if there is same player's piece after otherPlayer's

unsigned char right_isValid = 0;
unsigned char left_isValid = 0;
unsigned char down_isValid = 0;
unsigned char up_isValid = 0;
unsigned char NE_isValid = 0;
unsigned char SE_isValid = 0;
unsigned char SW_isValid = 0;
unsigned char NW_isValid = 0;

void valid_flip()
{
	//check right
	if(LED_matrix[x][y-1] == getOtherPlayer(player))
	{
		for(unsigned char b = y-1; b > 0; b--)
		{
			if(LED_matrix[x][b] == player)
			{
				right_isValid = 1;
			}
		}
	}
	//check left
	if(LED_matrix[x][y+1] == getOtherPlayer(player))
	{
		for(unsigned char b = y+1; b < 7; b++)
		{
			if(LED_matrix[x][b] == player)
			{
				left_isValid = 1;
			}
		}
	}
	//check down
	if(LED_matrix[x-1][y] == getOtherPlayer(player))
	{
		for(unsigned char b = x-1; b > 0; b--)
		{
			if(LED_matrix[b][y] == player)
			{
				down_isValid = 1;
			}
		}
	}
	//check up
	if(LED_matrix[x+1][y] == getOtherPlayer(player))
	{
		for(unsigned char b = x+1; b < 7; b++)
		{
			if(LED_matrix[b][y] == player)
			{
				up_isValid = 1;
			}
		}
	}
	//check SW 
	if(LED_matrix[x-1][y+1] == getOtherPlayer(player))
	{
		unsigned char a = x;
		unsigned char b = y; 
		
		while((a!=0) && (b!=7))
		{
			a--; 
			b++; 
			if(LED_matrix[a][b] == player)
			{
				SW_isValid = 1; 
			}
		}
	}
	
	//check NE
	if(LED_matrix[x+1][y-1] == getOtherPlayer(player))
	{
		unsigned char a = x; 
		unsigned char b = y; 
		
		while((a != 7) && (b != 0))
		{
			a++; 
			b--; 
			if(LED_matrix[a][b] == player)
			{
				NE_isValid = 1; 
				break; 
			}
		}
	}
	
	//check NW
	if(LED_matrix[x+1][y+1] == getOtherPlayer(player))
	{
		unsigned char a = x; 
		unsigned char b = y; 
		
		while((a!=7) && (b != 7))
		{
			a++; 
			b++; 
			if(LED_matrix[a][b] == player)
			{
				NW_isValid = 1; 
				break; 
			}
		}
	}
	
	//check SE
	if(LED_matrix[x-1][y-1] == getOtherPlayer(player))
	{
		unsigned char a = x; 
		unsigned char b = y; 
		
		while((a != 0) && (y != 0))
		{
			a--;
			b--; 
			if(LED_matrix[a][b] == player)
			{
				SE_isValid = 1; 
				break;
			}
		}
	}
}

//========= flip functions ==========
void flip(){
	
	valid_flip();
	unsigned char tempX = x; 
	unsigned char tempY = y;
	
	//check right
	if(right_isValid == 1)
	{
		while(LED_matrix[x][tempY-1] != player)
		{
			tempY--;
			LED_matrix[x][tempY] = player;
		}
		right_isValid = 0; 
		tempY = y; 
	}
	//check left
	if(left_isValid == 1)
	{
		while(LED_matrix[x][tempY+1] != player)
		{
			tempY++; 
			LED_matrix[x][tempY] = player;
		}
		left_isValid = 0;
		tempY = y; 
	}
	//check down
	if(down_isValid == 1)
	{
		while(LED_matrix[tempX-1][y] != player)
		{
			tempX--;
			LED_matrix[tempX][y] = player;
		}
		down_isValid = 0;
		tempX = x; 
	}
	
	//check up
	if(up_isValid == 1)
	{
		while(LED_matrix[tempX+1][y] != player)
		{
			tempX++;
			LED_matrix[tempX][y] = player;
		}
		up_isValid = 0;
		tempX = x; 
	} 
	
	//check SW
	if(SW_isValid == 1)
	{
		while(LED_matrix[tempX-1][tempY+1] != player)
		{
			tempX--; 
			tempY++; 
			LED_matrix[tempX][tempY] = player; 
		}
		SW_isValid = 0; 
		tempX = x; 
		tempY = y; 
	}
	
	//check NE
	if(NE_isValid == 1)
	{
		while(LED_matrix[tempX+1][tempY-1] != player)
		{
			tempX++; 
			tempY--; 
			LED_matrix[tempX][tempY] = player; 
		}
		NE_isValid = 0; 
		tempX = x; 
		tempY = y; 
	}
	
	//check NW
	if(NW_isValid == 1)
	{
		while(LED_matrix[tempX+1][tempY+1] != player)
		{
			tempX++;
			tempY++;
			LED_matrix[tempX][tempY] = player;
		}
		NW_isValid = 0;
		tempX = x;
		tempY = y;
	}
	
	//check SE
	if(SE_isValid == 1)
	{
		while(LED_matrix[tempX-1][tempY-1] != player)
		{
			tempX--;
			tempY--;
			LED_matrix[tempX][tempY] = player;
		}
		SE_isValid = 0;
		tempX = x;
		tempY = y;
	}
	getScore(); 
}


//============ CHANGE PLAYERS/FLIP ====================
enum changePlayer_sm{init, wait_select, change_players, flip_state} change_player;

void change_player_sm()
{
	unsigned char tempD = ~PIND; 
	 
	switch(change_player)
	{
		case -1: 
			change_player = init; 
			break;
		case init: 
		    player = 1; 
			change_player = wait_select; 
			break; 
		case wait_select: 
			if(GetBit(tempD, 3) && ((x!=0) && (x!=7) && (y!=0) && (y!=7)))
			{
				LED_matrix[x][y] = player; 
				change_player = flip_state;
				select = 1; 
			}
			else if (!(GetBit(tempD, 3) && ((x!=0) && (x!=7) && (y!=0) && (y!=7)))){
				change_player = wait_select;
			}
			break;
		case flip_state:
			change_player = change_players;
			
			break;
		case change_players: 
			change_player = wait_select; 
			break; 
		default:
			change_player = init; 
			break;
	}
	
	switch(change_player)
	{
		case init: 
			break; 
		case wait_select: 
			break; 		
		case flip_state:  
			flip(); 
			x = 0;
			y = 0;
			break; 
		case change_players: 
			player = getOtherPlayer(player); 
			break;
		default: 
			break; 		
	}
}
 
enum move_states{init_move, wait_move, moveUp, moveLeft, moveDown, moveRight} move_state;
	
void move()
{
	 unsigned char tempD = ~PIND; 
	 LED_matrix[x][y] = player; 
	 
	 switch(move_state) 
	 {
		 case -1: 
			move_state = init_move; 
			break; 
		 case init_move: 
			move_state = wait_move;  
			break; 
		 case wait_move: 
			if(GetBit(tempD, 6))
			{
				move_state = moveUp; 
			}
			else if(GetBit(tempD, 7))
			{	
				move_state = moveLeft; 
			}		
			else if(GetBit(tempD, 5))
			{
				move_state = moveDown; 
			}				
			else if(GetBit(tempD, 4))
			{
				move_state = moveRight; 
			}		
			else 
			{
				move_state = wait_move; 
			}	
			break;
		case moveUp: 
			move_state = wait_move; 
			break; 
		case moveLeft: 
			move_state = wait_move;  
			break; 
		case moveDown: 
			move_state = wait_move; 
			break; 
		case moveRight: 
			move_state = wait_move;  
			break; 
		default: 
			move_state = init_move; 
			break;  
	 }
	 
	 switch(move_state)
	 {
		 case init_move: 
			break; 
		 case wait_move: 
			break; 
		 case moveUp:
			if(((x+1) < 8) && (LED_matrix[x+1][y] == 0)) 
			{ 
				x++; 
				if(select == 0)
				{
					LED_matrix[x-1][y] = 0;
				}	
				else if(select == 1)
				{
					x = 0; 
					y = 0; 
				}
				select = 0; 
			}			
			break; 
		 case moveLeft: 
 			if(((y+1) < 8) && (LED_matrix[x][y+1] == 0)) 
 			{ 
 				y++;  
				if(select == 0) 
				{
					LED_matrix[x][y-1] = 0;
				}
				else if(select == 1)
				{
					x = 0;
					y = 0; 
				}
				select = 0; 
			}
			break; 
		 case moveDown:
			if(((x-1) >= 0) && (LED_matrix[x-1][y] == 0))
			{ 
				x--;
				if(select == 0)
				{
					LED_matrix[x+1][y] = 0;
				}
				else if(select == 1)
				{
					x = 0;
					y = 0;
				}
				select = 0; 
			}
			break; 
		 case moveRight: 
			if(((y-1) >= 0) && (LED_matrix[x][y-1] == 0))
			{ 
				y--;
				if(select == 0)  
				{
					LED_matrix[x][y+1] = 0;
				}
				else if(select == 1)
				{
					x = 0;
					y = 0;
				}
				select = 0; 
			}			
			break; 
		 default: 
			break; 	 
	 }
}


//============== detect start/reset ==================
enum detect_startRestart_sm{init_reset, wait_reset, set_reset} startRestart_state;

void detect_startRestart()
{
	unsigned char tempD = ~PIND;
	
	switch(startRestart_state)
	{
		case -1: 
			startRestart_state = init_reset; 
			break; 
		case init_reset: 
			startRestart_state = wait_reset; 
			break; 
		case wait_reset: 
			if(GetBit(tempD,2))
			{
				startRestart_state = set_reset; 
			}
			else if(!GetBit(tempD, 2))
			{
				startRestart_state = wait_reset; 
			}
			break;
		case set_reset: 
			startRestart_state = init_reset; 
			break;
		default: 
			startRestart_state = init_reset;
			break;
	}
	
	switch(startRestart_state)
	{
		case init_reset: 
			reset = 0;  
			break; 
		case wait_reset: 
			break; 
		case set_reset: 
			reset = 1; 
			clear_board();
			initialize_board(); 
			p1_score = 0; 
			p2_score = 0; 
			x = 0; 
			y = 0; 
			break; 
		default: 
			break; 
	}
}


//=================== DISPLAY ========================
//display helper function
void display_Players(unsigned char i)
{
	column_sel_p1 = 0x00;
	column_sel_p2 = 0x00;
	for(unsigned char j = 0; j < 8; j++)
	{
		if(LED_matrix[i][j] == 1)
		{
			column_sel_p1 = (0x01 << j) | column_sel_p1;
		}
		else if(LED_matrix[i][j] == 2)
		{
			column_sel_p2 = (0x01 << j) | column_sel_p2;
		}
	}
	
}

enum display_matrix{init_display, display_row, display_column, display_p1, display_p2} display_state;


void display()
{
	getScore(); 
	switch(display_state)
	{
		case -1: 
			display_state = init_display; 
			break; 
		case init_display: 
			display_state = display_row; 
			break; 
		case display_row: 
			if(i < 8)
			{
				display_state = display_p1;
			}
			else if(!(i<8))
			{
				display_state = init_display; 
			}			
			break; 
		case display_p1:
			display_state = display_p2; 
			break; 
		case display_p2: 
			display_state = display_row;
			i++;  
			break; 
		default: 
			display_state = init_display; 
			break; 	
	}
	
	switch(display_state)
	{
		case init_display: 
			i = 0;  
			column_val = 0;
			column_sel_p1 = 0; 
			column_sel_p2 = 0; 
			break; 
		case display_row: 
			column_val = 0x01 << i; 
			display_Players(i);
			break; 	
		case display_p1: 
			PORTC = 0xFF;
			PORTA = column_val; // PORTA displays column pattern
			PORTB = ~column_sel_p1; // PORTB selects column to display pattern
			break;
		case display_p2:	
			PORTB = 0xFF; // PORTB selects column to display pattern
			PORTA = column_val; // PORTA displays column pattern
			PORTC = ~column_sel_p2;
			break; 
		default: 
			break;
	}
}
	
	
	
//================================= USART ==================================
enum send_score_sm{init_send, wait_send, send, wait_transmitted} send_score_state;

void send_score()
{
	 switch(send_score_state)
	 {
		case -1: 
			send_score_state = init_send; 
			break; 
		case init_send: 
			send_score_state = wait_send; 
			break; 
		case wait_send: 
			if(!(USART_IsSendReady(0)))
			{
				send_score_state = wait_send; 
			}
			else if(USART_IsSendReady(0))
			{
				send_score_state = send; 
			}			
			break; 
		case send: 
			send_score_state = wait_transmitted; 
			break;
		case wait_transmitted: 
			if(!(USART_HasTransmitted(0)))
			{
				send_score_state = wait_transmitted; 
			}
			else if(USART_HasTransmitted(0))
			{
				send_score_state = wait_send; 
			}
			break; 		
		default: 
			send_score_state = init_send; 
			break; 
	}
	switch(send_score_state)
	{
		case init_send: 
			break; 
		case wait_send: 
			break; 
		case send: 
			USART_Send(p1_score, 0);
			break; 
		case wait_transmitted: 
			break; 
		default:
			break;
	}	
	 
}

int main(void)
{
	DDRA = 0xFF; PORTA = 0x00;
	DDRB = 0xFF; PORTB = 0x00; 
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0x02; PORTD = 0xFD;
	
	initUSART(0); //initializes USART0
	USART_Flush(0); //empties the UDR of USART0
	
	unsigned char p = 0; 
	tasks[p].state = -1; 
	tasks[p].period = periodMove; 
	tasks[p].elapsedTime = tasks[p].period; 
	tasks[p].TickFct = &move; 
	++p;
	tasks[p].state = -1;
	tasks[p].period = periodChangePlayer;
	tasks[p].elapsedTime = tasks[p].period;
	tasks[p].TickFct = &change_player_sm;
	++p; 
	tasks[p].state = -1;
	tasks[p].period = periodDisplay;
	tasks[p].elapsedTime = tasks[p].period;
	tasks[p].TickFct = &display;
	++p; 
	tasks[p].state = -1;
	tasks[p].period = periodStartReset;
	tasks[p].elapsedTime = tasks[p].period;
	tasks[p].TickFct = &detect_startRestart;
	++p;
	tasks[p].state = -1;
	tasks[p].period = periodSend;
	tasks[p].elapsedTime = tasks[p].period;
	tasks[p].TickFct = &send_score;
	
	TimerSet(tasksPeriodGCD);
	TimerOn();

    while(1)
    {	  
		 sleep(1); 
    }
	return 0; 
}