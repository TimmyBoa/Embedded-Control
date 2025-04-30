////////////////////////////////////////////////////////////////////////
//** ENGR-2350 Template Project 
//** NAME: Timothy Pollard, Billy Di
//** RIN: 662084381 662056905
//** This is the base project for several activities and labs throughout
//** the course.  The outline provided below isn't necessarily *required*
//** by a C program; however, this format is required within ENGR-2350
//** to ease debugging/grading by the staff.
////////////////////////////////////////////////////////////////////////

// We'll always add this include statement. This basically takes the
// code contained within the "engr_2350_msp432.h" file and adds it here.
#include "engr2350_msp432.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
// Add function prototypes here as needed.
void GPIOInit();
void timerInit();
void Encoder_ISR();

// Add global variables here as needed.
int32_t capture_value;
uint32_t enc_total;
int32_t enc_counts_track;
int32_t enc_counts;
int32_t overflows;
uint8_t enc_flag;
const double radius = 20;
const double pi = M_PI;

Timer_A_ContinuousModeConfig timerConfig = {
    .clockSource = TIMER_A_CLOCKSOURCE_ACLK,
    .clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1,
    .timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE,
    .timerClear = TIMER_A_DO_CLEAR
};

Timer_A_CaptureModeConfig timerCaptureConfig = {
    .captureRegister = TIMER_A_CAPTURECOMPARE_REGISTER_0,
    .captureMode = TIMER_A_CAPTUREMODE_FALLING_EDGE,
    .captureInputSelect = TIMER_A_CAPTURE_INPUTSELECT_CCIxA,
    .synchronizeCaptureSource = TIMER_A_CAPTURE_SYNCHRONOUS,
    .captureInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE,
    .captureOutputMode = 0
};

int main() {    //// Main Function ////
  
    // Add local variables here as needed.
    uint16_t distance;
    float delta_t;
    double temp;
    uint16_t speed_rpm;
    uint16_t speed_mm;

    // We always call the sysInit function first to set up the 
    // microcontroller for how we are going to use it.
    sysInit();
    GPIOInit();
    timerInit();

    // Place initialization code (or run-once) code here
    printf("\r\nDistance\tEnc_Counts\tDelta T\t\tAng.Speed\tLin.Speed\r\n");
    float count = 0;
    while(1){  
        if(enc_flag){ // Check to see if capture occurred
            enc_flag = 0; // reset capture flag
            // d = 2\pi r* \frac{1}{360}*Cevent
            temp = 2*pi*radius;
            count +=1;
            distance = ((temp*count)/360); // finding total distance around circumfrence

            // \Delta t(k)  = \frac{(Ncap(K) - Ncap(k-1)) + 65536Noverflows(k)}{ftclk}
            temp = ((double)(capture_value+(enc_counts))/(24.0*1000000.0));
            delta_t = fabs((float)temp); // change in time between events

            // \omega[rpm] = \frac{1}{360}\frac{1}{\Delta t}\frac{60s}{1min}
            temp = (1.0/delta_t)*60.0; // calculation without round to int early
            speed_rpm = temp/360; // speed in rpm

            // \textit{v} = \omega[rad/s]r = \frac{2\pi}{360}\frac{1}{\Delta t}r
            temp = (2.0)*(radius)*(pi)*(1.0/delta_t); // calculation without round to int early
            speed_mm = temp/360; // speed in mm

            printf("%5u mm\t%6u\t\t%.4f s\t%5u rpm\t%5u mm/s\r\n",distance,enc_counts,delta_t,speed_rpm,speed_mm);

        }
        // Place code that runs continuously in here
    }   
}    //// Main Function ////  

// Add function declarations here as needed
void GPIOInit(){
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P10, GPIO_PIN4, GPIO_PRIMARY_MODULE_FUNCTION);
    //GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P10, GPIO_PIN5, GPIO_PRIMARY_MODULE_FUNCTION);
}

void timerInit(){
    Timer_A_configureContinuousMode(TIMER_A3_BASE, &timerConfig);
    Timer_A_initCapture(TIMER_A3_BASE, &timerCaptureConfig);
    Timer_A_registerInterrupt(TIMER_A3_BASE, TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT, Encoder_ISR);
    Timer_A_registerInterrupt(TIMER_A3_BASE, TIMER_A_CCR0_INTERRUPT, Encoder_ISR);
    Timer_A_startCounter(TIMER_A3_BASE, TIMER_A_CONTINUOUS_MODE);
    Timer_A_enableInterrupt(TIMER_A3_BASE);
    Timer_A_enableCaptureCompareInterrupt(TIMER_A3_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
}

// Add interrupt functions last so they are easy to find
void Encoder_ISR(){
    if (Timer_A_getCaptureCompareEnabledInterruptStatus(TIMER_A3_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0)){
        Timer_A_clearCaptureCompareInterrupt(TIMER_A3_BASE , TIMER_A_CAPTURECOMPARE_REGISTER_0);
        capture_value = Timer_A_getCaptureCompareCount(TIMER_A3_BASE , TIMER_A_CAPTURECOMPARE_REGISTER_0);
        enc_total += capture_value;
        enc_counts = enc_counts_track + capture_value;
        enc_counts_track = -capture_value;
        enc_flag = 1;
        overflows=0;
    }
    if (Timer_A_getInterruptStatus(TIMER_A3_BASE) == TIMER_A_INTERRUPT_PENDING){
        Timer_A_clearInterruptFlag(TIMER_A3_BASE);
        enc_counts_track += 65536;
        overflows+=1;
    }
}
