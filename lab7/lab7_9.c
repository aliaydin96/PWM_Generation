// 
//      Lab5_1: TMS320F28335
//      (c) Frank Bormann
//
//###########################################################################
//
// FILE:    Lab5_1.c
//
// TITLE:   DSP28335ControlCARD; Digital Output
//          4 - bit - counter at 4 LEDs LD1(GPIO9), LD2(GPIO11), LD3(GPIO34)
//          and LD4 (GPIO49)
//          software delay loop; watchdog disabled
//          template file for Lab5_1
//###########################################################################
//  Ver | dd mmm yyyy | Who  | Description of changes
// =====|=============|======|===============================================
//  3.0 | 02 May 2008 | F.B. | Lab5_1 for F28335;
//  3.1 | 06 Nov 2009 | F.B  | Lab5_1 for F28335 and PE revision5
//###########################################################################


 long fpwm = 26000;      // in Hz
 int CLKDIV = 0;
 int HSPCLKDIV = 0;
 int CLKDIV_2 = 0;
 int HSPCLKDIV_2 = 0;
 int CLKDIV_3 = 0;
 int HSPCLKDIV_3 = 0;


#include "DSP2833x_Device.h"
#include "IQmathLib.h"
#pragma DATA_SECTION(sine_table, "IQmathTables");
_iq30 sine_table[512];
_iq30 sine_table_2[512];
_iq30 sine_table_3[512];
static unsigned int index = 0;
// Prototype statements for functions found within this file.
void Gpio_select(void); 
extern void InitSysCtrl(void);
extern void InitPieCtrl(void);
extern void InitPieVectTable(void);
interrupt void ePWM1A_compare_isr(void);

void Setup_ePWM1(void);
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
        Setup_ePWM1();
        InitPieCtrl();

        InitPieVectTable();

        EALLOW;
        PieVectTable.EPWM1_INT = &ePWM1A_compare_isr;
        EDIS;

        PieCtrlRegs.PIEIER3.bit.INTx1 = 1;

        IER |= 4;
        EINT;
        ERTM;


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
    GpioCtrlRegs.GPADIR.bit.GPIO9 = 1;  // peripheral explorer: LED LD1 at GPIO9
    GpioCtrlRegs.GPADIR.bit.GPIO11 = 1; // peripheral explorer: LED LD2 at GPIO11

    GpioCtrlRegs.GPBDIR.all = 0;        // GPIO63-32 as inputs
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1; // peripheral explorer: LED LD3 at GPIO34
    GpioCtrlRegs.GPBDIR.bit.GPIO49 = 1; // peripheral explorer: LED LD4 at GPIO49
    GpioCtrlRegs.GPCDIR.all = 0;        // GPIO87-64 as inputs
    EDIS; 
}


void Setup_ePWM1(void){
    EPwm1Regs.TBCTL.all = 0;
    EPwm1Regs.TBCTL.bit.CLKDIV = 0;            //CLKDIV = 1
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;        //HSPCLKDIV = 1
    EPwm1Regs.TBCTL.bit.CTRMODE = 2;         // Count up and down operation (10) = 2
    EPwm1Regs.AQCTLA.all = 0x0060;          //set ePWM1A to 1 on �CMPA - up match�
                                           //clear ePWM1A on event �CMPA - down match�

    if ((75000000 >= fpwm) && (fpwm >= 1200)){
        EPwm1Regs.TBCTL.bit.CLKDIV = 0;
        CLKDIV = 1;
        EPwm1Regs.TBCTL.bit.HSPCLKDIV = 0;
        HSPCLKDIV = 1;
    }
    else if((1200 > fpwm) && (fpwm > 0)){
        EPwm1Regs.TBCTL.bit.CLKDIV = 7;
        CLKDIV = 14;
        EPwm1Regs.TBCTL.bit.HSPCLKDIV = 7;
        HSPCLKDIV = 128;
    }

    long fcpu = 150000000; // we determine up in configcputimer

    EPwm1Regs.TBPRD = (0.5 * fcpu) / (fpwm * CLKDIV * HSPCLKDIV);        //the maximum number for TBPRD is (216 -1) or 65535

    /*  TBPRD = 0.5*fcpu/(fpwm*CLKDIV*HSPCLKDIV)
         * fcpu = 60 MHz , fpwm = 1 KHz

         */
    EPwm1Regs.CMPA.half.CMPA = EPwm1Regs.TBPRD / 2; // 50% duty cycle first

    EPwm1Regs.ETSEL.all = 0;
    EPwm1Regs.ETSEL.bit.INTEN = 1;      // interrupt enable for ePWM1
    EPwm1Regs.ETSEL.bit.INTSEL = 5;     // interrupt on CMPA down match
    EPwm1Regs.ETPS.bit.INTPRD = 1;      // interrupt on first event




    EPwm2Regs.TBCTL.all = 0;

    EPwm2Regs.TBCTL.bit.CTRMODE = 2;  // Count up and down operation (10) = 2
    EPwm2Regs.AQCTLA.all = 0x0006;

    if ((75000000 >= fpwm) && (fpwm >= 1200)){
        EPwm2Regs.TBCTL.bit.CLKDIV = 0;
        CLKDIV_2 = 1;
        EPwm2Regs.TBCTL.bit.HSPCLKDIV = 0;
        HSPCLKDIV_2 = 1;
    }
    else if((1200 > fpwm) && (fpwm > 0)){
        EPwm2Regs.TBCTL.bit.CLKDIV = 7;
        CLKDIV_2 = 14;
        EPwm2Regs.TBCTL.bit.HSPCLKDIV = 7;
        HSPCLKDIV_2 = 128;
    }

    EPwm2Regs.TBPRD = (0.5 * fcpu) / (fpwm * CLKDIV_2 * HSPCLKDIV_2);
    EPwm2Regs.CMPA.half.CMPA = EPwm2Regs.TBPRD / 2; // 50% duty cycle first

    EPwm2Regs.ETSEL.all = 0;
    EPwm2Regs.ETSEL.bit.INTEN = 1;      // interrupt enable for ePWM1
    EPwm2Regs.ETSEL.bit.INTSEL = 5;     // interrupt on CMPA down match
    EPwm2Regs.ETPS.bit.INTPRD = 1;      // interrupt on first event


    EPwm3Regs.TBCTL.all = 0;
    EPwm3Regs.TBCTL.bit.CTRMODE = 2;  // Count up and down operation (10) = 2
    EPwm3Regs.AQCTLA.all = 0x0006;
    if ((75000000 >= fpwm) && (fpwm >= 1200)){
        EPwm3Regs.TBCTL.bit.CLKDIV = 0;
        CLKDIV_3 = 1;
        EPwm3Regs.TBCTL.bit.HSPCLKDIV = 0;
        HSPCLKDIV_3 = 1;
    }
    else if((1200 > fpwm) && (fpwm > 0)){
        EPwm3Regs.TBCTL.bit.CLKDIV = 7;
        CLKDIV_3 = 14;
        EPwm3Regs.TBCTL.bit.HSPCLKDIV = 7;
        HSPCLKDIV_3 = 128;
    }
    EPwm3Regs.TBPRD = (0.5 * fcpu) / (fpwm * CLKDIV_3 * HSPCLKDIV_3);
    EPwm3Regs.CMPA.half.CMPA = EPwm3Regs.TBPRD / 2; // 50% duty cycle first


    EPwm1Regs.TBCTL.bit.SYNCOSEL = 1;   // GENERATE A SIGNAL IF CTR =0

    EPwm2Regs.TBCTL.bit.PHSEN = 1; // PHASE ENABLE FOR EPWM2A
    EPwm2Regs.TBCTL.bit.SYNCOSEL = 0;  // SYNIN = SYNCOUT
    EPwm2Regs.TBPHS.half.TBPHS = 10000; // 1/3 PHASE SHIFT

    EPwm3Regs.TBCTL.bit.PHSEN = 1; // PHASE ENABLE FOR EPWM3
    EPwm3Regs.TBPHS.half.TBPHS = 20000;  // 2/3 PHASE SHIFT

}

interrupt void ePWM1A_compare_isr(void) {

    EALLOW;
    SysCtrlRegs.WDKEY = 0xAA;   // service WD #2
    EDIS;

    EPwm1Regs.CMPA.half.CMPA = EPwm1Regs.TBPRD -_IQsat( _IQ30mpy((sine_table[index]+_IQ30(0.9999))/2, EPwm1Regs.TBPRD), EPwm1Regs.TBPRD,0);


    EPwm2Regs.CMPA.half.CMPA = EPwm2Regs.TBPRD -_IQsat( _IQ30mpy((sine_table_2[index]+_IQ30(0.9999))/2, EPwm2Regs.TBPRD), EPwm2Regs.TBPRD,0);



    EPwm3Regs.CMPA.half.CMPA = EPwm3Regs.TBPRD -_IQsat( _IQ30mpy((sine_table_3[index]+_IQ30(0.9999))/2, EPwm3Regs.TBPRD), EPwm3Regs.TBPRD,0);

    index +=1;
    if( index > 511) index = 0;
    EPwm1Regs.ETCLR.bit.INT = 1; PieCtrlRegs.PIEACK.all = 4;
}



//===========================================================================
// End of SourceCode.
//===========================================================================
