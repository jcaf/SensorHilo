/*
 * pinGetLevel.c
 *
 *  Created on: Dec 3, 2020
 *      Author: jcaf
 */
#include "../system.h"
#include "../types.h"
#include "pinGetLevel.h"

#define PINGETLEVEL_PERIODIC_ACCESS 20//msE-3		//aplication-level

#define PINGETLEVEL_SCAN_DEBOUNCE_MIN 40//ms 	//added 2020
//#include <math.h>
//#define PINGETLEVEL_SCAN_KCOUNT_DEBOUNCE (int)( PINGETLEVEL_PERIODIC_ACCESS < PINGETLEVEL_SCAN_DEBOUNCE_MIN ? ceil((PINGETLEVEL_SCAN_DEBOUNCE_MIN*1.0f)/PINGETLEVEL_PERIODIC_ACCESS): 1)
#define PINGETLEVEL_SCAN_KCOUNT_DEBOUNCE (int)( PINGETLEVEL_PERIODIC_ACCESS < PINGETLEVEL_SCAN_DEBOUNCE_MIN ? CEIL_INTEGERS(PINGETLEVEL_SCAN_DEBOUNCE_MIN, PINGETLEVEL_PERIODIC_ACCESS): 1)


struct _pinGetLevel pinGetLevel[PINGETLEVEL_NUMMAX];


static uint8_t pinGetLevel_0(void)
{
	return ReadPin(PORTRxGETLEVEL_0, PINxGETLEVEL_0);
}
static uint8_t pinGetLevel_1(void)
{
	return ReadPin(PORTRxGETLEVEL_1, PINxGETLEVEL_1);
}
static uint8_t pinGetLevel_2(void)
{
	return ReadPin(PORTRxGETLEVEL_2, PINxGETLEVEL_2);
}
static uint8_t pinGetLevel_3(void)
{
	return ReadPin(PORTRxGETLEVEL_3, PINxGETLEVEL_3);
}
static uint8_t pinGetLevel_4(void)
{
	return ReadPin(PORTRxGETLEVEL_4, PINxGETLEVEL_4);
}


void pinGetLevel_init(void)
{
	PinTo1(PORTWxGETLEVEL_0, PINxGETLEVEL_0);//Pull-up
	PinTo1(PORTWxGETLEVEL_1, PINxGETLEVEL_1);//Pull-up
        PinTo1(PORTWxGETLEVEL_2, PINxGETLEVEL_2);//Pull-up
        PinTo1(PORTWxGETLEVEL_1, PINxGETLEVEL_3);//Pull-up
        PinTo1(PORTWxGETLEVEL_2, PINxGETLEVEL_4);//Pull-up
        
        __delay_ms(1);
        
        ConfigInputPin(CONFIGIOxGETLEVEL_0, PINxGETLEVEL_0);
	pinGetLevel[0].readPinLevel = pinGetLevel_0;

        
	ConfigInputPin(CONFIGIOxGETLEVEL_1, PINxGETLEVEL_1);
	pinGetLevel[1].readPinLevel = pinGetLevel_1;

        
	ConfigInputPin(CONFIGIOxGETLEVEL_2, PINxGETLEVEL_2);
	pinGetLevel[2].readPinLevel = pinGetLevel_2;

        
        ConfigInputPin(CONFIGIOxGETLEVEL_3, PINxGETLEVEL_3);
	pinGetLevel[3].readPinLevel = pinGetLevel_3;

        
	ConfigInputPin(CONFIGIOxGETLEVEL_4, PINxGETLEVEL_4);
	pinGetLevel[4].readPinLevel = pinGetLevel_4;
        
	__delay_ms(1);
	//Set initial level
	for (int i=0; i<PINGETLEVEL_NUMMAX; i++)
	{
		pinGetLevel[i].bf.level = pinGetLevel[i].readPinLevel();
		pinGetLevel[i].bf.level_last = pinGetLevel[i].bf.level;

		#ifdef PINGETLEVEL_INITwCHANGED
		pinGetLevel[i].bf.changed = 1;//provocar un cambio inicial
		#endif
	}
}
/*
void pinGetLevel_job(void)//non-block
{
	int8_t level_temp;

	for (int8_t i = 0; i< PINGETLEVEL_NUMMAX; i++)
	{
		if  (pinGetLevel[i].sm0 == 0)
	    {
	        level_temp = pinGetLevel[i].readPinLevel();

	        if (pinGetLevel[i].level_last != level_temp)
	        {
	            pinGetLevel[i].level_last = level_temp;//save
	            pinGetLevel[i].sm0++;
	        }
	    }
	    else if (pinGetLevel[i].sm0 == 1)
	    {
            if (++pinGetLevel[i].counterDebounce == PINGETLEVEL_SCAN_KCOUNT_DEBOUNCE)//ms
            {
                pinGetLevel[i].counterDebounce = 0x0;

                level_temp = pinGetLevel[i].readPinLevel();

                if (pinGetLevel[i].level_last == level_temp)
                    {pinGetLevel[i].level = pinGetLevel[i].level_last;}

                pinGetLevel[i].sm0 = 0x00;
            }
	    }
	}
}
*/
void pinGetLevel_job(void)//non-block
{
	int8_t level_temp;

	for (int8_t i = 0; i< PINGETLEVEL_NUMMAX; i++)
	{
		if  (pinGetLevel[i].bf.sm0 == 0)
	    {
	        level_temp = pinGetLevel[i].readPinLevel();

	        if (pinGetLevel[i].bf.level_last != level_temp)
	        {
	            pinGetLevel[i].bf.level_last = level_temp;//save
	            pinGetLevel[i].bf.sm0 = 1;
	        }
	    }
	    else if (pinGetLevel[i].bf.sm0 == 1)
	    {
            if (++pinGetLevel[i].counterDebounce == PINGETLEVEL_SCAN_KCOUNT_DEBOUNCE)//ms
            {
                pinGetLevel[i].counterDebounce = 0x0;

                level_temp = pinGetLevel[i].readPinLevel();

                if (pinGetLevel[i].bf.level_last == level_temp)
                {
                	pinGetLevel[i].bf.level = level_temp;//pinGetLevel[i].bf.level_last;}
                	pinGetLevel[i].bf.changed = 1;//clear in app-level
                }
                else
                {
                	pinGetLevel[i].bf.level_last = level_temp;
                }

                pinGetLevel[i].bf.sm0 = 0;
            }
	    }
	}
}
/*
int8_t pinGetLevel_hasChanged(uint8_t i)
{
	return pinGetLevel[i].bf.changed;
}

void	pinGetLevel_clearChange(uint8_t i)
{
	pinGetLevel[i].bf.changed = 0;
}
int8_t pinGetLevel_level(uint8_t i)
{
	return pinGetLevel[i].bf.level;
}
*/
