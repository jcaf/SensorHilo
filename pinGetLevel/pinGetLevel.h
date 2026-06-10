/*
 * pinGetLevel.h
 *
 *  Created on: Dec 3, 2020
 *      Author: jcaf
 */

#ifndef PINGETLEVEL_PINGETLEVEL_H_
#define PINGETLEVEL_PINGETLEVEL_H_

#include "../main.h"


#define PINGETLEVEL_NUMMAX 6 //# of pines to check

struct _pinGetLevel
{
  	int8_t counterDebounce;
    struct _pinGetLevel_bf
    {
		unsigned sm0:1;
		unsigned level:1;
		unsigned level_last:1;
		unsigned changed:1;
		unsigned __a:4;
  	}bf;

    //
    PTRFX_retUINT8_T readPinLevel;
};

extern struct _pinGetLevel pinGetLevel[PINGETLEVEL_NUMMAX];

//
#define PINGETLEVEL_INITwCHANGED
void pinGetLevel_init();//by default always changed-flag = 1 at begin
//
void pinGetLevel_job(void);

#define pinGetLevel_hasChanged(i)pinGetLevel[i].bf.changed
#define pinGetLevel_setChange(i) do{pinGetLevel[i].bf.changed = 1;}while(0)
#define pinGetLevel_clearChange(i) do{pinGetLevel[i].bf.changed = 0;}while(0)
#define pinGetLevel_level(i) pinGetLevel[i].bf.level


//REMAPING DEFINITIOS PORTW/R
#define PORTWxGETLEVEL_0 	PORTWxSW_ANULAR
#define PORTRxGETLEVEL_0 	PORTRxSW_ANULAR
#define CONFIGIOxGETLEVEL_0 	CONFIGIOxSW_ANULAR
#define PINxGETLEVEL_0		PINxSW_ANULAR

#define PORTWxGETLEVEL_1 	PORTWxSW_STOP
#define PORTRxGETLEVEL_1 	PORTRxSW_STOP
#define CONFIGIOxGETLEVEL_1 	CONFIGIOxSW_STOP
#define PINxGETLEVEL_1		PINxSW_STOP

#define PORTWxGETLEVEL_2 	PORTWxSW_JOG
#define PORTRxGETLEVEL_2 	PORTRxSW_JOG
#define CONFIGIOxGETLEVEL_2 	CONFIGIOxSW_JOG
#define PINxGETLEVEL_2		PINxSW_JOG


#define PORTWxGETLEVEL_3    PORTWxSW_START	
#define PORTRxGETLEVEL_3 	PORTRxSW_START
#define CONFIGIOxGETLEVEL_3 CONFIGIOxSW_START
#define PINxGETLEVEL_3		PINxSW_START

#define PORTWxGETLEVEL_4 	PORTWxTEST_INIT
#define PORTRxGETLEVEL_4 	PORTRxTEST_INIT
#define CONFIGIOxGETLEVEL_4 CONFIGIOxTEST_INIT
#define PINxGETLEVEL_4		PINxTEST_INIT


#define PORTWxGETLEVEL_5 	PORTWx24VAC_PRESENTE
#define PORTRxGETLEVEL_5 	PORTRx24VAC_PRESENTE
#define CONFIGIOxGETLEVEL_5 CONFIGIOx24VAC_PRESENTE
#define PINxGETLEVEL_5		PINx24VAC_PRESENTE


//pinGetLevel layout
#define PGLEVEL_LYOUT_0 0
#define PGLEVEL_LYOUT_1 1
#define PGLEVEL_LYOUT_2 2
#define PGLEVEL_LYOUT_3 3
#define PGLEVEL_LYOUT_4 4
#define PGLEVEL_LYOUT_5 5


#endif /* PINGETLEVEL_PINGETLEVEL_H_ */
