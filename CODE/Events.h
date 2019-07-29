/** ###################################################################
**     Filename  : Events.h
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

#ifndef __Events_H
#define __Events_H
/* MODULE Events */

#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "CAN1.h"
#include "TJA1043_STB.h"
#include "rti.h"
#include "AD_BATT.h"

#pragma CODE_SEG DEFAULT


void AD_BATT_OnEnd(void);
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

/* END Events */
#endif /* __Events_H*/

/*
** ###################################################################
**
**     This file was created by Processor Expert 3.05 [04.46]
**     for the Freescale HCS12 series of microcontrollers.
**
** ###################################################################
*/
