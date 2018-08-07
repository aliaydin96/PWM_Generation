/*
 * sinus2.c
 *
 *  Created on: Aug 1, 2018
 *      Author: ROG
 */

#include "DSP2833x_Device.h"
#include <math.h>
# define PI 3.14159265358979323846  /* pi */
/////////////////////////////////////////////////////////////////////////////////////////////////
                                                                                               //
////////////////////////////////////////////////////////////////////////////////////////////   //
                                                                                          //   //
 long switchingFrequency = 15000;      // switching frequency in Hz                       //   //
                                                                                          //   //
 double fundamentalSinusoidalFrequency = 50;  // sinusoidal output frequency in Hz        //   //
                                                                                          //   //
 float phaseAngle = 0;   // starting angle of sinus                                       //   //
                                                                                          //   //
 float fundamentalSinusoidalMagnitude = 2;//Peaktopeak Value of output sinus waveform   //   //
                                                                                          //   //
 ///////////////////////////////////////////////////////////////////////////////////////////   //
                                                                                               //
 ////////////////////////////////////////////////////////////////////////////////////////////////
 long deviceClockFrequency = 150000000; // f28335 clock frequency in hz

 float maximumDeviceVoltage = 3.3;  // maximum voltage provided by device pin which 3.3V

 double counter = 0;
 double counter2 = 0;
 double counter3 = 0;

 float frequencyModulationRatio =0;
 float magnitudeModulationRatio = 0;

 float sinus=0;
 float sinus2=0;
 float sinus3=0;
 int CLKDIV = 0;
 int HSPCLKDIV = 0;


// Prototype statements for functions found within this file.
void Gpio_select(void);
extern void InitSysCtrl(void);
extern void InitPieCtrl(void);
extern void InitPieVectTable(void);
interrupt void ePWMA_compare_isr(void);
void Setup_ePWM(void);

//###########################################################################
//                      main code
//###########################################################################

void main(void)
{

        InitSysCtrl();      // Basic Core Initialization
        EALLOW;
        SysCtrlRegs.WDCR = 0x00AF;
        EDIS;

        DINT;               // Disable all interrupts

        Gpio_select();      // GPIO9,GPIO11,GPIO34 and GPIO49 as output (LEDs @ peripheral explorer)
        Setup_ePWM();
        InitPieCtrl();

        InitPieVectTable();

        EALLOW;
        PieVectTable.EPWM1_INT = &ePWMA_compare_isr;
        EDIS;

        PieCtrlRegs.PIEIER3.bit.INTx1 = 1;
        IER |= 4;
        EINT;
        ERTM;

        frequencyModulationRatio = fundamentalSinusoidalFrequency / switchingFrequency;

        magnitudeModulationRatio = fundamentalSinusoidalMagnitude / maximumDeviceVoltage;

        counter = phaseAngle;
        counter2 = 1 / (3 * frequencyModulationRatio) + phaseAngle;  // to obtain 120 degree phase shift
        counter3 = 2 / (3 * frequencyModulationRatio) + phaseAngle;      // to obtain 240 degree  phase shift

        while(1)
        {
                EALLOW;
                SysCtrlRegs.WDKEY = 0x55;   // service WD #1
                EDIS;
        }
}


void Gpio_select(void)
{
    EALLOW;
    GpioCtrlRegs.GPAMUX1.all = 0;       // GPIO15 ... GPIO0 = General Puropse I/O
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1; // ePWM1A active
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1; // ePWM2A active
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1; // ePWM3A active
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

    EPwm1Regs.TBCTL.all = 0;
    EPwm1Regs.TBCTL.bit.CTRMODE = 2;         // Count up and down operation (10) = 2
    EPwm1Regs.AQCTLA.all = 0x0060;          //set ePWM1A to 1 on “CMPA - up match”
                                           //clear ePWM1A on event “CMPA - down match”

    EPwm2Regs.TBCTL.all = 0;
    EPwm2Regs.TBCTL.bit.CTRMODE = 2;         // Count up and down operation (10) = 2
    EPwm2Regs.AQCTLA.all = 0x0060;          //set ePWM1A to 1 on “CMPA - up match”
                                           //clear ePWM1A on event “CMPA - down match”
    EPwm3Regs.TBCTL.all = 0;
    EPwm3Regs.TBCTL.bit.CTRMODE = 2;         // Count up and down operation (10) = 2
    EPwm3Regs.AQCTLA.all = 0x0060;          //set ePWM1A to 1 on “CMPA - up match”
                                           //clear ePWM1A on event “CMPA - down match”

    if ((75000000 >= switchingFrequency) && (switchingFrequency >= 1200)){
        EPwm1Regs.TBCTL.bit.CLKDIV = 0;
        EPwm2Regs.TBCTL.bit.CLKDIV = 0;
        EPwm3Regs.TBCTL.bit.CLKDIV = 0;
        CLKDIV = 1;
        EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;
        EPwm2Regs.TBCTL.bit.HSPCLKDIV = 0;
        EPwm3Regs.TBCTL.bit.HSPCLKDIV = 0;
        HSPCLKDIV = 1;
    }
    else if((1200 > switchingFrequency) && (switchingFrequency > 0)){
        EPwm1Regs.TBCTL.bit.CLKDIV = 7;
        EPwm2Regs.TBCTL.bit.CLKDIV = 7;
        EPwm3Regs.TBCTL.bit.CLKDIV = 7;
        CLKDIV = 14;
        EPwm1Regs.TBCTL.bit.HSPCLKDIV = 7;
        EPwm2Regs.TBCTL.bit.HSPCLKDIV = 7;
        EPwm3Regs.TBCTL.bit.HSPCLKDIV = 7;
        HSPCLKDIV = 128;
    }

    EPwm1Regs.TBPRD = (0.5 * deviceClockFrequency) / (switchingFrequency * CLKDIV * HSPCLKDIV);        //the maximum number for TBPRD is (216 -1) or 65535
    EPwm2Regs.TBPRD = (0.5 * deviceClockFrequency) / (switchingFrequency * CLKDIV * HSPCLKDIV);
    EPwm3Regs.TBPRD = (0.5 * deviceClockFrequency) / (switchingFrequency * CLKDIV * HSPCLKDIV);

    EPwm1Regs.CMPA.half.CMPA = EPwm1Regs.TBPRD / 2; // 50% duty cycle first
    EPwm2Regs.CMPA.half.CMPA = EPwm2Regs.TBPRD / 2; // 50% duty cycle first
    EPwm3Regs.CMPA.half.CMPA = EPwm3Regs.TBPRD / 2; // 50% duty cycle first

    EPwm1Regs.ETSEL.all = 0;
    EPwm1Regs.ETSEL.bit.INTEN = 1;      // interrupt enable for ePWM1
    EPwm1Regs.ETSEL.bit.INTSEL = 4;     // interrupt on CMPA up match
    EPwm1Regs.ETPS.bit.INTPRD = 1;      // interrupt on first event

    EPwm2Regs.ETSEL.all = 0;
    EPwm2Regs.ETSEL.bit.INTEN = 1;      // interrupt enable for ePWM1
    EPwm2Regs.ETSEL.bit.INTSEL = 4;     // interrupt on CMPA up match
    EPwm2Regs.ETPS.bit.INTPRD = 1;      // interrupt on first event

    EPwm3Regs.ETSEL.all = 0;
    EPwm3Regs.ETSEL.bit.INTEN = 1;      // interrupt enable for ePWM1
    EPwm3Regs.ETSEL.bit.INTSEL = 4;     // interrupt on CMPA up match
    EPwm3Regs.ETPS.bit.INTPRD = 1;      // interrupt on first event

}

interrupt void ePWMA_compare_isr(void) {
    EALLOW;
    SysCtrlRegs.WDKEY = 0xAA;   // service WD #2
    EDIS;

    sinus = (sin(2 * PI * frequencyModulationRatio * counter) + 1) / 2.0;

    EPwm1Regs.CMPA.half.CMPA = EPwm1Regs.TBPRD - magnitudeModulationRatio * EPwm1Regs.TBPRD * sinus - 1;

    counter +=1;
    if( counter > ((1 / frequencyModulationRatio)-1)) counter = 0;

    sinus2 = (sin(2 * PI * frequencyModulationRatio * counter2) + 1) / 2.0;

    EPwm2Regs.CMPA.half.CMPA = EPwm2Regs.TBPRD - magnitudeModulationRatio * EPwm2Regs.TBPRD * sinus2 - 1;
    counter2 +=1;
    if( counter2 > ((1 / frequencyModulationRatio) - 1)) counter2 = 0;

    sinus3 = (sin(2 * PI * frequencyModulationRatio * counter3) + 1) / 2.0;
    EPwm3Regs.CMPA.half.CMPA = EPwm3Regs.TBPRD - magnitudeModulationRatio * EPwm3Regs.TBPRD * sinus3 - 1;
    counter3 +=1;
    if( counter3 > ((1 / frequencyModulationRatio) - 1)) counter3 = 0;

    EPwm1Regs.ETCLR.bit.INT = 1;
    EPwm2Regs.ETCLR.bit.INT = 1;
    EPwm3Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = 4;

}

//===========================================================================
// End of SourceCode.
//===========================================================================







