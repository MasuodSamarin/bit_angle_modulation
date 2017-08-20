/*
 * =====================================================================================
 *
 *       Filename:  bam.c
 *
 *    Description:  bit angle modulation console Version
 *                  im not navtive english
 *                  so, forgive me for bad english
 *                  i do my best and i promise edit the comment in the fucure editing
 *
 *
 *        Version:  1.0
 *        Created:  08/04/2017 02:31:39 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  masuood samarin, 
 *   Organization:  base2embedded
 *
 * =====================================================================================
 */


#include	<stdio.h>
#include	<stdlib.h>
#include    <string.h>


/* HELPER FUNCTION
 * ii use this to change the represent the data value in binary format
 * i use it in the printf function
 * its not actually useful in the final program*/
char * int2bin(int i);


typedef unsigned char uint8_t;

#define NUM_OF_SHFT_REG    2
#define NUM_OF_SHFT_REG_OUT_CHN    8
#define MAX_PWM_CHNN (NUM_OF_SHFT_REG * NUM_OF_SHFT_REG_OUT_CHN)

#define BAM_RESOLUTION  4
#define BAM_MAX_STATE   4

/*
 * becuase we start counting from zero
 * so,i subtract the max numbers from 1*/
#define BAM_STATE_0_MAX_COUNTER 0
#define BAM_STATE_1_MAX_COUNTER 1
#define BAM_STATE_2_MAX_COUNTER 3
#define BAM_STATE_3_MAX_COUNTER 7

/*  we have 4 state and this is the enumeration of those satate*/
typedef enum {
    BAM_STATE_BIT_0 = 0,
    BAM_STATE_BIT_1,
    BAM_STATE_BIT_2,
    BAM_STATE_BIT_3,
} BAM_STATES_T;

/* incomplete data type to BAM_Handler we use this in all function call's*/
typedef struct BAM_Handler *BAM_Handler_Ptr;

/*  wrappper the function which is functionallity is sending data to the shift register chain
 *  so we can use both SPI or GPIO function call*/
typedef void (*bam_state_send_data_fp) (BAM_Handler_Ptr);
/* this is orginal function prototype */
void bam_state_send_data_func (BAM_Handler_Ptr handler);


/* BAM_Handler is everything we have  
 * we pass pointer to instance of this struct to every functions and API's
 * i wonder i actually, do't need some of these data member 
 * for ex cur_state, in the end if i dont use any af that i delete it 
 *
 * buffer is the led pool 2d array which is the row in the array is BAM_RESOLUTION and
 * the coloumn is NUM_OF_SHFT_REG,
 *
 * max_counter is the maximum delay, that certain state must be spend
 * counter keep track of that,
 * data is the pointer to the arry that must be to send to the shift register chains.
 *
 * */
typedef struct BAM_Handler{

    BAM_STATES_T next_state;
    BAM_STATES_T cur_state;
    uint8_t *buffer;
    uint8_t *data;
    uint8_t max_counter;
    uint8_t counter;
    const bam_state_send_data_fp send_data ;
    
}BAM_Handler;


/* 
 * LED_BUFFER holding pwm percentage of each channel in number between 0 - 16 
 * becuase we design the 4bit resolution pwm, so we have 4 rows ecach of them has 1bit of the whole pwm value
 * we have 2 coloumn becuase we have 16 channel of pwm, each one for 8 channel or one shift register
 * first row must send to chain in state 0
 * sec row in same manner send to chain in sate 1 
 * and goes a round
 * i pick some random number just to show in screen when we saw the operation of program
 * */
uint8_t LED_BUFFER [BAM_RESOLUTION][NUM_OF_SHFT_REG] = {

    { 121, 18 },
    { 12, 127 },
    { 233, 86 },
    { 64, 135 }
};


/* 
 * this is function pointer that each state must be look like this*/
typedef void (*bam_state_machine_fp) (BAM_Handler_Ptr);

/* declaration of state function
 * each satate has own prototype*/
void bam_state_func_0 (BAM_Handler_Ptr);
void bam_state_func_1 (BAM_Handler_Ptr);
void bam_state_func_2 (BAM_Handler_Ptr);
void bam_state_func_3 (BAM_Handler_Ptr);

/* array of function pointer's 
 * we use BAM_TABLE to switch between state's*/
const bam_state_machine_fp BAM_TABLE [BAM_MAX_STATE] = {
    bam_state_func_0,
    bam_state_func_1,
    bam_state_func_2,
    bam_state_func_3
};


/*
 * HELPER FUNCTIONS SECTION
 * */

/* dynamic allocator  for BAM_Handler
 *  its return a pointer to teh new instance of BAM_Handler
 *  it actually return BAM_Handler_Ptr
 *
 * */
BAM_Handler_Ptr bam_init_handler (void);

/* 
 * this function must be run in every tick period
 * */
void bam_refresh (BAM_Handler_Ptr);

/* we simply use this for show the result in the screen
 * in the final code we dont use this 
 * insted of we put SPI implementation for sending data to shift register chains*/
void bam_print_data (BAM_Handler_Ptr handler);

/* 
void bam_print_data (BAM_Handler_Ptr handler);
 *  actually we dont need to this function becuase the state change automaticlly
 *  we implement the changing state in the cocrete state function
 * */
void bam_change_state(BAM_Handler_Ptr handler, BAM_STATES_T next_state);



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
    int
main ( int argc, char *argv[] )
{

    BAM_Handler_Ptr handler = bam_init_handler();
/*    BAM_Handler handler = { .next_state = BAM_STATE_BIT_0, 
                            .data = LED_BUFFER[BAM_STATE_BIT_0],
                            .counter = 0,
                            .send_data = bam_state_sd };
*/    
    int i;
    for (i=0; i<15; i++){
        bam_refresh (handler);
    }

    return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */

BAM_Handler_Ptr bam_init_handler (void){

    BAM_Handler init = { .next_state = BAM_STATE_BIT_0, 
                            .cur_state = BAM_STATE_BIT_0,
                            .data = LED_BUFFER[BAM_STATE_BIT_0],
                            .max_counter = BAM_STATE_0_MAX_COUNTER,
                            .counter = 0,
                            .send_data = bam_state_send_data_func };

    BAM_Handler_Ptr handler = (BAM_Handler_Ptr)malloc(sizeof(BAM_Handler));

    if(handler){
        memcpy(handler, &init, sizeof(BAM_Handler));
        return handler;
    }
    printf("allocation heap memory failure");
    return NULL;
}

void bam_state_send_data_func (BAM_Handler_Ptr handler){
    bam_print_data(handler);
}

void bam_change_state(BAM_Handler_Ptr handler, BAM_STATES_T next_state){

    handler->cur_state = handler->next_state;
    handler->next_state = next_state;
}

void bam_print_data (BAM_Handler_Ptr handler){

    printf("\nstate:%d\t  max cnt:%d\tsending data:%s,%s\tdelay cycle:",
            (int)handler->next_state, handler->max_counter+1, int2bin(handler->data[0]), int2bin(handler->data[1]));
}

void bam_refresh (BAM_Handler_Ptr handler){

    BAM_TABLE[handler->next_state](handler);
    printf("|");
}



/*
 *  concrete state function
 *  this is the place that we really implement the state's code
 *  everything we have in the program 
 *  simply 
 *  first of all i send data to the chain
 *  followed by line's of code that  i set the next_state variable's
 *
 *  note that because in the first state we actually spend one cycle period, so we dont check Or 
 *  anything else, just jump to next state 
 *
 *  but in other state we add a if condition to check send data ,delay or just jump over the state
 *  i hope its make sense :))
 *
 * */
void bam_state_func_0 (BAM_Handler_Ptr handler){
    handler->send_data(handler);

    handler->next_state = BAM_STATE_BIT_1;
    handler->data = LED_BUFFER[BAM_STATE_BIT_1];
    handler->counter = 0;
    handler->max_counter = BAM_STATE_1_MAX_COUNTER;
}

void bam_state_func_1 (BAM_Handler_Ptr handler){
    if(!handler->counter){
        handler->send_data(handler);

    }else{
        handler->next_state = BAM_STATE_BIT_2;
        handler->data = LED_BUFFER[BAM_STATE_BIT_2];
        handler->counter = 0;
        handler->max_counter = BAM_STATE_2_MAX_COUNTER;
        return;
    }

    handler->counter = handler->counter + 1;
}
void bam_state_func_2 (BAM_Handler_Ptr handler){
    if(!handler->counter){
        handler->send_data(handler);

    }else if (handler->counter >= handler->max_counter){
        handler->next_state = BAM_STATE_BIT_3;
        handler->data = LED_BUFFER[BAM_STATE_BIT_3];
        handler->counter = 0;
        handler->max_counter = BAM_STATE_3_MAX_COUNTER;
        return;
    }

    handler->counter = handler->counter + 1;
}
void bam_state_func_3 (BAM_Handler_Ptr handler){
    if(!handler->counter){
        handler->send_data(handler);

    }else if (handler->counter >= handler->max_counter){
        handler->next_state = BAM_STATE_BIT_0;
        handler->data = LED_BUFFER[BAM_STATE_BIT_0];
        handler->counter = 0;
        handler->max_counter = BAM_STATE_0_MAX_COUNTER;
        return;
    }

    handler->counter = handler->counter + 1;
}


/*  
 *  HELPER FUNCTION
	return the binary value 
	porpose is to use in printf
*/
#define DIGIT_NUMBER    8 // its 
char * int2bin(int i)
{
    size_t bits = (size_t) DIGIT_NUMBER;

    char * str = malloc(bits + 1);
    if(!str) return NULL;
    str[bits] = 0;

    // type punning because signed shift is implementation-defined
    unsigned u = *(unsigned *)&i;
    for(; bits--; u >>= 1)
        str[bits] = u & 1 ? '1' : '0';

    return str;
}
