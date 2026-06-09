/*  sesnro de hilo
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

volatile struct _isrflag isrflag;
struct _mainflag mainflag;

enum _ESTADO_HILO
{
    BUSCANDO = 0,
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
    //estado de sensor de hilo
    int8_t estado_hilo;
    uint8_t count_changelevel;
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
////////////////////////////////////////////////////////////////////////////////
//uint8_t read_estado_hilo1(void)
//{
//    return ReadPin(PORTRxTEST1_SENSORHILO_SENSING_SW, PINxTEST1_SENSORHILO_SENSING_SW);
//}
//uint8_t read_estado_hilo2(void)
//{
//    return ReadPin(PORTRxTEST2_SENSORHILO_SENSING_SW, PINxTEST2_SENSORHILO_SENSING_SW);
//}
//uint8_t read_estado_hilo3(void)
//{
//    return ReadPin(PORTRxTEST3_SENSORHILO_SENSING_SW, PINxTEST3_SENSORHILO_SENSING_SW);
//}
////////////////////////////////////////////////////////////////////////////////
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

int main(void)
{
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
    //    PinTo1(PORTWx24VAC_PRESENTE, PINx24VAC_PRESENTE);
    //    ConfigInputPin(CONFIGIOx24VAC_PRESENTE, PINx24VAC_PRESENTE);
    //////////////////////////////////////////////////////
    
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

    ConfigInputPin(CONFIGIOxTEST_INIT, PINxTEST_INIT);

    ConfigOutputPin(CONFIGIOxRELAY_TIMER, PINxRELAY_TIMER);

    ConfigOutputPin(CONFIGIOxBUZZER, PINxBUZZER);
    

    ////////////////////
    //Solo se enciende cuando detecto un error, 
    //    PinTo1(PORTWxTEST1_CONTROL24VAC, PINxTEST1_CONTROL24VAC);
    //    PinTo1(PORTWxTEST2_CONTROL24VAC, PINxTEST2_CONTROL24VAC);
    //    PinTo1(PORTWxTEST3_CONTROL24VAC, PINxTEST3_CONTROL24VAC);
    //    
    
    ////////////////////////////////////////////////////////////////////////////
    //Config to 1ms
    TCNT0 = 0x00;
    TCCR0A = (1 << WGM01);
    TCCR0B =  (0 << CS02) | (1 << CS01) | (1 << CS00); //CTC, PRES=64
    OCR0A = CTC_SET_OCR_BYTIME(1e-3, 64);//1ms Exacto @PRES=64
    //
    TIMSK0 |= (1 << OCIE0A);
    
    //
    
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
    
    for (int8_t i=0; i<NUM_CANALES_SENSOR; i++)
    {
        canal[i].pinchangelevel_enable();
    }
    
    BitTo1(PCICR, PCIE2);
    sei();
    
    while (1)
    {
        if (isrflag.sysTickMs)
        {
            isrflag.sysTickMs = 0;
            mainflag.sysTickMs = 1;
        }
        //----------------------
        buscando_estado_hilo();
        //----------------------
        
        //test pulsadores externos
        if (PinRead(PORTRxSW_ANULAR, PINxSW_ANULAR) == SW_ANULAR_ON_LEVEL)
        {
        }
        if (PinRead(PORTRxSW_STOP, PINxSW_STOP) == SW_STOP_PRESSED_LEVEL)
        {
            PinTo0(PORTWxRELAY_START_STOP, PINxRELAY_START_STOP);
        }
        if (PinRead(PORTRxSW_JOG, PINxSW_JOG) == SW_JOG_PRESSED_LEVEL)
        {
            PinTo1(PORTWxRELAY_JOG, PINxRELAY_JOG);
        }
        if (PinRead(PORTRxSW_START, PINxSW_START) == SW_START_PRESSED_LEVEL)
        {
            PinTo1(PORTWxRELAY_START_STOP, PINxRELAY_START_STOP);
        }
        //(PORTWxRELAY_TIMER, PINxRELAY_TIMER);
        //PinTo1(PORTWxBUZZER, PINxBUZZER); //al ocurrir un error
        ////////////////////////////////////////////////////////////////
        
        //Error por cortocircuito
        for (int i=0; i<NUM_CANALES_SENSOR; i++)
        {
            if (canal[i].fx_shortckt_read() == SHORTCIRCUIT_LEVEL )
            {
                canal[i].u_error.bf.shortckt = 1;//error = 1;

                canal[i].v24ac.off();
                
                //desactiva relay
                PinTo0(PORTWxRELAY_START_STOP, PINxRELAY_START_STOP);
                PinTo1(PORTWxBUZZER, PINxBUZZER);
                //
                PinTo1(PORTWxTEST_LED_ROJO, PINxTEST_LED_ROJO);
            }
        }
        
        // si no existe ningun de los 24VDC, entonces tambien es un error
        //Error en general, 
        //EN ESTE PUNTO DE PARAR EL CANAL CORRESPONDIENTE??        
        
        for (int8_t i=0; i<NUM_CANALES_SENSOR ; i++)
        {
            if (canal[i].estado_hilo != BUSCANDO )
            {
                //quiere decir que ya tiene un valor establecido
                if (canal[i].estado_hilo == HILO_ROTO )
                {
                    canal[i].v24ac.on();
                    
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
                }
                //dejar preparado para la sgt. busqueda
                canal[i].estado_hilo = BUSCANDO;
            }
        }
        //
        for (int8_t i=0; i<NUM_CANALES_SENSOR ; i++)
        {
            if (canal[i].v24ac.bf.timming)
            {
                if (mainflag.sysTickMs)
                {
                    if (++canal[i].v24ac.count_time_encendido >= (uint16_t)(NUMPERIODOS*T_60HZ) )         
                    {
                        canal[i].v24ac.off();
                        canal[i].v24ac.bf.timming = 0;
                        
                        canal[i].pinchangelevel_enable();   //ahora volver a busqueda_hilo = on;
                    }
                }
            }
        }//endfor
     
        mainflag.sysTickMs = 0;
    
    }//endwhile
    
    return 0;
}

ISR(TIMER0_COMPA_vect)
{
    isrflag.sysTickMs = 1;    //PinToggle(PORTWxGETLEVEL_0, PINxGETLEVEL_0);
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
    volatile static uint8_t PD_last;
    
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
    
    PD_last = PUERTOD;
}
////////////////////////////////////////////////////////////////////////////////
//acaba el tiempo:
//solo tiene 2 opciones, hiloabierto, hilook
////////////////////////////////////////////////////////////////////////////////    
//void buscando_estado_hilo(void)
//{
//    if (busc_estado_hilo.sm0 == 0) 
//    {
//        
//        busc_estado_hilo.counter_ticks = 0;
//        busc_estado_hilo.sm0++;
//        
//        for (int8_t i=0; i<NUM_CANALES_SENSOR; i++ )
//        {
//            canal[i].count_changelevel = 0x00;
//        }
//        BitTo1(PCICR, PCIE2);//Activar interrupciones por cambio de nivel
//    }
//    else
//    {
//        if (mainflag.sysTickMs)
//        {
//            if ( ++busc_estado_hilo.counter_ticks >= (NUMPERIODOS*T_60HZ) )
//            {
//                BitTo0(PCICR, PCIE2);
//                
//                for (int8_t i=0; i<NUM_CANALES_SENSOR; i++)
//                {
//                     if (canal[i].count_changelevel >= (uint16_t)(PORCENTAJE_UMBRAL_NUMCAMBIOS*(2*NUMPERIODOS)) )
//                     {
//                        canal[i].estado_hilo = HILO_ROTO;
//                     }
//                     else
//                     {
//                        canal[i].estado_hilo = HILO_OK;
//                     }
//                }
//                //
//                busc_estado_hilo.sm0 = 0;
//                //
//            }
//        }
//    }
//}

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

                canal[i].pinchangelevel_enable();
                //BitTo1(PCICR, PCIE2);//Activar interrupciones por cambio de nivel
            }
            else
            {
                if (mainflag.sysTickMs)
                {
                    if ( ++busc_estado_hilo[i].counter_ticks >= (NUMPERIODOS*T_60HZ) )
                    {
                        //BitTo0(PCICR, PCIE2);
                        canal[i].pinchangelevel_disable();

                        if (canal[i].count_changelevel >= (uint16_t)(PORCENTAJE_UMBRAL_NUMCAMBIOS*(2*NUMPERIODOS)) )
                        {
                           canal[i].estado_hilo = HILO_ROTO;
                        }
                        else
                        {
                           canal[i].estado_hilo = HILO_OK;
                        }
                        //
                        busc_estado_hilo[i].sm0 = 0;
                        //
                    }
                }
            }
            
        }
       
    }//endfor
}