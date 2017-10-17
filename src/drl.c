/*
   //      _______ __  _________ _________
   //      \_     |  |/  .  \   |   \ ___/
   //        / :  |   \  /   \  |   / _>_
   //       /      \ | \ |   /  |  /     \
   //      /   |___/___/____/ \___/_     /
   //      \___/--------TECH--------\___/
   //        ==== ABOVE SINCE 1994 ====
   //
   //  Ab0VE TECH - DRivingLights Controller
   //
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// CONFIG
#define DRL_SIG   PINB0 // DRL output
#define SIDE_SIG  PINB1 // Sidemarkers
#define PRK_SIG   PINB2 // P/N AT selector
#define IGN_SIG   PINB3 // ADC - iginition prent
#define PWR_SIG   PINB4 // keep ON

#define FULL_DRL  255
#define DIM_DRL   128
#define FULL_DRL_ON_SHUTDOWN // switch on 100% DRL on SHUTDOWN

#define DELAY 1000
#define DELAY_SHUTDOWN 10000 // N times of DELAY

uint8_t DRL_ILLUMINACE=0;
uint8_t DRL_STATE=0;
int16_t DEAD_COUNTER=-1;
uint8_t counter=0;
uint8_t DRL_buf;

#define _SLEEP __asm__ __volatile__ ("sleep");

ISR (TIM0_OVF_vect) { //PWM via timer overflow
        if (++counter==0)
        {
                DRL_buf=DRL_STATE;
                PORTB |=_BV(DRL_SIG);
        }
        if (counter==DRL_buf) PORTB&=~_BV(DRL_SIG);
}

int main(){
        DDRB = _BV(DRL_SIG) | _BV(PWR_SIG);
        TIMSK0 = 0b00000010;
        TCCR0B = 0b00000001;
        sei();

        PORTB |= _BV(PWR_SIG); // latch POWER ON

        while (1) { // Main loop
// SIGNALS Reading
                if ((PORTB & _BV(SIDE_SIG))==0) { // No headlights
                        if ((PORTB & _BV(PRK_SIG))!=0) { // P or N position AT
                                DRL_ILLUMINACE=DIM_DRL; // 50%
                        } else
                                DRL_ILLUMINACE=FULL_DRL; // 100% on D
                } else
                        DRL_ILLUMINACE=0;

// IGNITION check
                if ((PORTB & _BV(IGN_SIG))==0) {         // IGNITION DOWN
                        DEAD_COUNTER=DELAY_SHUTDOWN; // Time-out before full OFF
#ifdef FULL_DRL_ON_SHUTDOWN
                        DRL_ILLUMINACE=255;
#endif
                } else
                {
                        if (DEAD_COUNTER>0) // restart ignition while shutdown
                        {
                                DEAD_COUNTER=-1; // stop shutdown
                        }
                }

// Dim or Bright controll
                if (DRL_ILLUMINACE>DRL_STATE) DRL_STATE++;
                if (DRL_ILLUMINACE<DRL_STATE) DRL_STATE--;

// Shutdown squence
                if (DEAD_COUNTER>0) DEAD_COUNTER--;
                if (DEAD_COUNTER==0) { // SHUTDOWN NOW
                        while (DRL_STATE > 0) {
                                DRL_STATE--;
                                _delay_us(DELAY);
                        }
                        PORTB = 0; // Clear DRL_SIG and PRW_SIG
                        _SLEEP; // If power not down for some reasons
                }

                _delay_us(DELAY);
        }
}
