////////////////////////////////////////////////////////////////////////
//** ENGR-2350 Lab 4 Template
//** NAME: Timothy Pollard
//** RIN: 662084381
////////////////////////////////////////////////////////////////////////
//**
//** README!!!!!
//** README!!!!!
//** README!!!!!
//**
//** This template project has all initializations required to both control the motors
//** via PWM and measure the speed of the motors. The PWM is configured using a 25 kHz
//** period (960 counts). The motors are initialized to be DISABLED and in FORWARD mode.
//** The encoders measurements are stored within the variables TachR and TachL for the
//** right and left motors, respectively. A maximum value for TachR and TachL is
//** enforced to be 1e6 such that when the wheel stops, a reasonable value for the
//** encoders exists: a very large number that can be assumed to be stopped.
//** Finally, a third timer is added to measure a 100 ms period for control system
//** timing. The variable runControl is set to 1 each period and then reset in the main.

#include "engr2350_msp432.h"
#include <math.h>

void GPIOInit();
void TimerInit();
void ADCInit();
void Encoder_ISR();
void T2_100ms_ISR();

Timer_A_UpModeConfig TA0cfg; // PWM timer
Timer_A_UpModeConfig TA2cfg; // 100 ms timer
Timer_A_ContinuousModeConfig TA3cfg; // Encoder timer
Timer_A_CompareModeConfig TA0_ccr3; // PWM Right
Timer_A_CompareModeConfig TA0_ccr4; // PWM Left
Timer_A_CaptureModeConfig TA3_ccr0; // Encoder Right
Timer_A_CaptureModeConfig TA3_ccr1; // Encoder Left


// Encoder total events
uint32_t enc_total_L,enc_total_R;
// Speed measurement variables
// Note that "Tach" stands for "Tachometer," or a device used to measure rotational speed
int32_t TachL_count,TachL,TachL_sum,TachL_sum_count,TachL_avg; // Left wheel
int32_t TachR_count,TachR,TachR_sum,TachR_sum_count,TachR_avg; // Right wheel
    // TachL,TachR are equivalent to enc_counts from Activity 10/Lab 3
    // TachL/R_avg is the averaged TachL/R value after every 12 encoder measurements
    // The rest are the intermediate variables used to assemble TachL/R_avg

uint8_t runControl = 0; // Flag to denote that 100ms has passed and control should be run.
int16_t min_speed = 10;
int16_t max_speed = 50;
uint8_t min_pwm = 10;
uint8_t max_pwm = 90;

int32_t speed_errorL = 0;
uint16_t corrected_speedL; // In Terms of Percentage
uint16_t output_compareL; // Compare Value

int32_t speed_errorR = 0;
uint16_t corrected_speedR; // In Terms of Percentage
uint16_t output_compareR; // Compare Value

float min_turn_radius = 0.2;
double kIL = -0.50;
double kIR = -0.05;
double kPL = -0.5;
double kPR = -0.5;



int main() {    /** Main Function ****/
    sysInit();
    GPIOInit();
    ADCInit();
    TimerInit();

    __delay_cycles(24e6);
    GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7); // Turn L Motor On
    GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6); // Turn R Motor On
    while( 1 ) {
        if(runControl){    // If 100 ms has passed
            runControl = 0;    // Reset the 100 ms flag
            // Control routine: Explicitly follow pseudocode from Lab document

            uint16_t desired_speedL;
            uint16_t desired_speedR;
            bool reverse_flagL = false;
            bool reverse_flagR = false;
            bool left_turn_flag = false;
            uint16_t differential_speed;
            // pot_value = 0-1024

            ADC14_toggleConversionTrigger();
            //while(ADC14_isBusy()){}
            uint16_t pot1_val = ADC14_getResult(ADC_MEM9);
            uint16_t pot2_val = ADC14_getResult(ADC_MEM12);
            //printf("%u",pot1_val);
            //printf("%u",pot2_val);

            //uint16_t  pot1_val=800;
            //uint16_t pot2_val=200;
            float adjusted_pot1 = pot1_val/10.24; // scaled to 0-100
            float adjusted_pot2 = pot2_val/10.24; // scaled to 0-100

            //Convert the speed potentiometer measurement to a desired speed
            if (adjusted_pot1 < 45 ){ // below 45%
                desired_speedL = 50-(0.9*adjusted_pot1)+10; // Reverse Direction 10-50
                reverse_flagL = true;

                desired_speedR = 50-(0.9*adjusted_pot1)+10; // Reverse Direction 10-50
                reverse_flagR = true;
            } else if (adjusted_pot1 > 55) { // above 55%
                desired_speedL = (adjusted_pot1-55.0)*(0.9)+10; // Postive Direction 10-50
                reverse_flagL = false;

                desired_speedR = (adjusted_pot1-55.0)*(0.9)+10; // Postive Direction 10-50
                reverse_flagR = false;
            } else { // in middle
                desired_speedL = 0;
                desired_speedR = 0;
            }

            // Convert the turn potentiometer to scale left and right wheel speed
            if (adjusted_pot2 < 35) {
                differential_speed = 80-(80*adjusted_pot2/35);
                left_turn_flag = true;
            } else if (adjusted_pot2 > 65){
                differential_speed = ((adjusted_pot2-65.0)*(80)/35);
                left_turn_flag = false;
            } else {
                differential_speed = 0;
            }
            uint16_t differential_speedL = differential_speed*desired_speedL/100;
            uint16_t differential_speedR = differential_speed*desired_speedR/100;
            //printf("\rDifferntial_speed: %u \t %u\n", differential_speedL, differential_speedR);

            // Ensure the desired speed follows the control specifications below
            // Convert the steering potentiometer measurement to a differential speed
             /*calculate the current speed error from the encoder and |desired wheel speed|
                            (use trendline equation from figure for current speed)
               add current speed error to an "error sum" variable (tracking integral error)
               calculate the corrected speed using the discrete equation above (should be >= 0)
               convert the corrected speed (duty cycle) into a compare value
               enforce minimum and maximum limits on the compare value */


            if(abs(desired_speedL)<min_speed){ // Calculation for Left Wheel
                // set the desired wheel speed and compare value to 0
                Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4, 0 );
            } else {
                if(left_turn_flag) {
                    desired_speedL-=abs(differential_speedL);

                } else {
                    desired_speedL+=abs(differential_speedL);
                }
                if (!reverse_flagL) { // set wheel direction depending on sign (+/-) of desired wheel speed
                    GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN4);
                    corrected_speedL = (1500000.0/TachL_avg - abs(desired_speedL))*kPL+(abs(desired_speedL))+kIL*speed_errorL;
                    output_compareL = 9.59*(corrected_speedL); // (9.59) = 959/100 compare period/percentage
                    Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4, output_compareL);
                } else {
                    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN4);
                    corrected_speedL = (1500000.0/TachL_avg - abs(desired_speedL))*kPL+(abs(desired_speedL))+kIL*speed_errorL;
                    output_compareL = 9.59*(corrected_speedL); // (9.59) = 959/100 compare period/percentage
                    Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4, output_compareL);
                }
                speed_errorL += (1500000.0/TachL_avg - abs(desired_speedL))*0.1;//added discrete time
            }

            if(abs(desired_speedR)<min_speed){ // Calculation for Right Wheel
                // set the desired wheel speed and compare value to 0
                Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3, 0 );
            } else {
                if(left_turn_flag){
                    desired_speedR+=differential_speedR;
                } else {
                    desired_speedR-=differential_speedR;
                }
                if (!reverse_flagR) { // set wheel direction depending on sign (+/-) of desired wheel speed
                    GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN5);
                    corrected_speedR = kPR*(1500000.0/TachR_avg - abs(desired_speedR))+(abs(desired_speedR))+kIR*speed_errorR;
                    output_compareR = 9.59*(corrected_speedR); // (9.59) = 959/100 compare period/percentage
                    Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3, output_compareR);
                } else {
                    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN5);
                    corrected_speedR = kPR*(1500000.0/TachR_avg - abs(desired_speedR))+(abs(desired_speedR))+kIR*speed_errorR;
                    output_compareR = 9.59*(corrected_speedR); // (9.59) = 959/100 compare period/percentage
                    Timer_A_setCompareValue(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_3, output_compareR);
                }
                double errorR = (1500000.0/TachR_avg - abs(desired_speedR))*0.1;//added discrete time
                if (errorR>0){
                    speed_errorR += errorR;
                } else {
                    speed_errorR -= errorR;
                }
                speed_errorR += (1500000.0/TachR_avg - abs(desired_speedR))*0.1;//added discrete time

            }


            //print values
            //printf("\n\rInitial Desired Speed: %u\n", pot1_val);
            //printf("\rAdjusted Potentiometer 1 Value: %.2f\n", adjusted_pot1);
            printf("\rDesired SpeedL: %u\n", desired_speedL); // (after applying minimum/maximum thresholds)  VD
            //printf("\rTachL Encoder Avg: %u\n", TachL_avg); // , (duty cycle units) VR
            //printf("\rSpeed error summation: %u\n", speed_errorL); // (integral error)
            printf("\rCorrected SpeedL: %u\n", corrected_speedL); // , (duty cycle �units�)
            //printf("\rPWM compare value: %u\n\n", output_compareL);
            printf("\rEncoder Measured SpeedL: %u\n", (1500000/TachL_avg));

            //printf("\n\rInitial Desired Speed: %u\n", pot1_val);
            //printf("\rAdjusted Potentiometer 1 Value: %.2f\n", adjusted_pot1);
            printf("\rDesired SpeedR: %u\n", desired_speedR); // (after applying minimum/maximum thresholds)  VD
            //printf("\rTachL Encoder Avg: %u\n", TachL_avg); // , (duty cycle units) VR
            //printf("\rSpeed error summation: %u\n", speed_errorL); // (integral error)
            printf("\rCorrected SpeedR: %u\n", corrected_speedR); // , (duty cycle �units�)
            //printf("\rPWM compare value: %u\n\n", output_compareL);
            printf("\rEncoder Measured SpeedR: %u\n", (1500000/TachR_avg));
        }
    }
}    /** End Main Function ****/   



void ADCInit(){
    // Add your ADC initialization code here.
    //  Don't forget the GPIO, either here or in GPIOInit()!!
    ADC14_enableModule();
    ADC14_initModule(ADC_CLOCKSOURCE_SMCLK, ADC_PREDIVIDER_1 , ADC_DIVIDER_4 , ADC_NOROUTE);
    ADC14_setResolution(ADC_10BIT);

    ADC14_configureConversionMemory(ADC_MEM12 , ADC_VREFPOS_AVCC_VREFNEG_VSS , ADC_INPUT_A12 , false);
    ADC14_configureConversionMemory(ADC_MEM9 , ADC_VREFPOS_AVCC_VREFNEG_VSS , ADC_INPUT_A9 , false);
    ADC14_configureMultiSequenceMode(ADC_MEM9 , ADC_MEM12 , false );

    ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);
    ADC14_enableConversion();
}

void GPIOInit(){
    GPIO_setAsOutputPin(GPIO_PORT_P5,GPIO_PIN4|GPIO_PIN5);   // Motor direction pins
    GPIO_setAsOutputPin(GPIO_PORT_P3,GPIO_PIN6|GPIO_PIN7);   // Motor enable pins
        // Motor PWM pins
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,GPIO_PIN6|GPIO_PIN7,GPIO_PRIMARY_MODULE_FUNCTION);
        // Motor Encoder pins
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P10,GPIO_PIN4|GPIO_PIN5,GPIO_PRIMARY_MODULE_FUNCTION);

    GPIO_setOutputLowOnPin(GPIO_PORT_P5,GPIO_PIN4|GPIO_PIN5);   // Motors set to forward
    GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN6|GPIO_PIN7);   // Motors are OFF

        // Potentiometer pins
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN1, GPIO_TERTIARY_MODULE_FUNCTION); // ADC input
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4, GPIO_TERTIARY_MODULE_FUNCTION); // ADC input
}

void TimerInit(){
    // Configure PWM timer for 24 kHz
    TA0cfg.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    TA0cfg.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    TA0cfg.timerPeriod = 959;
    Timer_A_configureUpMode(TIMER_A0_BASE,&TA0cfg);
    // Configure TA0.CCR3 for PWM output, Left Motor
    TA0_ccr3.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_3;
    TA0_ccr3.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
    TA0_ccr3.compareValue = 0;
    Timer_A_initCompare(TIMER_A0_BASE,&TA0_ccr3);
    // Configure TA0.CCR4 for PWM output, Right Motor
    TA0_ccr4.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_4;
    TA0_ccr4.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
    TA0_ccr4.compareValue = 0;
    Timer_A_initCompare(TIMER_A0_BASE,&TA0_ccr4);
    // Configure Encoder timer in continuous mode
    TA3cfg.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    TA3cfg.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    TA3cfg.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE;
    Timer_A_configureContinuousMode(TIMER_A3_BASE,&TA3cfg);
    // Configure TA3.CCR0 for Encoder measurement, Left Encoder
    TA3_ccr0.captureRegister = TIMER_A_CAPTURECOMPARE_REGISTER_0;
    TA3_ccr0.captureMode = TIMER_A_CAPTUREMODE_RISING_EDGE;
    TA3_ccr0.captureInputSelect = TIMER_A_CAPTURE_INPUTSELECT_CCIxA;
    TA3_ccr0.synchronizeCaptureSource = TIMER_A_CAPTURE_SYNCHRONOUS;
    TA3_ccr0.captureInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE;
    Timer_A_initCapture(TIMER_A3_BASE,&TA3_ccr0);
    // Configure TA3.CCR1 for Encoder measurement, Right Encoder
    TA3_ccr1.captureRegister = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    TA3_ccr1.captureMode = TIMER_A_CAPTUREMODE_RISING_EDGE;
    TA3_ccr1.captureInputSelect = TIMER_A_CAPTURE_INPUTSELECT_CCIxA;
    TA3_ccr1.synchronizeCaptureSource = TIMER_A_CAPTURE_SYNCHRONOUS;
    TA3_ccr1.captureInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE;
    Timer_A_initCapture(TIMER_A3_BASE,&TA3_ccr1);
    // Register the Encoder interrupt
    Timer_A_registerInterrupt(TIMER_A3_BASE,TIMER_A_CCR0_INTERRUPT,Encoder_ISR);
    Timer_A_registerInterrupt(TIMER_A3_BASE,TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT,Encoder_ISR);
    // Configure 10 Hz timer
    TA2cfg.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    TA2cfg.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_64;
    TA2cfg.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE;
    TA2cfg.timerPeriod = 37499;
    Timer_A_configureUpMode(TIMER_A2_BASE,&TA2cfg);
    Timer_A_registerInterrupt(TIMER_A2_BASE,TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT,T2_100ms_ISR);
    // Start all the timers
    Timer_A_startCounter(TIMER_A0_BASE,TIMER_A_UP_MODE);
    Timer_A_startCounter(TIMER_A2_BASE,TIMER_A_UP_MODE);
    Timer_A_startCounter(TIMER_A3_BASE,TIMER_A_CONTINUOUS_MODE);
}

void Encoder_ISR(){
    // If encoder timer has overflowed...
    if(Timer_A_getEnabledInterruptStatus(TIMER_A3_BASE) == TIMER_A_INTERRUPT_PENDING){
        Timer_A_clearInterruptFlag(TIMER_A3_BASE);
        TachR_count += 65536;
        if(TachR_count >= 1e6){ // Enforce a maximum count to TachR so stopped can be detected
            TachR_count = 1e6;
            TachR = 1e6;
        }
        TachL_count += 65536;
        if(TachL_count >= 1e6){ // Enforce a maximum count to TachL so stopped can be detected
            TachL_count = 1e6;
            TachL = 1e6;
        }
    // Otherwise if the Left Encoder triggered...
    }else if(Timer_A_getCaptureCompareEnabledInterruptStatus(TIMER_A3_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0)&TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG){
        Timer_A_clearCaptureCompareInterrupt(TIMER_A3_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
        enc_total_R++;   // Increment the total number of encoder events for the left encoder
        // Calculate and track the encoder count values
        TachR = TachR_count + Timer_A_getCaptureCompareCount(TIMER_A3_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
        TachR_count = -Timer_A_getCaptureCompareCount(TIMER_A3_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
        // Sum values for averaging
        TachR_sum_count++;
        TachR_sum += TachR;
        // If 6 values have been received, average them.
        if(TachR_sum_count == 6){
            TachR_avg = TachR_sum/6;
            TachR_sum_count = 0;
            TachR_sum = 0;
        }
    // Otherwise if the Right Encoder triggered...
    }else if(Timer_A_getCaptureCompareEnabledInterruptStatus(TIMER_A3_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_1)&TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG){
        Timer_A_clearCaptureCompareInterrupt(TIMER_A3_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_1);
        enc_total_L++;
        TachL = TachL_count + Timer_A_getCaptureCompareCount(TIMER_A3_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_1);
        TachL_count = -Timer_A_getCaptureCompareCount(TIMER_A3_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_1);
        TachL_sum_count++;
        TachL_sum += TachL;
        if(TachL_sum_count == 6){
            TachL_avg = TachL_sum/6;
            TachL_sum_count = 0;
            TachL_sum = 0;
        }
    }
}

void T2_100ms_ISR(){
    Timer_A_clearInterruptFlag(TIMER_A2_BASE);
    runControl = 1;
}
