// ADC0 Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// ADC0 SS1

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "adc0.h"

#define ADC_CTL_DITHER          0x00000040

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initAdc0Ss1()
{
    // Enable clocks
    SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0;
    _delay_cycles(16);

    // Configure ADC
    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN1;                // disable sample sequencer 1 (SS1) for programming
    ADC0_CC_R = ADC_CC_CS_SYSPLL;                    // select PLL as the time base (not needed, since default value)
    ADC0_PC_R = ADC_PC_SR_1M;                        // select 1Msps rate
    ADC0_EMUX_R = ADC_EMUX_EM1_PROCESSOR;            // select SS1 bit in ADCPSSI as trigger
    ADC0_SSCTL1_R = ADC_SSCTL1_END2;                 // mark third sample as the end
    //turn on interrupt bit?
    ADC0_IM_R = ADC_IM_MASK1;
    ADC0_SSCTL1_R = ADC_SSCTL1_IE2;
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN1;                 // enable SS1 for operation
}

// Set SS1 input sample average count
void setAdc0Ss1Log2AverageCount(uint8_t log2AverageCount)
{
    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN1;                // disable sample sequencer 1 (SS1) for programming
    ADC0_SAC_R = log2AverageCount;                   // sample HW averaging
    if (log2AverageCount == 0)
        ADC0_CTL_R &= ~ADC_CTL_DITHER;               // turn-off dithering if no averaging
    else
        ADC0_CTL_R |= ADC_CTL_DITHER;                // turn-on dithering if averaging
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN1;                 // enable SS1 for operation
}

// Set SS1 analog input
// we're using AIN1, AIN2, AIN4
void setAdc0Ss1Mux()
{
    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN1;                // disable sample sequencer 1 (SS1) for programming
    ADC0_SSMUX1_R = 0x1 ;
    ADC0_SSMUX1_R |= 0x20 | 0x400;                           // Set analog input for 3 samples
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN1;                 // enable SS1 for operation
}

// Request and read one sample from SS1
int16_t readAdc0Ss1()
{
    //ADC0_PSSI_R |= ADC_PSSI_SS1;                     // set start bit
    while (ADC0_ACTSS_R & ADC_ACTSS_BUSY);           // wait until SS1 is not busy
    while (ADC0_SSFSTAT1_R & ADC_SSFSTAT1_EMPTY);
    return ADC0_SSFIFO1_R;                           // get single result from the FIFO
}
