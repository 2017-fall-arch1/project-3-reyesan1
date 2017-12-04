#include <msp430.h>
#include "libTimer.h"
#include "buzzer.h"

#define r 0
#define a 1760
#define b 3030
#define gs 6644
#define g 3136
#define fs 5919
#define e 5274
#define lds 4978
#define f 5587
#define lf 5919
#define ds 2489
#define d 4699
#define lcs 4434
#define lc 4186
#define lb 7458
static int kp[] = {ds,f,lc,a,gs,r,b,b,b,9};
static int n = 0;
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
    buzzer_advance_frequency();
}

void buzzer_advance_frequency() {
  //period = kp[n];
  //buzzer_set_period(period);
  //n++;
  buzzer_set_period(0);
}

void buzzer_set_period(short cycles)
{
  CCR0 = cycles;
  CCR1 = cycles >> 2;
  	/* one half cycle */
}


    
    
  

