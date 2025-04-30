
////////////////////////////////////////////////////////////////////////
//** ENGR-2350 Lab 2
//** NAMEs: Timothy Pollard, Billy Di
//** RINs:  662084381,       662056905
//** Section: 2
////////////////////////////////////////////////////////////////////////

#include "engr2350_msp432.h"

// Add function prototypes here, as needed.
void GPIOInit();
void timerInit();
uint8_t readBumpers();
void setRGB(int8_t color);
uint32_t timer = TIMER_A2_BASE;
uint32_t timer_count;
uint8_t checkGuess(int8_t *sol,int8_t *guess,int8_t *result);
void printResult(int8_t *guess,int8_t *result);
void Timer_ISR();
// Add global variables here, as needed.
const uint8_t Lpattern = 4; // The pattern length. Could be changed if desired.
uint8_t guessCount = 0;
int8_t entry = -1;
uint8_t entryCount = 0;
int8_t patternGuess[] = {0,0,0,0};
Timer_A_UpModeConfig timerConfig = {
    .clockSource = TIMER_A_CLOCKSOURCE_SMCLK,
    .clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_64,
    .timerPeriod = 37500,
    .timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_ENABLE,
    .captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,
    .timerClear = TIMER_A_DO_CLEAR
};
int8_t colors[] = {0, 0, 0, 0};//Array of Colors

int main() {    //// Main Function ////
    
    sysInit();
    GPIOInit();
    timerInit();
    // Place initialization code (or run-once) code here
    bool stay = true;
    int8_t correctAnswer[4];
    srand(time(NULL));
    int i=0;
    for(i =0; i<4; i++){
        correctAnswer[i]=rand() % 6;
        printf("%u", correctAnswer[i]);
    }

    Timer_A_enableInterrupt(timer);
    while(1)
    {
        //Start Up Sequence indicated with flag "stay"
        while(stay){
            uint8_t PushButton = GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN7);
            __delay_cycles(240e3);
            int k = readBumpers();
            if(PushButton){
                printf("\n\rWelcome To Colordle\n\r");
                printf("This is game is played by guessing the color sequence.\n\r");
                printf("Each push button represents a different color\n\r");
                printf("after a guess of the sequence the corresponding flash to that entry will be flashed\n\r");
                printf("Red: Color not in sequence\n\r");
                printf("Yellow: Color in sequence out of order\n\r");
                printf("Green: Color in sequence and the correct spot\n\r");
                //Turn White for 2.5 seconds
                timer_count=0;
                Timer_A_clearTimer(timer);
                while(timer_count<25){
                    setRGB(6);
                }
                setRGB(-1);
                guessCount=0;
                entryCount=0;
                stay = false;
                timer_count=0;
                break;
            }
        }

        // Too many guesses
        if(guessCount > 5) {
           printf("Game Over! Too many guesses :(");
           stay = true;
        }

        // Been more than 30 seconds(time out)
        if(timer_count>300){
            //Clear Guess
            printf("Timer Reset");
            for(i = 0; i<Lpattern; i++){
                patternGuess[i] = 0;
            }

            //Force Guess Check
            int8_t result[Lpattern] = {0,0,0,0};
            uint8_t correctPositions = checkGuess(correctAnswer, patternGuess, result);
            printResult(patternGuess, result);
            for(i=0; i<Lpattern; i++){
                timer_count=0;
                Timer_A_clearTimer(timer);
                while(timer_count<10){
                    setRGB(result[i]);
                }
                timer_count=0;
                Timer_A_clearTimer(timer);
                while(timer_count<5){
                    setRGB(-1);
                }
            }
            guessCount++;

        }

        //No debounce since only need to detect if it is pressed not consecutive presses
        uint8_t resetButton = GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN7);
        // Reset if button pressed
        if(resetButton) {
            printf("Reset Button:\n\r");
            for(i = 0; i < Lpattern; i++) {
                patternGuess[i] = 0;
            }
            timer_count=0;
            Timer_A_clearTimer(timer);
            while(timer_count<5){
                setRGB(6);
            }
            setRGB(-1);
            entryCount = 0;
            timer_count=0;
            Timer_A_clearTimer(timer);
        }

        // Guess for one color has been made and no more than 3 entries already
        entry = readBumpers();
        if (entry!=-1 && entryCount<Lpattern){
            printf("Entry:");
            //add guess to array
            patternGuess[entryCount]=entry;

            //wait for 0.5 seconds without resetting timer to prevent new entry and display LED
            uint8_t start = timer_count;
            while ((timer_count - start) < 5) {
                //Do Nothing
            }
            setRGB(-1);
            entryCount++;
        }

        // Guess for sequence has been made
        if(entryCount==4){
            printf("Guess:");
            int8_t result[Lpattern] = {0,0,0,0};
            uint8_t correctPositions = checkGuess(correctAnswer, patternGuess, result);
            printResult(patternGuess, result);
            for(i=0; i<Lpattern; i++){
                //Blink result color
                timer_count=0;
                Timer_A_clearTimer(timer);
                while(timer_count<10){
                    setRGB(result[i]);
                }
                timer_count=0;
                Timer_A_clearTimer(timer);
                while(timer_count<5){
                    setRGB(-1);
                }
            }

            if(correctPositions==Lpattern){ // Too many guesses
                //Blink Green
                timer_count=0;
                Timer_A_clearTimer(timer);
                while(timer_count<50){
                    if(timer_count%10){
                        setRGB(1);
                    } else {
                        setRGB(-1);
                    }
                }
                printf("\n\rCongrats you won!!!!!!!!!!!!!\n\r");
                setRGB(-1);
                stay = true;
            }
            guessCount++;
            entryCount=0;
        }
    }   
}    //// Main Function ////  

// GPIO Intilization
void GPIOInit(){
    // Button Inputs
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN0);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN2);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN3);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN5);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN6);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN7);
    GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN7);

    // LED RGB Output
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN1);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN2);
}

//Timer Intilization
void timerInit(){
    Timer_A_configureUpMode(timer, &timerConfig);
    Timer_A_startCounter(timer, TIMER_A_UP_MODE);
    Timer_A_registerInterrupt(TIMER_A2_BASE,TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT,Timer_ISR);
    Interrupt_enableMaster();
}

// Timer Interrupt
void Timer_ISR(){
    Timer_A_clearInterruptFlag(TIMER_A2_BASE);
    timer_count++;
}

//Reads in each bumpers and checks for a press and set LED color accordingly
uint8_t readBumpers() {
    __delay_cycles(240e3);
    uint8_t PB1 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN0);
    uint8_t PB2 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN2);
    uint8_t PB3 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN3);
    uint8_t PB4 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN5);
    uint8_t PB5 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN6);
    uint8_t PB6 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN7);
    __delay_cycles(240e3);
    if (!PB1){
        setRGB(0);
        putchar('0');
        return 0;
    } else if (!PB2){
        setRGB(1);
        putchar('1');
        return 1;
    } else if (!PB3){
        setRGB(2);
        putchar('2');
        return 2;
    } else if (!PB4){
        setRGB(3);
        putchar('3');
        return 3;
    } else if (!PB5){
        setRGB(4);
        putchar('4');
        return 4;
    } else if (!PB6){
        setRGB(5);
        putchar('5');
        return 5;
    } else{
        //putchar('x');
        return -1;
    }
}

// Sets LED a specific color based upon Input
void setRGB(int8_t color) {
    /* Designation of Each Color:
    Off -1
    BMP0 Red 0
    BMP1 Green 1
    BMP2 Blue 2
    BMP3 Yellow 3
    BMP4 Magenta 4
    BMP5 Cyan 5
    White 6  */
    if (color==0){
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
    } else if (color==1){
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
    } else if (color==2){
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);

    } else if (color==3){
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);

    } else if (color==4){
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
    } else if (color==5){
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);

    } else if (color==6){
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN0);
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN1);
        GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
    }else if (color==-1){
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0);
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN1);
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
    }
}

/**
 * checkGuess is used to check the player's guess against the solution
 * and produce the associated correct positions, incorrect positions, and
 * incorrect colors.
 *
 *  !!! WARNING !!! All of these inputs are expected to be pointers. Arrays are
 *              !!! technically pointers already! They should not have an & in
 *              !!! front of them when passed into the function.
 * Input Parameters:
 *      int8_t * sol: A 4-element array that stores the game solution (input)
 *      int8_t * guess: A 4-element array that stores the player's guess (input)
 *      int8_t * result: A 4-element array that stores the guess correctness result
 *                    This array is really an output of this function. It is
 *                    modified within the function, with the changes persistent
 *                    after the function is complete.
 *                    This array will only have the values of:
 *                      0: Red - Incorrect color
 *                      1: Green - Correct color and position
 *                      3: Yellow - Correct color, incorrect position
 * Outputs:
 *      uint8_t - the number of correct positions. This may be used to determine
 *                is the guess was correct.
 */

uint8_t checkGuess(int8_t *sol,int8_t *guess,int8_t *result) {
    uint8_t _i,_j; // Loop variables. underscores added to avoid conflict with possible globals.
    uint8_t matched[4]; // Array to store if a color in the answer has been matched yet or not
    for(_i=0;_i<Lpattern;_i++){ // set default values of arrays
        result[_i] = 0; // Answer is incorrect (RED)
        matched[_i] = 0; // Guess position is not used yet
    }
    uint8_t Ncorrect = 0; // Number of positions correct.
    // Fist loop through and find corrects
    for(_i=0;_i<Lpattern;_i++){
        if(sol[_i] == guess[_i]){ // If the guess and answer match...
            Ncorrect++; // Increment number of correct guesses
            result[_i] = 1; // 1 for green
            matched[_i] = 1; // 1 for used (can't compare this position again)
        }
    }
    // Now check for correct color, incorrect position
    for(_i=0;_i<Lpattern;_i++){ // Loop through guess positions
        if(result[_i] == 1) continue; // If this position is marked correct, skip it
        for(_j=0;_j<Lpattern;_j++){ // Loop through answer positions, looking for the same color
         // if(i==j) continue; // If checking the same position, skip. This isn't necessary as it would correspond
                               // the correct case and would be skipped by the "checked" array anyway
            if(matched[_j]) continue; // If this answer color is already taken by a correct or close, skip it
            if(guess[_i] == sol[_j]){ // If the colors are the same (correct color, incorrect position)
                result[_i] = 3; // 3 for yellow
                matched[_j] = 1; // 1 for used (can't compare this position again)
            }
        }
    }
    return Ncorrect; // return number of correct positions
}

/*
 * printResult will take the players guess and the checked result and print them
 * in the necessary format on the terminal. The colors in the player's guess will be
 * printed first, using the first letter of each color. Afterwards the result of
 * the guess is printed using the characters:
 *              $ - correct color and position (Green result)
 *              O - correct color, incorrect position (Yellow result)
 *              X - incorrect color (Red result)
 *
 *  !!! WARNING !!! Both of these inputs are expected to be pointers. Arrays are
 *                  technically pointers already! They should not have an & in
 *                  front of them when passed into the function.
 * Input Parameters:
 *      int8_t * guess: A 4-element array that stores the player's guess (input)
 *      int8_t * result: A 4-element array that stores the guess correctness result
 */
void printResult(int8_t *guess,int8_t *result){
    uint8_t _i = 0; // loop variable
    for(_i=0;_i<Lpattern;_i++){
        switch(guess[_i]){
        case 0: putchar('R'); break;
        case 1: putchar('G'); break;
        case 2: putchar('B'); break;
        case 3: putchar('Y'); break;
        case 4: putchar('M'); break;
        case 5: putchar('C'); break;
        }
    }
    putchar(' '); // put a space in
    for(_i=0;_i<Lpattern;_i++){
        switch(result[_i]){
        case 0: putchar('X'); break;
        case 3: putchar('O'); break;
        case 1: putchar('$'); break;
        }
    }
    putchar('\r');putchar('\n');
}

// Add interrupt functions last so they are easy to find
