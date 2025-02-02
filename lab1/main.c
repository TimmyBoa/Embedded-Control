//////////////////////////
// Lab 1
// ENGR-2350 S25
// Names: Timothy Pollard & Billy Di
// Section: 3
// Side: B
// Seat: 26
//////////////////////////

#include "engr2350_msp432.h"
#include "lab1lib.h"
void GPIOInit();
void testIO();
void controlSystem();

uint8_t LEDFL = 0; // Two variables to store the state of
uint8_t LEDFR = 0; // the front left/right LEDs (on-car)
int flag=0;
    int main() {    //// Main Function ////
    sysInit(); // Basic car initialization
    initSequence(); // Initializes the lab1Lib Driver
    GPIOInit();
    printf("\r\n\n"
           "===========\r\n"
           "Lab 1 Begin\r\n"
           "===========\r\n");

    while(1){
        //testIO(); // Used in Part A to test the IO
        controlSystem(); // Used in Part B to implement the desired functionality
    }
}    //// Main Function ////


void GPIOInit(){
    //Bump and SS1 switches on Protoboard
    GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN5);
    GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN4);
    GPIO_setAsInputPin(GPIO_PORT_P5, GPIO_PIN6);

    //BiLED PINS
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN0);
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN1);
    //LEDFL AND LEDFR
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN0);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN5);

    //Bump Switches on TI-RLSK
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN0);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN2);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN3);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN5);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN6);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN7);

    //Motor Control Pins
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN6);
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN4);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN5);

}

void testIO(){
    // Add printf statement(s) for testing inputs
    // Example code for testing outputs
    while(1){
        uint8_t cmd = getchar();
        if(cmd == 'a'){
            GPIO_setOutputHighOnPin(GPIO_PORT_P8,GPIO_PIN0);
        }else if(cmd == 'z'){
            GPIO_setOutputLowOnPin(GPIO_PORT_P8,GPIO_PIN0);
        }else if(cmd == 's'){
            GPIO_setOutputHighOnPin(GPIO_PORT_P8,GPIO_PIN5);
        }else if(cmd == 'x'){
            GPIO_setOutputLowOnPin(GPIO_PORT_P8,GPIO_PIN5);
        }else if(cmd == 'q'){
            GPIO_setOutputHighOnPin(GPIO_PORT_P6,GPIO_PIN0);
            GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN1);
        }else if(cmd == 'w'){
            GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN0);
            GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN1);
        }else if(cmd == 'e'){
            GPIO_setOutputHighOnPin(GPIO_PORT_P6,GPIO_PIN1);
            GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN0);
        }

    }
}

void controlSystem(){
    uint8_t PB1,PB2,SS1,BMP0,BMP1,BMP2,BMP3,BMP4,BMP5;
    //Switches on protoboard
    PB1 = GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN5);
    PB2 = GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN4);
    SS1 = GPIO_getInputPinValue(GPIO_PORT_P5, GPIO_PIN6);

    //Bump Switches on board
    BMP0 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN0);
    BMP1 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN2);
    BMP2 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN3);
    BMP3 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN5);
    BMP4 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN6);
    BMP5 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN7);
    __delay_cycles(240e3);
    if (SS1){
        /*
        if (statusSequence()==100 && flag==0) {
            runSequence();
            flag=1;
        } else{
            putchar('a');
            if(statusSequence()==100 && flag==1){
                putchar('b');
                GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN1);
                GPIO_setOutputHighOnPin(GPIO_PORT_P6,GPIO_PIN0);
            } else{
                putchar('c');
                GPIO_setOutputHighOnPin(GPIO_PORT_P6,GPIO_PIN1);
                GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN0);
            }
        }*/
        GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN1);
        GPIO_setOutputHighOnPin(GPIO_PORT_P6,GPIO_PIN0);
        if (statusSequence()==100) {
            if (flag==0){
                runSequence();
                flag=1;
                printf("Run Again\n");
            }
        } else{
            putchar('a');
            if(statusSequence()==100){
                if (flag==1){
                    putchar('b');
                    GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN1);
                    GPIO_setOutputHighOnPin(GPIO_PORT_P6,GPIO_PIN0);
                }
            } else{
                putchar('c');
                GPIO_setOutputHighOnPin(GPIO_PORT_P6,GPIO_PIN1);
                GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN0);
            }
        }
    } else {
        putchar('d');
        flag=0;
        GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN0);
        GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN1);
        if (BMP0==0){
            printf("BMP0");
            recordSegment(2);
            GPIO_toggleOutputOnPin(GPIO_PORT_P8,GPIO_PIN0);
            GPIO_toggleOutputOnPin(GPIO_PORT_P8,GPIO_PIN5);
        } else if (BMP1==0){
            printf("BMP1");
            recordSegment(1);
            GPIO_toggleOutputOnPin(GPIO_PORT_P8,GPIO_PIN0);
            GPIO_toggleOutputOnPin(GPIO_PORT_P8,GPIO_PIN5);
        } else if (BMP2==0){
            printf("BMP2");
            recordSegment(0);
            GPIO_toggleOutputOnPin(GPIO_PORT_P8,GPIO_PIN0);
            GPIO_toggleOutputOnPin(GPIO_PORT_P8,GPIO_PIN5);
        } else if (BMP3==0){
            printf("BMP3");
            recordSegment(127);
            GPIO_toggleOutputOnPin(GPIO_PORT_P8,GPIO_PIN0);
            GPIO_toggleOutputOnPin(GPIO_PORT_P8,GPIO_PIN5);
        } else if (BMP4==0){
            printf("BMP4");
            recordSegment(-1);
            GPIO_toggleOutputOnPin(GPIO_PORT_P8,GPIO_PIN0);
            GPIO_toggleOutputOnPin(GPIO_PORT_P8,GPIO_PIN5);
        } else if (BMP5==0){
            printf("BMP5");
            recordSegment(-2);
            GPIO_toggleOutputOnPin(GPIO_PORT_P8,GPIO_PIN0);
            GPIO_toggleOutputOnPin(GPIO_PORT_P8,GPIO_PIN5);
        } else if (PB1==1){
            printf("PB1");
            popSegment();
        } else if (PB2==1){
            printf("PB2");
            clearSequence();
        }
      while(BMP0==0 || BMP1==0 || BMP2==0 || BMP3==0 || BMP4==0 || BMP5==0 || PB1==1  || PB2==1){
          //Input ProtoBoard
          PB1 = GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN5);
          PB2 = GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN4);
          SS1 = GPIO_getInputPinValue(GPIO_PORT_P5, GPIO_PIN6);

          //Bump Switches on board
          BMP0 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN0);
          BMP1 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN2);
          BMP2 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN3);
          BMP3 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN5);
          BMP4 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN6);
          BMP5 = GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN7);
          __delay_cycles(240e3);
      }
    }
    /*
    BMP0 adds the â€œturn right 90Â°â€� segment

    BMP1 adds the â€œturn right 45Â°â€� segment

    BMP2 adds the â€œdrive forwardâ€� segment

    BMP3 adds the â€œstop for 1sâ€� segment

    BMP4 adds the â€œturn left 45Â°â€� segment

    BMP5 adds the â€œturn left 90Â°â€� segment
    */
}
