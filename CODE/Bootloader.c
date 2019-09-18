/** ###################################################################
**     Filename  : Bootloader.c
**     Project   : Bootloader
**     Processor : MC9S12G128MLF
**     Version   : Driver 01.14
**     Compiler  : CodeWarrior HC12 C Compiler
**     Date/Time : 2019/1/8, 10:20
**     Abstract  :
**         Main module.
**         This module contains user's application code.
**     Settings  :
**     Contents  :
**         No public methods
**
** ###################################################################*/
/* MODULE Bootloader */

/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
#include "CAN1.h"
#include "TJA1043_STB.h"
#include "rti.h"
#include "AD_BATT.h"
/* Include shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"

/* User includes (#include below this line is not maintained by Processor Expert) */
#include "diagnostic.h"
#include "DiagnosticTimer.h"
#include "config.h"
#include "EEpromDriver.h"
#include "MemManager.h"

uint8_t FBL_SOFT_VERSION[4] = {"V2.0"};
uint8_t FBL_REQUIREMENT_VERSION[4] = {"V1.7"};
const char ProjectName[] = {"BOOT20"};


DiagTimer BattDetectTimer;
DiagTimer BootloaderDelay;
DiagTimer PowerManagerTimer;
static bool FirstStart = TRUE;
static bool IsInIndication = FALSE;
static uint8_t BatVol;

#define BOOT_SLEEP_IN_NO_REUEST        60000

#pragma CODE_SEG __NEAR_SEG NON_BANKED 
ISR(can_rx_interrupt)
{
    uint8_t tmp;
    uint8_t canData[8];
    uint8_t canDlc;
    uint8_t canRtr;
    uint8_t canIde;
    uint32_t canId;

    Cpu_DisableInt();

    /*CANRFLG
     * 7-WUPIF Wake-Up Interrupt Flag
     * 6-CSCIF CAN Status Change Interrupt Flag
     * 5:4-RSTAT Receiver Status Bits
     * 3:2-TSTAT Transmitter Status Bits
     * 1-OVRIF Overrun Interrupt Flag
     * 0-RXF Receive Buffer Full Flag
     *         0 No new message available within the RxFG
     *         1 The receiver FIFO is not empty. A new message is available in the RxFG
     * */

    /*Receive Buffer Full Flag*/
    if (CANRFLG_RXF)
    {
        tmp = CANRXIDR1;
        if (tmp & (1<<3))
        { /*1 Extended format (29 bit)*/
            canId = CANRXIDR0;
            canId <<= 21;
            tmp = ((tmp>>5)<<3) | (tmp&7);
            canId |= ((uint32_t)tmp)<<15;
            canId |= ((uint32_t)CANRXIDR2)<<7;
            tmp = CANRXIDR3;
            canRtr = tmp & 1;
            canId |= (tmp>>1);
        }
        else
        { /*0 Standard format (11 bit)*/
            canRtr = tmp & 1;
            canId = CANRXIDR0;
            canId <<= 3;
            canId |= ((tmp >> 5) & 0x07);
        }
        canDlc = CANRXDLR_DLC;
        if (canDlc == 0) 
        {
            CANRFLG_RXF = 1;
            return ;
        }
        
        canIde = (CANRXIDR0 >> 3) & 0x01;
        for (tmp = 0; tmp < CANRXDLR_DLC; ++tmp)
        {
            canData[tmp] = *(&CANRXDSR0 + tmp);

        }
        Diagnostic_RxFrame(canId , canData , canIde , canDlc , canRtr);
        CANRFLG_RXF = 1;
    }

    Cpu_EnableInt();
}

ISR(CanWakeUpInt)
{
// NOTE: The routine should include the following actions to obtain
//       correct functionality of the hardware.
//
//      The ISR is invoked by WUPIF flag. The WUPIF flag is cleared
//      if a "1" is written to the flag in CANRFLG register.
//      Example: CANRFLG = CANRFLG_WUPIF_MASK;
    CANRFLG_WUPIF=1;
    DiagTimer_Set(&PowerManagerTimer , BOOT_SLEEP_IN_NO_REUEST);
    TJA1043_STB_PutVal(0);
}

ISR(rti_interrupt)
{
    CPMUFLG_RTIF = 1;
    Diagnostic_1msTimer();
}
#pragma CODE_SEG DEFAULT
/*--------------------------------------------------------------------------------------*/

void BootHardWareInit(void)
{
    TJA1043_STB_PutVal(0);
    rti_Init();
}

uint8_t SendFrame(uint32_t ID, uint8_t *array, uint8_t length, uint8_t priority) 
{
    uint8_t i;

    CANTBSEL=CANTFLG;
    if (CANTBSEL==0) 
    {
        return 0;  
    }

    if(ID > 0xFFF)
    {
        *((VUINT8 *) (&CANTXIDR0)) = (VUINT8)(ID >> 21); 
        *((VUINT8 *) (&CANTXIDR1)) = 0x18 | (VUINT8)(((ID >>13) & 0xE0) |  ((ID >> 15) & 0x07)); 
        *((VUINT8 *) (&CANTXIDR2)) = (VUINT8)(ID >> 7); 
        *((VUINT8 *) (&CANTXIDR3)) = 0xFE & (VUINT8)((ID << 1) & 0xFE); 
    }
    else
    {
        *((VUINT8 *) (&CANTXIDR0)) = ((VUINT8)(ID >> 3)); 
        *((VUINT8 *) (&CANTXIDR1)) = ((VUINT8)((ID << 5) & 0xE0)); 
    }
    
    for (i=0;i<length;i++)
    {
        *(&CANTXDSR0+i)=array[i];
    }   
    CANTXDLR=length;  
    CANTXTBPR=priority; 

    CANTFLG=CANTBSEL;    //发送缓冲区相应TXE位写1清除该位来通知MSCAN发送数据  
    return CANTBSEL;
}

uint32_t TestSecurityLevelBoot(uint32_t seed)
{
	return 0x12345678;
}

uint8_t CAN1_Disable(void)
{
    CANCTL0_INITRQ = 0x01U;            /* Disable device */
    while(CANCTL1_INITAK == 0U) {}     /* Wait for device initialization acknowledge */
    CANRIER = 0x00U;                   /* Disable interrupts */
    return ERR_OK;                       /* OK */
}

void RTI_DIsable(void)
{
    CPMUINT = 0x00;
    CPMUFLG_RTIF = 1;
}

void Prepare_Before_Jump(void)
{
    CAN1_Disable();   
    Cpu_DisableInt(); /*disable the CPU interrupt*/
    MemManger_CleanCodeRam();       /*clean the bootloader used RAM  for NVM driver*/
    RTI_DIsable();    /*disable RTI interrupt*/
    //Bit1_PutVal(0);
}

void ClearRequestFlag(void)
{
    #if(1 == PROGRAM_FLAG_SIZE)
    *(uint8_t*)BOOT_PROGRAM_FLAG_ADDRESS = PROGRAM_NO_REQUEST;
    #elif(2 == PROGRAM_FLAG_SIZE)
    *(uint16_t*)BOOT_PROGRAM_FLAG_ADDRESS = PROGRAM_NO_REQUEST;
    #elif(4 == PROGRAM_FLAG_SIZE)
    *(uint32_t*)BOOT_PROGRAM_FLAG_ADDRESS = PROGRAM_NO_REQUEST;
    #endif
}

void SystemReset(uint8_t resetType)
{
    uint16_t i;
    if(resetType == 1 || resetType == 3)
    {
        union{
            void (*vector)(void);
            uint8_t c[2];
        }softReset;
        for(i = 0;i < 4000;i++);//delay wait CAN message success send
        Cpu_DisableInt();
        TJA1043_STB_PutVal(1);
        

        softReset.c[0]=*(uint8_t *)0xFFFE;
        softReset.c[1]=*(uint8_t *)0xFFFF;
        softReset.vector(); 
    }
}

void DiagRequested(void)
{
    DiagTimer_Set(&PowerManagerTimer , BOOT_SLEEP_IN_NO_REUEST);
}

void VarifyFBLVersion(void)
{
    uint8_t FBLVersion[4];
    Diagnostic_EEProm_Read(BOOTADER_VERSION_ADDRESS , 4 , FBLVersion);
    if(memcmp(FBLVersion , FBL_SOFT_VERSION , 4) != 0)
    {
        Diagnostic_EEProm_Write(BOOTADER_VERSION_ADDRESS , 4 , FBL_SOFT_VERSION);
    }
}


void DiagnosticInit(void) {
    bool Result;
    Diagnostic_EEProm_Init();
    Diagnostic_Init();

    Diagnostic_LoadAllData();
    
    Diagnostic_SetNLParam(70, 150, 150, 70, 70, 70, 0, 0 , 0xAA);
}

void VoltageDetectLoop(void)
{
    if(DiagTimer_HasExpired(&BattDetectTimer))
    {
        uint16_t BattVoltage;
        AD_BATT_Measure(1);
        AD_BATT_GetValue16(&BattVoltage);
        //BatVol = (uint8_t)(((uint32_t)BattVoltage*4750/27621) + 7);
        BatVol = (uint8_t)(((uint32_t)BattVoltage*215/1023) + 7);
        
        Power_BattResolve(BatVol);
        
        DiagTimer_Set(&BattDetectTimer,200);
    }
}

void PowerManagerProc(void)
{
    if(DiagTimer_HasExpired(&PowerManagerTimer))
    {
        CANRFLG_RXF = 1;
        CANRFLG_OVRIF = 1;
        CANRFLG_CSCIF = 1;
        CANRFLG_WUPIF = 1;
        CANCTL1_LISTEN = 0;

        CANTARQ_ABTRQ = 7;
        CANCTL0_WUPE = 1;

        CANCTL0_SLPRQ = 1;       
        while(CANCTL1_SLPAK == 1U) {              /* Wait for init exit */
        }
        CANRIER_WUPIE = 1;
        
         
        TJA1043_STB_PutVal(1);
    }
}

void BootLoaderLoop(void)
{
    CAN1_Init();
    BootHardWareInit();
    MemManger_Init();
    
    VarifyFBLVersion();
    DiagTimer_Set(&PowerManagerTimer , BOOT_SLEEP_IN_NO_REUEST);
    DiagTimer_Set(&BattDetectTimer,500);
    while(1)
    {
        VoltageDetectLoop();
        PowerManagerProc();
        Diagnostic_Proc();
        MemManger_Task();
    }
}

void main(void)
{
    /* Write your local variable definition here */

    #if(1 == APP_VALID_FLAG_SIZE)
    uint8_t IsAPPValid;
    #elif(2 == APP_VALID_FLAG_SIZE)
    uint16_t IsAPPValid;
    #elif(4 == APP_VALID_FLAG_SIZE)
    uint32_t IsAPPValid;
    #endif
    
    #if(1 == PROGRAM_FLAG_SIZE)
    uint8_t ProRequestFlag = *(uint8_t*)BOOT_PROGRAM_FLAG_ADDRESS;
    #elif(2 == PROGRAM_FLAG_SIZE)
    uint16_t ProRequestFlag = *(uint16_t*)BOOT_PROGRAM_FLAG_ADDRESS;;
    #elif(4 == PROGRAM_FLAG_SIZE)
    uint32_t ProRequestFlag = *(uint32_t*)BOOT_PROGRAM_FLAG_ADDRESS;;
    #endif

    /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
    PE_low_level_init();
    /*** End of Processor Expert internal initialization.                    ***/
    
    /* Write your code here */
    
    //Diagnostic_EEProm_Read(BOOT_PROGRAM_FLAG_ADDRESS , PROGRAM_FLAG_SIZE , (uint8_t*)(&ProRequestFlag));
    Diagnostic_EEProm_Read(APP_VALID_FLAG_ADDRESS , APP_VALID_FLAG_SIZE , (uint8_t*)(&IsAPPValid));
    
    if(ProRequestFlag == PRAOGRAM_REQUESTED_FLAG)//external programing reqeust
    {
        DiagnosticInit();
        Diagnostic_10_02_Response_AfterJump();
        ClearRequestFlag();
        BootLoaderLoop();
    }
    else if(IsAPPValid != APP_IS_VALID)//app invalid
    {
        DiagnosticInit();
        BootLoaderLoop();
    }
    else
    {
        Diagnostic_EEProm_Init();
        VarifyFBLVersion();
        Prepare_Before_Jump();
        asm("SEI");
        asm("LDX 0x7FFA");
        asm("JSR 0 , X");
    }

  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END Bootloader */
/*
** ###################################################################
**
**     This file was created by Processor Expert 3.05 [04.46]
**     for the Freescale HCS12 series of microcontrollers.
**
** ###################################################################
*/
