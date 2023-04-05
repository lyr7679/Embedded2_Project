
//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red LED:
//   PF1 drives an NPN transistor that powers the red LED
// Green LED:
//   PF3 drives an NPN transistor that powers the green LED
// Pushbutton:
//   SW1 pulls pin PF4 low (internal pull-up is used)


//green led is gp3, redled gp2, pushbutton gp1, int gp0

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "gpio.h"
#include "adc0.h"
#include "uart0.h"
#include "nvic.h"
#include "wait.h"

//LEDS and pushbutton
#define RED_LED PORTF,1
#define BLUE_LED PORTF,2
#define GREEN_LED PORTF,3

//Analog inputs
#define MIC1 PORTE,2    //AIN1
#define MIC2 PORTE,1    //AIN2
#define MIC3 PORTD,3    //AIN4

#define SS1_VECTOR 31

//size of circular buffer for avg
#define N 10

//checking to make sure interrupt functions correctly
int counter = 0;
uint8_t avg_phase = 0;

//raw values coming in from read adc function
int mic1_raw = 0;
int mic2_raw = 0;
int mic3_raw = 0;

//circular buffer variables for each mic
uint8_t index1 = 0, index2 = 0, index3 = 0;
uint32_t sum1 = 0, sum2 = 0, sum3 = 0;
uint32_t threshold1 = 100, threshold2 = 100, threshold3 = 100;
uint32_t mic1_avg = 0, mic2_avg = 0, mic3_avg = 0;
uint32_t avg_arr1[N] = {0};
uint32_t avg_arr2[N] = {0};
uint32_t avg_arr3[N] = {0};

bool start_delay = false;
int time_delay_arr[3] = {-1, -1, -1};
uint32_t delay_time = 0;
uint8_t delay_count = 0;

uint32_t aoa_val = 0;

//UI variables
USER_DATA data;
uint32_t time_constant = 0;
uint32_t backoff_val = 0;
uint32_t holdoff_val = 0;
uint32_t hysteresis_val = 0;
bool displayAoa = false;
bool displayTdoa = false;
bool displayFail = false;


uint32_t delay_after = 0;
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


void readIsr()
{
    if(counter == 333333)
    {
        togglePinValue(RED_LED);
        counter = 0;
    }

    counter = counter + 1;

    char str[80];

    //read and store adc values
//    mic1_raw = readAdc0Ss1();
//    mic2_raw = readAdc0Ss1();
//    mic3_raw = readAdc0Ss1();
    mic3_raw = readAdc0Ss1();
    mic1_raw = readAdc0Ss1();
    mic2_raw = readAdc0Ss1();

    if(mic3_raw > 100 || mic2_raw > 100 || mic1_raw > 100)
    {
//        snprintf(str, sizeof(str), "mic1 raw: %d mic2 raw: %d  mic3 raw: %d\n\n", mic1_raw, mic2_raw, mic3_raw);
//        putsUart0(str);

//        snprintf(str, sizeof(str), "\navg1: %d    avg2: %d     avg3:  %d\n", mic1_avg, mic2_avg, mic3_avg);
//        putsUart0(str);
    }

    if(delay_count == 0)
    {
        threshold1 = 50;
        threshold2 = 50;
        threshold3 = 50;
    }
    else
    {
        threshold1 = 150;
        threshold2 = 150;
        threshold3 = 150;
    }

    if(mic1_raw > (mic1_avg + threshold1) || mic2_raw > (mic2_avg + threshold2) || mic3_raw > (mic3_avg + threshold3))
    {
        start_delay = true;
    }

    if(delay_after != 0)
    {
        start_delay = false;
        delay_after--;
    }

    if(start_delay == true)
    {
        snprintf(str, sizeof(str), "mic1 raw: %d mic2 raw: %d  mic3 raw: %d\n\n", mic1_raw, mic2_raw, mic3_raw);
        putsUart0(str);

//        snprintf(str, sizeof(str), "\navg1: %d    avg2: %d     avg3:  %d\n", mic1_avg, mic2_avg, mic3_avg);
//        putsUart0(str);
    }

    //can split average calculation to be done every 4th time
    //or split calculating avg for only ONE microphone each time
    //this lessens load + amount of math needed per interrupt
//    //mic1 circular buffer
    if(avg_phase == 0 && !start_delay)
    {

        sum1 -= avg_arr1[index1];  //oldest
        sum1 += mic1_raw;     //newest
        avg_arr1[index1] = mic1_raw;
        index1 = (index1 + 1) % N;
        mic1_avg = sum1 / N;

        avg_phase++;
    }

//
//    //mic2 circular buffer
    if(avg_phase == 1 && !start_delay)
    {
        if(mic2_raw > 100)
        {
           printf("test");
        }
        sum2 -= avg_arr2[index2];  //oldest
        sum2 += mic2_raw;     //newest
        avg_arr2[index2] = mic2_raw;
        index2 = (index2 + 1) % N;
        mic2_avg = sum2 / N;

        avg_phase++;
    }
//
//    //mic3 circular buffer
    if(avg_phase == 2 && !start_delay)
    {
        sum3 -= avg_arr3[index3];  //oldest
        sum3 += mic3_raw;     //newest
        avg_arr3[index3] = mic3_raw;
        index3 = (index3 + 1) % N;
        mic3_avg = sum3 / N;

        avg_phase = 0;
    }

//    snprintf(str, sizeof(str), "mic1 avg: %d    mic2 avg: %d    mic3 avg:    %d\n\n", mic1_avg, mic2_avg, mic3_avg);
//    putsUart0(str);

    if(mic1_raw > mic1_avg + threshold1 && delay_count <= 2 && time_delay_arr[0] == -1)
    {
        time_delay_arr[0] = delay_time;
        delay_count++;
    }

    if(start_delay && delay_count != 0)
        delay_time++;

    if(mic2_raw > mic2_avg + threshold2 && delay_count <= 2 && time_delay_arr[1] == -1)
    {
        time_delay_arr[1] = delay_time;
        delay_count++;
    }

    if(start_delay && delay_count != 0)
        delay_time++;

    if(mic3_raw > mic3_avg + threshold3 && delay_count <= 2 && time_delay_arr[2] == -1)
    {
        time_delay_arr[2] = delay_time;
        delay_count++;
    }

    if(start_delay && delay_count != 0)
        delay_time++;

    //if > 2, means we already crossed threshold for 3 mics
    //we can reset delay_time to zero
    // we should also take these actions if delay time exceeds a certain point
    if(delay_count > 2 || delay_time > 500)
    {
        snprintf(str, sizeof(str), "\navg1: %d    avg2: %d     avg3:  %d\n", mic1_avg, mic2_avg, mic3_avg);
        putsUart0(str);

        if(delay_time > 500)
        {
            snprintf(str, sizeof(str), "\nFAIL\n");
            putsUart0(str);
        }

        start_delay = false;
        delay_time = 0;
        delay_count = 0;
        delay_after = 5000;

        //print statement to check against raw value print
        snprintf(str, sizeof(str), "\n\ntd 1: %d td 2: %d  td 3: %d\n\n", time_delay_arr[0], time_delay_arr[1], time_delay_arr[2]);
        putsUart0(str);


        time_delay_arr[0] = -1; time_delay_arr[1] = -1; time_delay_arr[2] = -1;

//        can split off for printing data if it was valid or invalid

    }
    //waitMicrosecond(1000000);
    ADC0_PSSI_R |= ADC_PSSI_SS1;
    //clear interrupt
    ADC0_ISC_R = ADC_ISC_IN1;
}

// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    SYSCTL_RCGCHIB_R = SYSCTL_RCGCHIB_R0;
    _delay_cycles(3);

    // Enable clocks
    enablePort(PORTF);
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(BLUE_LED);
    selectPinPushPullOutput(GREEN_LED);

    enablePort(PORTD);
    selectPinAnalogInput(MIC3);

    enablePort(PORTE);
    selectPinAnalogInput(MIC1);
    selectPinAnalogInput(MIC2);

    enableNvicInterrupt(SS1_VECTOR);

}


//UI
void processShell()
{
    char str[80];
    bool knownCommand = false;
    getsUart0(&data);
    parseFields(&data);

    if(isCommand(&data, "reset", 0))
    {
        NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
        knownCommand = true;
    }

    if(isCommand(&data, "average", 0))
    {
        //avg value of each mic in DAC and SPL (dB) units
        snprintf(str, sizeof(str), "Microphone 1 average: %d \nMicrophone 2 average: %d \nMicrophone 3 average: %d \n\n", mic1_avg, mic2_avg, mic3_avg);
        putsUart0(str);

        knownCommand = true;
    }

    if(isCommand(&data, "tc", 0))
    {
        if(data.fieldCount > 1)
        {
            time_constant = getFieldInteger(&data, 1);
        }
        knownCommand = true;
    }

    if(isCommand(&data, "backoff", 0))
    {
        if(data.fieldCount > 1)
        {
            backoff_val = getFieldInteger(&data, 1);
        }
        knownCommand = true;
    }

    if(isCommand(&data, "holdoff", 0))
    {
        if(data.fieldCount > 1)
        {
            holdoff_val = getFieldInteger(&data, 1);
        }
        knownCommand = true;
    }

    if(isCommand(&data, "hysteresis", 0))
    {
        if(data.fieldCount > 1)
        {
            hysteresis_val = getFieldInteger(&data, 1);
        }
        knownCommand = true;
    }

    if(isCommand(&data, "aoa", 0))
    {
        snprintf(str, sizeof(str), "Current Angle of Arrival: %d (theta)\n\n", aoa_val);
        putsUart0(str);
        knownCommand = true;
    }

    if(isCommand(&data, "aoa", 1))
    {
       displayAoa = true;
       knownCommand = true;
    }

    if(isCommand(&data, "tdoa", 1))
    {
        //tdoa_display = getFieldString(&data, 1);

        if(strCmp(&data, "ON"))
            displayTdoa = true;
        else if(strCmp(&data, "OFF"))
            displayTdoa = false;
        knownCommand = true;
    }

    if(isCommand(&data, "fail", 1))
    {
        //fail_display = getFieldString(&data, 1);

        if(strCmp(&data, "ON"))
            displayFail = true;
        else if(strCmp(&data, "OFF"))
            displayFail = false;
        knownCommand = true;
    }

    if (knownCommand == false)
        putsUart0("Invalid command\n");
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------


int main(void)
{
    // Initialize hardware
    initHw();
    initUart0();
    initAdc0Ss1();

    int count = 0;

    // Setup UART0 baud rate
    setUart0BaudRate(115200, 40e6);

    setPinValue(RED_LED, 0);

    //set analog inputs ( + hardware sampling rate?)
    setAdc0Ss1Mux();
    setAdc0Ss1Log2AverageCount(0);
    ADC0_PSSI_R |= ADC_PSSI_SS1;                     // set start bit

    while(true)
    {
//        mic1_raw = readAdc0Ss1();
//        if(count == 3)
//        {
//            count = 0;
//            ADC0_PSSI_R |= ADC_PSSI_SS1;                     // set start bit
//        }
//        count++;
        //waitMicrosecond(500000);
        //ADC0_PSSI_R |= ADC_PSSI_SS1;

        processShell();
    }
}
