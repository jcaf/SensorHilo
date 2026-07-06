/*
 * indicator.h
 *
 *  Created on: Aug 30, 2021
 *      Author: jcaf
 */

#ifndef INDICATOR_INDICATOR_H_
#define INDICATOR_INDICATOR_H_

struct _indicator
{
		int8_t sm0;
		uint16_t counter0;
		//
		uint16_t KOn_MAX;
		uint16_t KOff_MAX;
		//

		volatile unsigned char *Port8bits;
		int8_t pin;
};



//volatile extern struct _indicator indicator;

	void indicator_setPortPin(struct _indicator *indicator, volatile unsigned char *Port8bits, int8_t pin);

	#ifdef TIMEON_TIMEOFF_SEPARADOS
		void indicatorTimed_setKSysTickTimeOn_ms(struct _indicator *indicator, uint16_t KSysTickTimeOn_ms);//div by SYSTICK_MS
		void indicatorTimed_setKSysTickTimeOff_ms(struct _indicator *indicator, uint16_t KSysTickTimeOff_ms);//div by SYSTICK_MS
	#else

		void indicatorTimed_setKSysTickTime_ms(struct _indicator *indicator, uint16_t KSysTickTime_ms);//div by SYSTICK_MS
	#endif

	void indicatorTimed_run_only_once(struct _indicator *indicator );
	void indicatorTimed_stop(struct _indicator *indicator);
	void indicatorTimed_cycle_start(struct _indicator *indicator);
	void indicatorTimed_job(struct _indicator *indicator);

	void indicator_on(struct _indicator *indicator);
	void indicator_off(struct _indicator *indicator);

#endif /* INDICATOR_INDICATOR_H_ */
