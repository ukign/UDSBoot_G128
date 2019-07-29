/** ###################################################################
**     Filename  : Events.c
**     Project   : Bootloader
**     Processor : MC9S12G128MLF
**     Component : Events
**     Version   : Driver 01.04
**     Compiler  : CodeWarrior HC12 C Compiler
**     Date/Time : 2019/1/8, 10:20
**     Abstract  :
**         This is user's event module.
**         Put your event handler code here.
**     Settings  :
**     Contents  :
**         No public methods
**
** ###################################################################*/
/* MODULE Events */


#include "Cpu.h"
#include "Events.h"

/* User includes (#include below this line is not maintained by Processor Expert) */

#pragma CODE_SEG DEFAULT

/*
** ===================================================================
**     Event       :  AD_BATT_OnEnd (module Events)
**
**     Component   :  AD_BATT [ADC]
**     Description :
**         This event is called after the measurement (which consists
**         of <1 or more conversions>) is/are finished.
**         The event is available only when the <Interrupt
**         service/event> property is enabled.
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
void AD_BATT_OnEnd(void)
{
  /* Write your code here ... */
}

/* END Events */

/*
** ###################################################################
**
**     This file was created by Processor Expert 3.05 [04.46]
**     for the Freescale HCS12 series of microcontrollers.
**
** ###################################################################
*/
