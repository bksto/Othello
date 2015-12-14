/*	Name & E-mail: Brittany Seto (bseto001@ucr.edu)
 *	Lab Section: 22
 *	Assignment: Lab #10  Mini-project
 *	Exercise Description: Othello
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */

#include <avr/io.h>
#include "io.c" // Our io.c file from lab 5
#include <avr/pgmspace.h> // Built in avr library
#include <avr/interrupt.h>
#include <usart.h>
//#include <usart_ATmega1284.h>

unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
	return (b ? x | (0x01 << k) : x & ~(0x01 << k));
}
unsigned char GetBit(unsigned char x, unsigned char k) {
	return ((x & (0x01 << k)) != 0);
}

//========= TIMER ===============

void sleep(unsigned int x)
{
	1000*x;
}

volatile unsigned char TimerFlag = 0; 

unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;

void TimerOn() {
	TCCR1B = 0x0B;
	OCR1A = 125;	
	TIMSK1 = 0x02; 
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	SREG |= 0x80; 
}

void TimerOff() {
	TCCR1B = 0x00; 
}

typedef struct task{
	int state;
	unsigned long period;
	unsigned long elapsedTime;
	int (*TickFct)(int);
}task;

task tasks[3];
const unsigned short tasksNum = 3;
const unsigned long tasksPeriodGCD = 50;
const unsigned long periodLCDscreen = 500;
const unsigned long periodDetectSelect = 100;
const unsigned long periodReceive = 250; 

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

ISR(TIMER1_COMPA_vect) {
	_avr_timer_cntcurr--; 
	if (_avr_timer_cntcurr == 0) { 
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

//==========special characters () and filled ()===========

const uint8_t circle_LeftHalf[] PROGMEM = {
	0b00011,
	0b00100,
	0b01000,
	0b10000,
	0b10000,
	0b01000,
	0b00100,
	0b00011
}; 
const uint8_t circle_RightHalf[] PROGMEM = {
	0b11000,
	0b00100,
	0b00010,
	0b00001,
	0b00001,
	0b00010,
	0b00100,
	0b11000
};

const uint8_t filledCircle_LeftHalf[] PROGMEM = {
	0b00011,
	0b00111,
	0b01111,
	0b11111,
	0b11111,
	0b01111,
	0b00111,
	0b00011
};
const uint8_t filledCircle_RightHalf[] PROGMEM = {
	0b11000,
	0b11100,
	0b11110,
	0b11111,
	0b11111,
	0b11110,
	0b11100,
	0b11000
};

const uint8_t face_LeftHalf[] PROGMEM = {
	0b00011,
	0b00100,
	0b01010,
	0b10000,
	0b10100,
	0b01011,
	0b00100,
	0b00011
};
const uint8_t face_RightHalf[] PROGMEM = {
	0b11000,
	0b00100,
	0b01010,
	0b00001,
	0b00101,
	0b11010,
	0b00100,
	0b11000
};


//==========define characters==============
void LCDdefinechar(const uint8_t *pc,uint8_t char_code){
	uint8_t a, pcc;
	uint16_t i;
	a=(char_code<<3)|0x40;
	for (i=0; i<8; i++){
		pcc=pgm_read_byte(&pc[i]);
		LCD_WriteCommand(a++);
		LCD_WriteData(pcc);
	}
}



//==========score helper function to display >10 scores==========
void display_score(unsigned char score, unsigned char cursor)
{
	unsigned char num_mods = 0;
	LCD_Cursor(cursor);
	//score = score - 208; 
// 	if(score < 10)
// 	{
// 		LCD_WriteData(score + '0');
// 	}
// 	else if(score >= 10)
// 	{
// 		while(score >= 10)
// 		{
// 			score = score - 10;
// 			num_mods++;
// 		}
// 		LCD_WriteData(num_mods + '0');
// 		
// 		LCD_Cursor(cursor+1);
// 		LCD_WriteData(score + '0');
// 	}
	LCD_WriteData(score-208); 
}

//==========variables==============
unsigned int p1_score = 2;
unsigned int p2_score = 2; 

//================================================ USART ============================================================
enum receive_score_sm {init_receive, has_received, receive, flush } receive_score_state;
	
void receive_score() {	
	switch(receive_score_state) 
	{
		case -1: 
			receive_score_state = init_receive; 
			break; 
		case init_receive: 
			receive_score_state = has_received; 
			break; 
		case has_received:
			if(USART_HasReceived(0)) 
			{
				receive_score_state = receive;
			} 
			else if(!(USART_HasReceived(0)))
			{
				receive_score_state = has_received;
			}
			break;
		case receive:
			receive_score_state = flush;
			break;
		case flush:
			receive_score_state = has_received;;
			break;
		default:
			receive_score_state = has_received;
			break;
	}
	switch(receive_score_state) {
		case has_received:
			break;
		case receive:
			p1_score = USART_Receive(0); 
			break;
		case flush:
			USART_Flush(0);
			break;
		default:
			break;
	}
}



//==========screens=============== 
void title_screen()
{
	 LCD_init();
	 
	 LCD_DisplayString(24, "OTHELLO");
	 
	 LCDdefinechar(circle_LeftHalf,1);
	 LCD_Cursor(3);
	 LCD_WriteData(0b00000001);
	 
	 LCDdefinechar(circle_RightHalf,2);
	 LCD_Cursor(4);
	 LCD_WriteData(0b00000010);
	 
	 LCDdefinechar(filledCircle_LeftHalf,3);
	 LCD_Cursor(5);
	 LCD_WriteData(0b00000011);
	 
	 LCDdefinechar(filledCircle_RightHalf,4);
	 LCD_Cursor(6);
	 LCD_WriteData(0b00000100);
	 
	 LCD_Cursor(19);
	 LCD_WriteData(0b00000011);
	 
	 LCD_Cursor(20);
	 LCD_WriteData(0b00000100);
	 
	 LCD_Cursor(21);
	 LCD_WriteData(0b00000001);
	 
	 LCD_Cursor(22);
	 LCD_WriteData(0b00000010);
	 
	 LCDdefinechar(face_LeftHalf,5);
	 
	 LCDdefinechar(face_RightHalf,6);

	 LCD_Cursor(32);
}

void start_screen()
{		
	LCD_DisplayString(2, "PRESS (START)    TO BEGIN");		
}

void score_screen()
{
	LCD_DisplayString(2, "--P1--  --P2--"); 
	
	display_score(p1_score, 20);
	display_score(p2_score, 28);
}


unsigned char board_full()
{
	if((p1_score + p2_score) == 36)
	{
		return 1;
	}
	return 0;
}

void winner_screen()
{
	if(p1_score > p2_score)
	{
		LCD_DisplayString(1, "PLAYER 1 WINS ");
	}
	else if(p2_score > p1_score)
	{
		LCD_DisplayString(1, "PLAYER 2 WINS ");
	}

	LCD_Cursor(15);
	LCD_WriteData(0b00000101);

	LCD_Cursor(16);
	LCD_WriteData(0b00000110);
}


unsigned char select; 
enum select_sm{init_select, wait_select, select_detected, select_detected_wait} select_state;

void detect_select(select_state)
{
	unsigned char tempC = ~PINC;
	
	//transitions
	switch(select_state)
	{
		case -1: 
			select_state = init_select; 
			break; 
		case init_select: 
			select_state = wait_select; 
			break; 
		case wait_select: 
			if(GetBit(tempC, 7))
			{
				select_state = select_detected; 
			}
			else if(!GetBit(tempC,7))
			{
				select_state = wait_select; 
			}
			break; 
		case select_detected: 
			select_state = select_detected_wait; 
			break; 
		case select_detected_wait: 
			if(select == 1)
			{
				select_state = select_detected_wait;
			}
			else
			{
				select_state = init_select;
				select = 0;
			}
			break;
		default: 
			select_state = init_select; 
			break; 
	}
	//state actions
	switch(select_state)	
	{
		case init_select: 
			select = 0; 
			break; 
		case wait_select: 
			break; 
		case select_detected: 
			select = 1; 
			break; 
		case select_detected_wait:
			break; 
		default: 
			break; 
	}	
}


//======= TITLE/START/SCORE SM==========

enum start_states {init, title, start, score_state, winner_state} start_state;

void lcd_screen(){
	
	unsigned char tempC = ~PINC; 
	unsigned char full = board_full(); 
	
	//transitions
	switch(start_state)
	{
		case init:
			start_state = title; 
			break; 
		case title:
			if(GetBit(tempC, 7))
			{
				start_state = score_state; 
			} 
			else if(!GetBit(tempC, 7))
			{
				start_state = start; 
			}
			break; 
		case start:
			if(GetBit(tempC, 7))
			{
				start_state = score_state;
			}
			else if(!GetBit(tempC, 7))
			{
				start_state = title;
			}
			break; 
		case score_state:
			if(!GetBit(tempC, 7) && full == 0)
			{
				start_state = score_state;
			}
			else if(full == 1)
			{
				start_state = winner_state;
			}
			else if(GetBit(tempC, 7))
			{
				start_state = init;
			}
			break;
		case winner_state: 
			if(!GetBit(tempC, 7))
			{
				start_state = winner_state;
			}
			else if(GetBit(tempC, 7))
			{
				start_state = init;
			}
			break;
		default: 
			start_state = init; 
			break; 
	}
	
	//state actions
	switch(start_state)
	{
		case init:
			p1_score = 0; 
			p2_score = 0; 
			break;
		case title:
			title_screen(); 
			break;
		case start:
			start_screen(); 
			break;
		case score_state:
			score_screen(); 
			break;
		case winner_state: 
			winner_screen(); 
			break; 
		default:
			break;
	}
}



int main(void)
{
	DDRA = 0xFF; PORTA = 0x00; // LCD control lines
	DDRB = 0xFF; PORTB = 0x00; // LCD data lines
	DDRC = 0x00; PORTC = 0xFF; 
	DDRD = 0x02; PORTD = 0xFD;
	
	initUSART(0); //initializes USART0
	USART_Flush(0); //empties the UDR of USART0
	
	unsigned char p = 0;
	tasks[p].state = -1;
	tasks[p].period = periodLCDscreen;
	tasks[p].elapsedTime = tasks[p].period;
	tasks[p].TickFct = &lcd_screen;
	++p;
	tasks[p].state = -1;
	tasks[p].period = periodDetectSelect;
	tasks[p].elapsedTime = tasks[p].period;
	tasks[p].TickFct = &detect_select;
	++p; 
	tasks[p].state = -1;
	tasks[p].period = periodReceive;
	tasks[p].elapsedTime = tasks[p].period;
	tasks[p].TickFct = &receive_score;
	
	TimerSet(tasksPeriodGCD);
	TimerOn();

	while(1)
	{
		sleep(1);
	}
	return 0;
	
}