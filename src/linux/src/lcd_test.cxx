/*
 * Author: Yevgeniy Kiveisha <yevgeniy.kiveisha@intel.com>
 * Copyright (c) 2014 Intel Corporation.
 */

#include <unistd.h>
#include <iostream>
#include <vector>
#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <string>

#include "pcd8544.h"

using namespace std;

uint8_t leaf[LCDWIDTH * LCDHEIGHT / 8] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFC, 0xFE, 0xFF, 0xFC, 0xE0,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
0xF8, 0xF0, 0xF0, 0xE0, 0xE0, 0xC0, 0x80, 0xC0, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x3F, 0x7F,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE7, 0xC7, 0xC7, 0x87, 0x8F, 0x9F, 0x9F, 0xFF, 0xFF, 0xFF,
0xC1, 0xC0, 0xE0, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFC, 0xFC, 0xFC, 0xFE, 0xFE, 0xFE,
0xFC, 0xFC, 0xF8, 0xF8, 0xF0, 0xE0, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x80, 0xC0, 0xE0, 0xF1, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x1F, 0x0F, 0x0F, 0x87,
0xE7, 0xFF, 0xFF, 0xFF, 0x1F, 0x1F, 0x3F, 0xF9, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xFD, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x3F, 0x0F, 0x07, 0x01, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0xF0, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
0x7E, 0x3F, 0x3F, 0x0F, 0x1F, 0xFF, 0xFF, 0xFF, 0xFC, 0xF0, 0xE0, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFC, 0xF0, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01,
0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0F, 0x1F, 0x3F, 0x7F, 0x7F,
0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x7F, 0x1F, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

void testdrawline ( void );
void testdrawtriangle( void );
void testdrawcircle( void );
void testfillrect( void );

PCD8544* lcd = NULL;

int
main (int argc, char **argv)
{	
	bool running = true;
	lcd = new PCD8544 (8, 23, 24);
	
	lcd->clearscr ();
	testdrawline ();
	usleep (2000000);
	
	lcd->clearscr ();
	testdrawtriangle ();
	usleep (2000000);
	
	lcd->clearscr ();
	testfillrect ();
	usleep (2000000);
	
	lcd->clearscr ();
	testdrawcircle ();
	usleep (2000000);
	
	lcd->clearscr ();
	lcd->setCursor(1, 1);
    lcd->setTextSize (1);
	lcd->setTextColor(BLACK, WHITE);
    lcd->print("small");
	lcd->refresh ();
	usleep (2000000);
	
	// lcd->clearscr ();
	lcd->setCursor(1, 10);
    lcd->setTextSize (3);
	lcd->setTextColor(BLACK, WHITE);
    lcd->print("BIG");
	lcd->refresh ();
	usleep (2000000);
	
    /* The Big Loop */
    while (running) {
		lcd->drawFromBuffer (leaf);
		usleep (2000000);
		lcd->clearscr ();
		usleep (2000000);
    }
	
	delete lcd;
}

void
testdrawtriangle ( void ) {
	for (int16_t i=0; i<min(lcd->m_width,lcd->m_height)/2; i+=5) {
		lcd->drawTriangle(lcd->m_width/2, lcd->m_height/2-i,
						 lcd->m_width/2-i, lcd->m_height/2+i,
						 lcd->m_width/2+i, lcd->m_height/2+i, BLACK);
		lcd->refresh();
	}
}

void
testfillrect ( void ) {
	uint8_t color = 1;
	for (int16_t i=0; i<lcd->m_height/2; i+=3) {
		// alternate colors
		lcd->fillRect(i, i, lcd->m_width-i*2, lcd->m_height-i*2, color%2);
		lcd->refresh();
		color++;
	}
}

void
testdrawcircle ( void ) {
	for (int16_t i=0; i<lcd->m_height; i+=2) {
		lcd->drawCircle(lcd->m_width/2, lcd->m_height/2, i, BLACK);
		lcd->refresh();
	}
}

void
testdrawline ( void ) {  
	for (int16_t i=0; i<lcd->m_width; i+=4) {
		lcd->drawLine(0, 0, i, lcd->m_height-1, BLACK);
		lcd->refresh();
	}
	for (int16_t i=0; i<lcd->m_height; i+=4) {
		lcd->drawLine(0, 0, lcd->m_width-1, i, BLACK);
		lcd->refresh();
	}
	usleep(250000);

	lcd->clearscr();
	for (int16_t i=0; i<lcd->m_width; i+=4) {
		lcd->drawLine(0, lcd->m_height-1, i, 0, BLACK);
		lcd->refresh();
	}
	for (int8_t i=lcd->m_height-1; i>=0; i-=4) {
		lcd->drawLine(0, lcd->m_height-1, lcd->m_width-1, i, BLACK);
		lcd->refresh();
	}
	usleep(250000);

	lcd->clearscr();
	for (int16_t i=lcd->m_width-1; i>=0; i-=4) {
		lcd->drawLine(lcd->m_width-1, lcd->m_height-1, i, 0, BLACK);
		lcd->refresh();
	}
	for (int16_t i=lcd->m_height-1; i>=0; i-=4) {
		lcd->drawLine(lcd->m_width-1, lcd->m_height-1, 0, i, BLACK);
		lcd->refresh();
	}
	usleep(250000);

	lcd->clearscr();
	for (int16_t i=0; i<lcd->m_height; i+=4) {
		lcd->drawLine(lcd->m_width-1, 0, 0, i, BLACK);
		lcd->refresh();
	}
	for (int16_t i=0; i<lcd->m_width; i+=4) {
		lcd->drawLine(lcd->m_width-1, 0, i, lcd->m_height-1, BLACK); 
		lcd->refresh();
	}
	usleep(250000);
}
