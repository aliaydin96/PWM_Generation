/*
 * PID.c
 *
 *  Created on: Oct 24, 2018
 *      Author: ROG
 */


#include "DSP2833x_Device.h"


int setPoint = 20;
float error = 0;
float controlOutput;
float proportionalGain = 0;
long switchingFrequency=1000;

long deviceClockFrequency = 150000000;
int CLKDIV = 1;
int HSPCLKDIV = 1;
// Prototype statements for functions found within this file.
void Gpio_select(void);
extern void InitSysCtrl(void);
interrupt void cpu_timer0_isr(void);
extern void InitPieCtrl(void);
extern void InitPieVectTable(void);
extern void InitCpuTimers(void);
extern void ConfigCpuTimer(struct CPUTIMER_VARS *, float, float);
extern void display_ADC(unsigned int);
extern void InitAdc(void);
interrupt void adc_isr(void);
interrupt void ePWMA_compare_isr(void);
void Setup_ePWM(void);
int PID_Controller(float);
float lm35;
int processValue;
int sum;
//###########################################################################
//                      main code
//###########################################################################
void main(void)
{

    // binary counter for digital output

    InitSysCtrl();      // Basic Core Initialization
    EALLOW;
    SysCtrlRegs.WDCR = 0x00AF;
    EDIS;


    DINT;               // Disable all interrupts

    Gpio_select();
    Setup_ePWM();
    InitPieCtrl();
    InitPieVectTable();
    InitAdc();

    AdcRegs.ADCTRL1.all = 0;
    AdcRegs.ADCTRL1.bit.SEQ_CASC = 1;    // Dual Sequencer Mode
    AdcRegs.ADCTRL1.bit.CONT_RUN = 0;   // Single Run Mode
    AdcRegs.ADCTRL1.bit.ACQ_PS = 7;     // 8 x ADC-Clock
    AdcRegs.ADCTRL1.bit.CPS = 0;        // divide by 1

    AdcRegs.ADCTRL2.all = 0;
    AdcRegs.ADCTRL2.bit.EPWM_SOCA_SEQ1 = 1;     // ePWM_SOCA trigger
    AdcRegs.ADCTRL2.bit.INT_ENA_SEQ1 = 1; // enable ADC int for seq1
    AdcRegs.ADCTRL2.bit.INT_MOD_SEQ1 = 0; // interrupt after every EOS

    AdcRegs.ADCTRL3.bit.ADCCLKPS = 3;       // set FCLK to 12.5 MHz

    AdcRegs.ADCMAXCONV.all = 0x0001; // 2 conversions

    AdcRegs.ADCCHSELSEQ1.bit.CONV00 = 4; // 1st channel ADCINA0
 //   AdcRegs.ADCCHSELSEQ1.bit.CONV01 = 1;
 //   AdcRegs.ADCCHSELSEQ1.bit.CONV02 = 2;
    EPwm2Regs.TBCTL.all = 0xC030;   // Configure timer control register


    EPwm2Regs.TBPRD = 2999; // TPPRD +1  =  TPWM / (HSPCLKDIV * CLKDIV * TSYSCLK)
                            //           =  20 �s / 6.667 ns
    EPwm2Regs.ETPS.all = 0x0100;            // Configure ADC start by ePWM2

    EPwm2Regs.ETSEL.all = 0x0A00;           // Enable SOCA to ADC

    EALLOW;
    PieVectTable.TINT0 = &cpu_timer0_isr;
    PieVectTable.ADCINT = &adc_isr;
    PieVectTable.EPWM1_INT = &ePWMA_compare_isr;
    EDIS;
    InitCpuTimers();

    ConfigCpuTimer(&CpuTimer0, 150, 100000);

    PieCtrlRegs.PIEIER1.bit.INTx6 = 1;
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;
    IER |= 5;
    EINT;
    ERTM;
    CpuTimer0Regs.TCR.bit.TSS = 0;

    while(1)
    {
          while(CpuTimer0.InterruptCount <10)
          {
               EALLOW;
               SysCtrlRegs.WDKEY = 0x55;   // service WD #1
               SysCtrlRegs.WDKEY = 0xAA;   // service WD #2
               EDIS;
           }
          int i = 0;
          sum = 0;
          for( i=0; i<100; i++ ){
              sum += processValue;
          }
           processValue = sum/100;

           error = setPoint - processValue;
           PID_Controller(error);
           CpuTimer0.InterruptCount = 0;

}}

int PID_Controller(float Error){

}

void Gpio_select(void)
{
    EALLOW;
    GpioCtrlRegs.GPAMUX1.all = 0;       // GPIO15 ... GPIO0 = General Puropse I/O
    GpioCtrlRegs.GPAMUX1.bit.GPIO10 = 1; // ePWM6A active
    GpioCtrlRegs.GPAMUX2.all = 0;       // GPIO31 ... GPIO16 = General Purpose I/O
    GpioCtrlRegs.GPBMUX1.all = 0;       // GPIO47 ... GPIO32 = General Purpose I/O
    GpioCtrlRegs.GPBMUX2.all = 0;       // GPIO63 ... GPIO48 = General Purpose I/O
    GpioCtrlRegs.GPCMUX1.all = 0;       // GPIO79 ... GPIO64 = General Purpose I/O
    GpioCtrlRegs.GPCMUX2.all = 0;       // GPIO87 ... GPIO80 = General Purpose I/O

    GpioCtrlRegs.GPADIR.all = 0;
    GpioCtrlRegs.GPBDIR.all = 0;        // GPIO63-32 as inputs

    GpioCtrlRegs.GPCDIR.all = 0;        // GPIO87-64 as inputs
    EDIS;
}
void Setup_ePWM(void){
    EALLOW;
    SysCtrlRegs.WDKEY = 0xAA;   // service WD #2
    EDIS;

    EPwm6Regs.TBCTL.all = 0;
    EPwm6Regs.TBCTL.bit.CTRMODE = 0;         // Count up and down operation (10) = 2
    EPwm6Regs.AQCTLA.all = 0x0060;          //set ePWM1A to 1 on �CMPA - up match�
                                           //clear ePWM1A on event �CMPA - down match�
    EPwm6Regs.AQCTLB.all = 0x0090;         //clear ePWM1B on �CMPA - up match�
                                            //set ePWM1B to 1 on event �CMPA - down match�
                                            // we made reverse action to obtain complementary wave

    EPwm6Regs.TBCTL.bit.CLKDIV = 0;
    CLKDIV = 1;
    EPwm6Regs.TBCTL.bit.HSPCLKDIV = 0;
    HSPCLKDIV = 1;

    EPwm6Regs.TBPRD = (0.5 * deviceClockFrequency) / (switchingFrequency * CLKDIV * HSPCLKDIV);        //the maximum number for TBPRD is (216 -1) or 65535
    EPwm6Regs.CMPA.half.CMPA = EPwm6Regs.TBPRD / 2; // 50% duty cycle first
    EPwm6Regs.ETSEL.all = 0;
    EPwm6Regs.ETSEL.bit.INTEN = 1;      // interrupt enable for ePWM1
    EPwm6Regs.ETSEL.bit.INTSEL = 4;     // interrupt on CMPA up match
    EPwm6Regs.ETPS.bit.INTPRD = 1;      // interrupt on first event

}
interrupt void ePWMA_compare_isr(void) {
    EALLOW;
    SysCtrlRegs.WDKEY = 0xAA;   // service WD #2
    EDIS;
    /////////////////////////

    EPwm6Regs.TBCTL.all = 0;
    EPwm6Regs.TBCTL.bit.CTRMODE = 2;         // Count up and down operation (10) = 2
    EPwm6Regs.AQCTLA.all = 0x0060;          //set ePWM1A to 1 on �CMPA - up match�
                                           //clear ePWM1A on event �CMPA - down match�

    EPwm6Regs.TBCTL.bit.CLKDIV = 0;
    CLKDIV = 1;
    EPwm6Regs.TBCTL.bit.HSPCLKDIV = 0;
    HSPCLKDIV = 1;

    EPwm6Regs.TBPRD = (0.5 * deviceClockFrequency) / (switchingFrequency * CLKDIV * HSPCLKDIV) ;        //the maximum number for TBPRD is (216 -1) or 65535

    EPwm6Regs.CMPA.half.CMPA = EPwm6Regs.TBPRD ;//

    EPwm6Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = 4;

}
interrupt void cpu_timer0_isr(void){
    CpuTimer0.InterruptCount++;
    EALLOW;
    SysCtrlRegs.WDKEY = 0xAA;
    EDIS;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

interrupt void adc_isr(void){
    lm35 = AdcMirror.ADCRESULT0;
    lm35 = lm35*300/4095;
    processValue = lm35-8;

    AdcRegs.ADCTRL2.bit.RST_SEQ1 = 1;
    AdcRegs.ADCST.bit.INT_SEQ1_CLR = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}



//===========================================================================
// End of SourceCode.
//===========================================================================




