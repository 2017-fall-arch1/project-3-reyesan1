#include <msp430.h>
#include "libTimer.h"
#include "buzzer.h"


int period =2000; 
void buzzer_init()
{
    /* 
       Direct timer A output "TA0.1" to P2.6.  
        According to table 21 from data sheet:
          P2SEL2.6, P2SEL2.7, anmd P2SEL.7 must be zero
          P2SEL.6 must be 1
        Also: P2.6 direction must be output
    */
    timerAUpmode();		/* used to drive speaker */
    P2SEL2 &= ~(BIT6 | BIT7);
    P2SEL &= ~BIT7; 
    P2SEL |= BIT6;
    P2DIR = BIT6;		/* enable output to speaker (P2.6) */
    //buzzer_set_period(6212);	/* start buzzing!!! */
    buzzer_advance_frequency();
}

void buzzer_advance_frequency() 
{
  
  buzzer_set_period(period);
}

void buzzer_set_period(short cycles)
{
  CCR0 = cycles;
  CCR1 = cycles >> 1;
  	/* one half cycle */
}


    
    
  

