/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"

#define GREEN_LED BIT6
u_int bgColor = COLOR_KHAKI;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */
Region p1;
Region p2;

AbRect rect1 = {abRectGetBounds, abRectCheck, {5,10}}; /**< 5x5 rectangle */
AbRect rect2 = {abRectGetBounds, abRectCheck, {5,10}}; /**< 5x5 rectangle */

u_char points1 =0;
u_char points2=0;

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 8, screenHeight/2 - 8}
};

  
Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  0
};

Layer layer3 = {		/**< Layer with an orange circle */
  (AbShape *)&rect1,
  {(screenWidth/2)-50, (screenHeight/2)+60}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLUE,
  &fieldLayer,
};

Layer layer1 = {		/**< Layer with a red square */
  (AbShape *)&rect2,
  {(screenWidth/2)+50, (screenHeight/2)+60}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_HOT_PINK,
  &layer3,
};


Layer layer0 = {		/**< Layer with an orange circle */
  (AbShape *)&circle8,
  {(screenWidth/2), (screenHeight/2)}, /**< bit below & right of center */
  {0,0}, {2,2},				    /* last & next pos */
  COLOR_FIREBRICK,
  &layer1,
};



/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml3 = { &layer3, {0,0}, 0 }; /**< not all layers move */
MovLayer ml1 = { &layer1, {0,0}, &ml3}; 
MovLayer ml0 = { &layer0, {2,2}, &ml1}; 

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence, Region *paddle1, Region *paddle2)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  int velocity;
  //for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    if(((shapeBoundary.topLeft.axes[1] < paddle1->botRight.axes[1]) && (shapeBoundary.topLeft.axes[1] > paddle1->topLeft.axes[1]) && (shapeBoundary.topLeft.axes[0] <= paddle1->botRight.axes[0])) ||((shapeBoundary.botRight.axes[1] > paddle2->topLeft.axes[1]) && (shapeBoundary.botRight.axes[0] >= paddle2->topLeft.axes[0]) && (shapeBoundary.botRight.axes[1] < paddle2->botRight.axes[1])) ){
  	velocity = ml->velocity.axes[0] = -ml->velocity.axes[0];
	newPos.axes[0] += (2*velocity);
    }
    //Ball bounces off of top and bottom walls
    if ((shapeBoundary.topLeft.axes[1] < fence->topLeft.axes[1]) ||
	(shapeBoundary.botRight.axes[1] > fence->botRight.axes[1])){
      velocity = ml->velocity.axes[1] = -ml->velocity.axes[1];
      newPos.axes[1] += (2*velocity);
      
    }
    //If ball hits left vertical wall
    if ((shapeBoundary.topLeft.axes[0] < fence->topLeft.axes[0])){
      points1++;
      
    }
    //If ball hits right vertical wall
    if ((shapeBoundary.botRight.axes[0] < fence->botRight.axes[0])){
      points2++;
      
    }

    ml->layer->posNext = newPos;
} /**< for ml */






/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  //p2sw_init(1);
  buzzer_init();
  shapeInit();

  layerInit(&layer0);
  layerDraw(&layer0);

  layerGetBounds(&fieldLayer, &fieldFence);
 
  layerGetBounds(&layer3, &p1);
  layerGetBounds(&layer1, &p2);
 
  p2sw_init(15);
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  
 
  
  //layer 3 is left paddle
  
  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    drawString5x7(10,10, "score:", COLOR_BLACK, bgColor);
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
  
    //drawString5x7(20,20, str, COLOR_BLACK, bgColor);
      
    redrawScreen = 0;
    //p2sw_init(15);
  
    // u_int switches = p2sw_read();
    // if((switches&=128) == 0){
    // layer3.pos.axes[1]+10;
    
  
    //movLayerDraw(&ml0, &layer0);
}
 

}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
 
  if (count == 15) {
    layerGetBounds(&fieldLayer, &fieldFence);
    layerGetBounds(&layer3, &p1);
    layerGetBounds(&layer1, &p2);
    movLayerDraw(&ml0, &layer0);
    mlAdvance(&ml0, &fieldFence, &p1, &p2);
    if (p2sw_read())
      redrawScreen = 1;

  u_int switches = p2sw_read(), i;
    char str[5];
    for (i = 0; i < 4; i++)
      if(!(switches & (1<<i))){
	  switch(i){
	  case 0:
	    ml3.velocity.axes[1]=-5;
	    movLayerDraw(&ml3,&layer3);
	    mlAdvance(&ml3,&fieldFence, &p1, &p2);
	    redrawScreen = 1;
	    break;
	  case 1:
	    ml3.velocity.axes[1]=5;
	    movLayerDraw(&ml3,&layer3);
	    mlAdvance(&ml3,&fieldFence, &p1, &p2);
	    redrawScreen = 1;
	    break;
	  
	  case 2:
	    ml1.velocity.axes[1]=5;
	    movLayerDraw(&ml1,&layer1);
	    mlAdvance(&ml1,&fieldFence, &p1, &p2);
	    redrawScreen = 1;
	    break;
	  
          case 3:
	    ml1.velocity.axes[1]=-5;
	    movLayerDraw(&ml1,&layer1);
	    mlAdvance(&ml1,&fieldFence, &p1, &p2);
	    redrawScreen = 1;
	    break;
	  
	  } 
      
      }
    count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
