/*  sensor de hilo
 * File:   main.c
 * Author: jcaf
 *
 * ATmega328P Clock interno @8Mhz, sin divisor, BOD = 2.7V
 * avrdude -c usbasp -B5 -p m328P -U lfuse:w:0xe2:m -U hfuse:w:0xd9:m -U efuse:w:0xfd:m
 * 
 * avrdude -c usbasp -p m328p -V -U flash:w:SensorHilo.X.production.hex
 * 
 * Created on May 22, 2026, 6:48 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "pinGetLevel/pinGetLevel.h"

volatile struct _isrflag isrflag;
struct _mainflag mainflag;
volatile static uint8_t PD_last;

enum _ESTADO_HILO
{
    UNKNOW = 0,
    HILO_OK = 1,
    HILO_ROTO = 2
};


struct busc_estado_hilo_t
{
    int8_t sm0;
    uint16_t counter_ticks;
   
}busc_estado_hilo[NUM_CANALES_SENSOR];

struct _canal
{
    uint8_t test_is_enabled;
    
    
    int8_t estado_hilo;
    int8_t nuevo_estado;
    
    uint16_t count_changelevel;
    PTRFX_retVOID pinchangelevel_enable;
    PTRFX_retVOID pinchangelevel_disable;
    PTRFX_retUINT8_T pinchangelevel_is_enable;
    
    //control relay de 24VAC
    struct _v24ac
    {
        PTRFX_retVOID on;
        PTRFX_retVOID off;
        uint16_t count_time_encendido;
        
        struct _v24ac_bf
        {
           unsigned timming:1;
           unsigned __a:7;
        }bf;
    }v24ac;
    
    //shortckt
    PTRFX_retUINT8_T fx_shortckt_read;
    
    union _test_sensorhilo_u_error
    {
        struct _test_sensorhilo_bf
        {
            unsigned shortckt:1;
            unsigned v24ac:1;
            unsigned __a:6;
        }bf;
        
        uint8_t error;
    }u_error;

};

volatile struct _canal canal[NUM_CANALES_SENSOR];

////////////////////////////////////////////////////////////////////////////////
uint8_t canal1_shortckt_read(void)
{
    return ReadPin(PORTRxTEST1_SHORTCKT, PINxTEST1_SHORTCKT);
}
uint8_t canal2_shortckt_read(void)
{
    return ReadPin(PORTRxTEST2_SHORTCKT, PINxTEST2_SHORTCKT);
}
uint8_t canal3_shortckt_read(void)
{
    return ReadPin(PORTRxTEST3_SHORTCKT, PINxTEST3_SHORTCKT);
}

void canal1_relay24VAC_ON(void)     
{
    PinTo1(PORTWxTEST1_CONTROL24VAC, PINxTEST1_CONTROL24VAC);
}
void canal2_relay24VAC_ON(void)     
{
    PinTo1(PORTWxTEST2_CONTROL24VAC, PINxTEST2_CONTROL24VAC);
}
void canal3_relay24VAC_ON(void)     
{
    PinTo1(PORTWxTEST3_CONTROL24VAC, PINxTEST3_CONTROL24VAC);
}

////////////////////////////////////////////////////////////////////////////////
void canal1_relay24VAC_OFF(void)     
{
    PinTo0(PORTWxTEST1_CONTROL24VAC, PINxTEST1_CONTROL24VAC);
}
void canal2_relay24VAC_OFF(void)     
{
    PinTo0(PORTWxTEST2_CONTROL24VAC, PINxTEST2_CONTROL24VAC);
}
void canal3_relay24VAC_OFF(void)     
{
    PinTo0(PORTWxTEST3_CONTROL24VAC, PINxTEST3_CONTROL24VAC);
}
////////////////////////////////////////////////////////////////////////////////
//PD2 = PCINT18
void canal1_pinchangelevel_disable(void)
{
    BitTo0(PCMSK2, PCINT18);
}
//PD3 = PCINT19
void canal2_pinchangelevel_disable(void)
{
    BitTo0(PCMSK2, PCINT19);
}
//PD4 = PCINT20
void canal3_pinchangelevel_disable(void)
{
    BitTo0(PCMSK2, PCINT20);
}
////////////////////////////////////////////////////////////////////
//PD2 = PCINT18
void canal1_pinchangelevel_enable(void)
{
    BitTo1(PCMSK2, PCINT18);
}
//PD3 = PCINT19
void canal2_pinchangelevel_enable(void)
{
    BitTo1(PCMSK2, PCINT19);
}
//PD4 = PCINT20
void canal3_pinchangelevel_enable(void)
{
    BitTo1(PCMSK2, PCINT20);
} 
 
uint8_t canal1_pinchangelevel_is_enable(void)
{
    return (PCMSK2 & (1<<PCINT18) );
}
uint8_t canal2_pinchangelevel_is_enable(void)
{
    return (PCMSK2 & (1<<PCINT19) );
}
uint8_t canal3_pinchangelevel_is_enable(void)
{
    return (PCMSK2 & (1<<PCINT20) );
}

void buscando_estado_hilo(void);

void canal_stop(int i)
{
    canal[i].test_is_enabled = 0;
    canal[i].pinchangelevel_disable();
    canal[i].v24ac.off();
    PinTo0(PORTWxTEST_LED_VERDE, PINxTEST_LED_VERDE);           
    //prender led ROJO
    PinTo1(PORTWxBUZZER, PINxBUZZER);
    PinTo1(PORTWxTEST_LED_ROJO, PINxTEST_LED_ROJO);
}
//
void canal_start(int i)
{
    canal[i].test_is_enabled = 1;
    canal[i].pinchangelevel_enable();
    busc_estado_hilo[i].sm0 = 0;//reset cuenta
}
//
#define RELAY_TIMER_TIMING_ON_DELAY 5000    //ms
int main(void)
{
    int8_t count_ticks_pinGetLevel_job = 0;
    
    uint16_t count_ticks_relay_timer = 0;
    int8_t timing_relay_timer=0;
    
    
    __delay_ms(100);    //estabilizar la bornera de power al conectarlo 
    
    PORTB = PORTC = PORTD = 0;
    
    ConfigOutputPin(CONFIGIOxTEST1_CONTROL24VAC, PINxTEST1_CONTROL24VAC);
    ConfigOutputPin(CONFIGIOxRELAY_START_STOP, PINxRELAY_START_STOP);
    //////////////////////////////////////////////////////
    //    Este bloque es inicializado en pinGetLevel
    //    
    //    PinTo1(PORTWxSW_ANULAR, PINxSW_ANULAR);
    //    ConfigInputPin(CONFIGIOxSW_ANULAR, PINxSW_ANULAR);
    //
    //    PinTo1(PORTWxSW_STOP, PINxSW_STOP);
    //    ConfigInputPin(CONFIGIOxSW_STOP, PINxSW_STOP);
    //
    //    PinTo1(PORTWxSW_JOG, PINxSW_JOG);
    //    ConfigInputPin(CONFIGIOxSW_JOG, PINxSW_JOG);
    //
    //    PinTo1(PORTWxSW_START, PINxSW_START);
    //    ConfigInputPin(CONFIGIOxSW_START, PINxSW_START);
    //
    //   ConfigInputPin(CONFIGIOxTEST_INIT, PINxTEST_INIT);
    //
    //    PinTo1(PORTWx24VAC_PRESENTE, PINx24VAC_PRESENTE);
    //    ConfigInputPin(CONFIGIOx24VAC_PRESENTE, PINx24VAC_PRESENTE);
    //    
    //////////////////////////////////////////////////////
    pinGetLevel_init();
    
    
    ConfigOutputPin(CONFIGIOxRELAY_JOG, PINxRELAY_JOG);

    ConfigOutputPin(CONFIGIOxLED_POWER_24VAC_5VDC_PRESENTE, PINxLED_POWER_24VAC_5VDC_PRESENTE);
    ConfigOutputPin(CONFIGIOxTEST_LED_VERDE, PINxTEST_LED_VERDE);
    ConfigOutputPin(CONFIGIOxTEST_LED_ROJO, PINxTEST_LED_ROJO);

    ConfigOutputPin(CONFIGIOxTEST2_CONTROL24VAC, PINxTEST2_CONTROL24VAC);
    ConfigOutputPin(CONFIGIOxTEST3_CONTROL24VAC, PINxTEST3_CONTROL24VAC);


    //No necesitan pull-up, leer directamente
    ConfigInputPin(CONFIGIOxTEST3_SHORTCKT, PINxTEST3_SHORTCKT);
    ConfigInputPin(CONFIGIOxTEST1_SHORTCKT, PINxTEST1_SHORTCKT);
    ConfigInputPin(CONFIGIOxTEST2_SHORTCKT, PINxTEST2_SHORTCKT);
    

    ////////////////////////////////////////////////////////////////////////////
    PinTo1(PORTWxTEST1_SENSORHILO_SENSING_SW, PINxTEST1_SENSORHILO_SENSING_SW);
    ConfigInputPin(CONFIGIOxTEST1_SENSORHILO_SENSING_SW, PINxTEST1_SENSORHILO_SENSING_SW);
    
    PinTo1(PORTWxTEST2_SENSORHILO_SENSING_SW, PINxTEST2_SENSORHILO_SENSING_SW);
    ConfigInputPin(CONFIGIOxTEST2_SENSORHILO_SENSING_SW, PINxTEST2_SENSORHILO_SENSING_SW);
    
    PinTo1(PORTWxTEST3_SENSORHILO_SENSING_SW, PINxTEST3_SENSORHILO_SENSING_SW);
    ConfigInputPin(CONFIGIOxTEST3_SENSORHILO_SENSING_SW, PINxTEST3_SENSORHILO_SENSING_SW);
    ////////////////////////////////////////////////////////////////////////////


    ConfigOutputPin(CONFIGIOxRELAY_TIMER, PINxRELAY_TIMER);
    ConfigOutputPin(CONFIGIOxBUZZER, PINxBUZZER);
    
    ////////////////////////////////////////////////////////////////////////////
    //Config to 1ms
    TCNT0 = 0x00;
    TCCR0A = (1 << WGM01);
    TCCR0B =  (0 << CS02) | (1 << CS01) | (1 << CS00); //CTC, PRES=64
    OCR0A = CTC_SET_OCR_BYTIME(1e-3, 64);//1ms Exacto @PRES=64
    //
    TIMSK0 |= (1 << OCIE0A);
    //
    
    canal[0].pinchangelevel_enable = canal1_pinchangelevel_enable;
    canal[1].pinchangelevel_enable = canal2_pinchangelevel_enable;
    canal[2].pinchangelevel_enable = canal3_pinchangelevel_enable;
    
    canal[0].pinchangelevel_disable = canal1_pinchangelevel_disable;
    canal[1].pinchangelevel_disable = canal2_pinchangelevel_disable;
    canal[2].pinchangelevel_disable = canal3_pinchangelevel_disable;
    
    canal[0].pinchangelevel_is_enable = canal1_pinchangelevel_is_enable;
    canal[1].pinchangelevel_is_enable = canal2_pinchangelevel_is_enable;
    canal[2].pinchangelevel_is_enable = canal3_pinchangelevel_is_enable;
    //
    canal[0].v24ac.on = canal1_relay24VAC_ON;
    canal[1].v24ac.on = canal2_relay24VAC_ON;
    canal[2].v24ac.on = canal3_relay24VAC_ON;
    
    canal[0].v24ac.off = canal1_relay24VAC_OFF;
    canal[1].v24ac.off = canal2_relay24VAC_OFF;
    canal[2].v24ac.off = canal3_relay24VAC_OFF;
    //
    canal[0].fx_shortckt_read = canal1_shortckt_read;
    canal[1].fx_shortckt_read = canal2_shortckt_read;
    canal[2].fx_shortckt_read = canal3_shortckt_read;
    
    
    PD_last = PIND;
    BitTo1(PCICR, PCIE2);
    sei();
    
    
    while (1)
    {
        if (isrflag.sysTickMs)
        {
            isrflag.sysTickMs = 0;
            mainflag.sysTickMs = 1;
        }
        
        if (mainflag.sysTickMs)
        {
            if (++count_ticks_pinGetLevel_job >= 20)
            {
                count_ticks_pinGetLevel_job = 0;
                
                pinGetLevel_job();
                
                //PINxSW_ANULAR
                if (pinGetLevel_hasChanged(0))
                {
                    pinGetLevel_clearChange(0);
                    if (pinGetLevel_level(0) == SW_ANULAR_ON_LEVEL)
                    {
                        
                    }
                }
                //PINxSW_STOP
                if (pinGetLevel_hasChanged(1))
                {
                    pinGetLevel_clearChange(1);
                    if (pinGetLevel_level(1) == SW_STOP_PRESSED_LEVEL)
                    {
                        PinTo0(PORTWxRELAY_START_STOP, PINxRELAY_START_STOP);
                        PinTo0(PORTWxRELAY_TIMER, PINxRELAY_TIMER);
                    }
                }
                //PINxSW_JOG
                if (pinGetLevel_hasChanged(2))
                {
                    pinGetLevel_clearChange(2);
                    if (pinGetLevel_level(2) == SW_JOG_PRESSED_LEVEL)
                    {
                        PinTo1(PORTWxRELAY_JOG, PINxRELAY_JOG);
                    }
                    else
                    {
                        PinTo0(PORTWxRELAY_JOG, PINxRELAY_JOG);
                    }
                }
                //PINxSW_START
                if (pinGetLevel_hasChanged(3))
                {
                    pinGetLevel_clearChange(3);
                    if (pinGetLevel_level(3) == SW_START_PRESSED_LEVEL)
                    {
                        PinTo1(PORTWxRELAY_START_STOP, PINxRELAY_START_STOP);   
                        //
                        timing_relay_timer   = 1;//iniciar temporizado de relay timer
                        count_ticks_relay_timer = 0;
                        PinTo0(PORTWxRELAY_TIMER, PINxRELAY_TIMER);
                        //
                    }
                }
                
               
                //PINxTEST_INIT
                if (pinGetLevel_hasChanged(4))
                {
                    pinGetLevel_clearChange(4);
                    if (pinGetLevel_level(4) == SW_TEST_INIT_ON_LEVEL)
                    {
                        for (int8_t i=0; i<NUM_CANALES_SENSOR; i++)
                        {
                            if (!canal[i].test_is_enabled)
                            {
                                canal_start(i);
                            }
                        }

                    }
                }
                
                //cada 20ms reviso el estado del los 24VAC
                //PINx24VAC_PRESENTE
                if (pinGetLevel_hasChanged(5))
                {
                    pinGetLevel_clearChange(5);
                    
                    if (pinGetLevel_level(5) != LEVEL24VAC_PRESENTE)
                    {
                        //Error
                        for (int8_t i=0; i<NUM_CANALES_SENSOR; i++)
                        {
                            canal[i].u_error.bf.v24ac = 1;
                            canal_stop(i);
                        }
                        PinTo0(PORTWxLED_POWER_24VAC_5VDC_PRESENTE, PINxLED_POWER_24VAC_5VDC_PRESENTE);
                    }
                    else//Sin error
                    {
                        for (int8_t i=0; i<NUM_CANALES_SENSOR; i++)
                        {
                            canal[i].u_error.bf.v24ac = 0;
                            //canal[i].test_is_enabled = 1; //se recupera por el pulsador de TEST_INIT
                        }
                        PinTo1(PORTWxLED_POWER_24VAC_5VDC_PRESENTE, PINxLED_POWER_24VAC_5VDC_PRESENTE);
                    }
                }
                
                
                //
                if (timing_relay_timer)
                {
                    if (++count_ticks_relay_timer >= (250) )
                    {
                        timing_relay_timer = 0;
                        count_ticks_relay_timer = 0;
                        PinTo1(PORTWxRELAY_TIMER, PINxRELAY_TIMER);
                    }
                }
                
                
                //
            }//count_ticks_pinGetLevel_job
                    
        }//mainflag 

        
        //----------------------
        buscando_estado_hilo();
        //----------------------
        
        for (int8_t i=0; i<NUM_CANALES_SENSOR ; i++)
        {
            if (canal[i].test_is_enabled)
            {
                ////////////////////////////////////////////////////////////////////
                //Error por cortocircuito
                if (canal[i].fx_shortckt_read() == SHORTCIRCUIT_LEVEL )
                {
                    canal[i].u_error.bf.shortckt = 1;
                    canal_stop(i);
                    
                    //desactiva relay
                    PinTo0(PORTWxRELAY_START_STOP, PINxRELAY_START_STOP);
                    
                    //
                    continue;
                }

                ////////////////////////////////////////////////////////////////////
                if (canal[i].nuevo_estado)
                {
                    canal[i].nuevo_estado = 0;
                    //
                    if (canal[i].estado_hilo == HILO_ROTO )
                    {
                        canal[i].v24ac.on();

                        //desactiva relay
                        PinTo0(PORTWxRELAY_START_STOP, PINxRELAY_START_STOP);
                    
                        //en este momento no tiene valor buscar el estado del hilo
                        //xq el hardware esta unido con el diodo, entonces
                        //siempre ve que oscila la senal, por eso, desabilito la fx_buscando_estado_hilo()


                        canal[i].pinchangelevel_disable();//busqueda_hilo = off;      

                        //lanzar el temporizador      
                        canal[i].v24ac.bf.timming = 1;
                        canal[i].v24ac.count_time_encendido = 0;

                    }
                    else if (canal[i].estado_hilo == HILO_OK )
                    {
                        canal[i].v24ac.off();
                        PinTo0(PORTWxTEST_LED_VERDE, PINxTEST_LED_VERDE);
                        canal[i].pinchangelevel_enable();   
                    }

                }

                ////////////////////////////////////////////////////////////////////
                if (canal[i].v24ac.bf.timming)
                {
                    if (mainflag.sysTickMs)
                    {
                        //if (++canal[i].v24ac.count_time_encendido >= (uint16_t)(NUMPERIODOS*T_60HZ) )         
                        if (++canal[i].v24ac.count_time_encendido >= 250 )         
                        {
                            canal[i].v24ac.count_time_encendido = 0;

                            canal[i].v24ac.off();
                            canal[i].v24ac.bf.timming = 0;

                            canal[i].pinchangelevel_enable();   
                        }
                    }
                }
            ////////////////////////////////////////////////////////////////////
            }//endif
        }//endfor
       
     
        mainflag.sysTickMs = 0;
    
    }//endwhile
    
    return 0;
}

ISR(TIMER0_COMPA_vect)
{
    isrflag.sysTickMs = 1;    

}

//
/*
 *  PCINT18,19,20
    
    PD2 = PCINT18
    PD3 = PCINT19
    PD4 = PCINT20
 * 
 * 
 * Bit 2 ? PCIF2: Pin Change Interrupt Flag 2
When a logic change on any PCINT[23:16] pin triggers an interrupt request, PCIF2 will be set. If the I-bit
in SREG and the PCIE2 bit in PCICR are set, the MCU will jump to the corresponding Interrupt Vector.
The flag is cleared when the interrupt routine is executed. Alternatively, the flag can be cleared by writing
'1' to it.
 */

ISR(PCINT2_vect)
{
    uint8_t PUERTOD = PIND;
    
    if (PCMSK2 & (1<<PCINT18) ) //esta habilitado ese pin?
    {
        if ( (PUERTOD & (1<<PD2)) ^ (PD_last & (1<<PD2)) )            
        {
            canal[0].count_changelevel++;
            
        }
    }
    
    if (PCMSK2 & (1<<PCINT19) )//esta habilitado ese pin?
    {
        if ( (PUERTOD & (1<<PD3)) ^ (PD_last & (1<<PD3)) )            
        {
            canal[1].count_changelevel++;
        }
    }
    
    if (PCMSK2 & (1<<PCINT20) )//esta habilitado ese pin?
    {
        if ( (PUERTOD & (1<<PD4)) ^ (PD_last & (1<<PD4)) )            
        {
            canal[2].count_changelevel++;
            
        }
    }
    
    PinToggle(PORTWxTEST_LED_VERDE, PINxTEST_LED_VERDE);       
    
    PD_last = PUERTOD;

}

//++++++++++++++++++++++++++++++++++++++++++++++++++
//tiene que ser independiente cada canal controlar su tiempo 
void buscando_estado_hilo(void)
{
    for (int8_t i=0; i<NUM_CANALES_SENSOR; i++ )
    {
        if (canal[i].pinchangelevel_is_enable())
        {
            if (busc_estado_hilo[i].sm0 == 0) 
            {
                busc_estado_hilo[i].counter_ticks = 0;
                busc_estado_hilo[i].sm0++;

                canal[i].count_changelevel = 0x00;
            }
            else
            {
                if (mainflag.sysTickMs)
                {
                    if ( ++busc_estado_hilo[i].counter_ticks >= ((uint16_t)(NUMPERIODOS*T_60HZ)) )
                    {
                        canal[i].pinchangelevel_disable();

                        if (canal[i].count_changelevel >= (uint16_t)(PORCENTAJE_UMBRAL_NUMCAMBIOS*(2*NUMPERIODOS)) )
                        //if (canal[i].count_changelevel >= 14 )
                        {
                           canal[i].estado_hilo = HILO_ROTO;
                        }
                        else
                        {
                           canal[i].estado_hilo = HILO_OK;
                        }
                        canal[i].nuevo_estado = 1;
                        //
                        canal[i].count_changelevel = 0;
                        busc_estado_hilo[i].sm0 = 0x00;
                        //
                    }
                }
            }
            
        }
       
    }//endfor
}

