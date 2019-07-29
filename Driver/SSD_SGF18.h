/************************************************************************
* (c) Copyright Freescale Semiconductor, Inc 2010, All Rights Reserved  *
*************************************************************************

************************************************************************
*                                                                      *
*        Standard Software Flash Driver For S12G                       *
*                                                                      *
* FILE NAME     :  SSD_SGF18.h                                         *
* DATE          :  April 10,2010                                       *
*                                                                      *
* AUTHOR        :  FPT Team			                                   *
* E-mail        :  b28216@freescale.com                                *
*                                                                      *
************************************************************************/
/************************** CHANGES ***********************************
 0.1.0    04.10.2010	 FPT Team			Port to S12G from S12P SSD
 1.1.1    04.15.2010     FPT Team			Update values of FLASH_END_ADDR
											and EE_END_ADDR
 1.1.2    09.13.2012     FPT Team           Update to support S12VR48 and S12VR64 (derivative 8 and 9)
                                            Determine BLOCK_SIZE by (FLASH_END_ADDR-FLASH_START_ADDR)
                                            Determine EE_MAXIMUM_SIZE by (EE_END_ADDR - EE_START_ADDR)
                                            Add MAX_CLOCK_VALUE macro.
***********************************************************************/

#ifndef _SSD_SGF18_H_
#define _SSD_SGF18_H_

/* ------------------------ Configuration Macros ------------------------ */

/* For S12G Core */
/* FLASH_DERIVATIVE 0  S12G240 */
/* FLASH_DERIVATIVE 1  S12G192 */
/* FLASH_DERIVATIVE 2  S12G128 */
/* FLASH_DERIVATIVE 3  S12G96  */
/* FLASH_DERIVATIVE 4  S12G64  */
/* FLASH_DERIVATIVE 5  S12G48/S12GN48 */
/* FLASH_DERIVATIVE 6  S12GN32 */
/* FLASH_DERIVATIVE 7  S12GN16 */
/* FLASH_DERIVATIVE 8  S12VR48 */
/* FLASH_DERIVATIVE 9  S12VR64 */

/* Set the derivative */
#define FLASH_DERIVATIVE                2     /*should be configured according to the right part number*/

/*--------------- SUBJECT TO CHANGES ACCORDING TO COMPILER -------------*/
/* For CodeWarrior, the keyword is near or __near */
#define SSD_SGF18_NEAR                  __near

/* For CodeWarrior, the keyword is far or __far */
#define SSD_SGF18_FAR                  __far



/*------------ Return Code Definition for HCS12X-SSD -------------------*/

#define SGF_OK                          0x0000
#define SGF_ERR_CLKSET                  0x0001
#define SGF_ERR_SIZE                    0x0002
#define SGF_ERR_RANGE                   0x0004
#define SGF_ERR_ACCERR                  0x0008
#define SGF_ERR_PVIOL                   0x0010
#define SGF_ERR_MGSTAT0                 0x0020
#define SGF_ERR_MGSTAT1                 0x0040
#define SGF_ERR_PROTMODE                0x0100
#define SGF_ERR_PROTSIZE                0x0200
#define SGF_ERR_DATAMISMATCH            0x0400
#define SGF_ERR_INVALIDCLK              0x0800
#define SGF_ERR_ADDR                    0x1000
#define SGF_ERR_SFDIF                   0x2000
#define SGF_ERR_DFDIF                   0x4000

/* Flash protection scenario modes */
#define FLASH_PROT_EXCEPT_HIGH_LOW      0x00
#define FLASH_PROT_EXCEPT_HIGH          0x01
#define FLASH_PROT_EXCEPT_LOW           0x02
#define FLASH_PROT_ALL                  0x03
#define FLASH_PROT_HIGH_LOW             0x04
#define FLASH_PROT_HIGH                 0x05
#define FLASH_PROT_LOW                  0x06
#define FLASH_PROT_NONE                 0x07

/* Flash protection high size modes */
#define FLASH_PROT_SIZE_HIGH_2K         0x00
#define FLASH_PROT_SIZE_HIGH_4K         0x01
#define FLASH_PROT_SIZE_HIGH_8K         0x02
#define FLASH_PROT_SIZE_HIGH_16K        0x03

/* Flash protection low size modes */
#define FLASH_PROT_SIZE_LOW_1K          0x00
#define FLASH_PROT_SIZE_LOW_2K          0x01
#define FLASH_PROT_SIZE_LOW_4K          0x02
#define FLASH_PROT_SIZE_LOW_8K          0x03

/* Flash security status */
#define FLASH_SECURITY_STATE_KEYEN      0x80
#define FLASH_SECURITY_STATE_UNSECURED  0x02

#define FLASH_NOT_SECURE                0x01
#define FLASH_SECURE_BACKDOOR_ENABLED   0x02
#define FLASH_SECURE_BACKDOOR_DISABLED  0x04

/* Total no. of backdoor key */
#define TOTAL_KEY_WORDS                 0x04

/* Command Sequence Index Macros */
#define COMMAND_INDEX0                  0x00
#define COMMAND_INDEX1                  0x01
#define COMMAND_INDEX2                  0x02
#define COMMAND_INDEX3                  0x03
#define COMMAND_INDEX4                  0x04
#define COMMAND_INDEX5                  0x05

/* Maximum value for program & read once phrase index */
#define MAX_PHRASE_INDEX                0x07

/*-------------- Read/Write/Set/Clear Operation Macros ------------------*/
#define REG_BIT_SET(address, mask)      (*(VUINT8 *SSD_SGF18_NEAR)(address) |= (mask))
#define REG_BIT_CLEAR(address, mask)    (*(VUINT8 *SSD_SGF18_NEAR)(address) &= ~(mask))
#define REG_BIT_TEST(address, mask)     (*(VUINT8 *SSD_SGF18_NEAR)(address) & (mask))
#define REG_WRITE(address, value)       (*(VUINT8 *SSD_SGF18_NEAR)(address) = (value))
#define REG_READ(address)               ((UINT8)(*(VUINT8 *SSD_SGF18_NEAR)(address)))
#define REG_WRITE16(address, value)     (*(VUINT16 *SSD_SGF18_NEAR)(address) = (value))
#define REG_READ16(address)             ((UINT16)(*(VUINT16 *SSD_SGF18_NEAR)(address)))

#define WRITE8(address, value)          (*(VUINT8 *SSD_SGF18_NEAR)(address) = (value))
#define READ8(address)                  ((UINT8)(*(VUINT8 *SSD_SGF18_NEAR)(address)))
#define SET8(address, value)            (*(VUINT8 *SSD_SGF18_NEAR)(address) |= (value))
#define CLEAR8(address, value)          (*(VUINT8 *SSD_SGF18_NEAR)(address) &= ~(value))
#define TEST8(address, value)           (*(VUINT8 *SSD_SGF18_NEAR)(address) & (value))

#define WRITE16(address, value)         (*(VUINT16 *SSD_SGF18_NEAR)(address) = (value))
#define READ16(address)                 ((UINT16)(*(VUINT16 *SSD_SGF18_NEAR)(address)))
#define SET16(address, value)           (*(VUINT16 *SSD_SGF18_NEAR)(address) |= (value))
#define CLEAR16(address, value)         (*(VUINT16 *SSD_SGF18_NEAR)(address) &= ~(value))
#define TEST16(address, value)          (*(VUINT16 *SSD_SGF18_NEAR)(address) & (value))

#define WRITE32(address, value)         (*(VUINT32 *SSD_SGF18_NEAR)(address) = (value))
#define READ32(address)                 ((UINT32)(*(VUINT32 *SSD_SGF18_NEAR)(address)))
#define SET32(address, value)           (*(VUINT32 *SSD_SGF18_NEAR)(address) |= (value))
#define CLEAR32(address, value)         (*(VUINT32 *SSD_SGF18_NEAR)(address) &= ~(value))
#define TEST32(address, value)          (*(VUINT32 *SSD_SGF18_NEAR)(address) & (value))

/*-------------------- Macro definition for HCS12G ----------------------*/
#define HCS12G_WORD_SIZE                0x0002

/*--------------------- CallBack function period ------------------------*/
#define FLASH_CALLBACK_CS               1   /* Check Sum */
#define FLASH_CALLBACK_PV               1   /* Program Verify */

/*-------------------- Callback function prototype ---------------------*/
typedef void (* SSD_SGF18_NEAR PCALLBACK)(void);

/*--------------------Null Callback function defination ------------------*/
#define NULL_CALLBACK                   ((PCALLBACK)0xFFFF)

/*---------------- Flash SSD Configuration Structure -------------------*/
typedef struct
{
    UINT32         registerBase;      /* base address of MCU register block */
    UINT16         busClock;          /* target bus clock                   */
    UINT8          ignoreSingleFault; /* ignore single fault setting flag   */
    BOOL           BDMEnable;         /* background debug mode enable bit   */
    PCALLBACK      CallBack;          /* pointer to callback function       */
} FLASH_SSD_CONFIG;


    /* Flash Start Address */
    /* G240         0x04000 */
    /* G192         0x10000 */
    /* G128         0x20000 */
    /* G96          0x28000 */
    /* G64          0x30000 */
    /* G48/GN48     0x34000 */
    /* GN32         0x38000 */
    /* GN16         0x3C000 */
    /* VR38         0x34000 */
    /* VR64         0x30000 */


    #if(0 == FLASH_DERIVATIVE)
    #define FLASH_START_ADDR               0x04000
    #elif(1 == FLASH_DERIVATIVE)
    #define FLASH_START_ADDR               0x10000
    #elif(2 == FLASH_DERIVATIVE)
    #define FLASH_START_ADDR               0x20000
    #elif(3 == FLASH_DERIVATIVE)
    #define FLASH_START_ADDR               0x28000
    #elif(4 == FLASH_DERIVATIVE)  
    #define FLASH_START_ADDR               0x30000
    #elif(5 == FLASH_DERIVATIVE)
    #define FLASH_START_ADDR               0x34000
    #elif(6 == FLASH_DERIVATIVE)
    #define FLASH_START_ADDR               0x38000
    #elif(7 == FLASH_DERIVATIVE)
    #define FLASH_START_ADDR               0x3C000
    #elif(8 == FLASH_DERIVATIVE)
    #define FLASH_START_ADDR               0x34000
    #elif(9 == FLASH_DERIVATIVE)
    #define FLASH_START_ADDR               0x30000
    #endif

    /* Flash block size */
    // #if(0 == FLASH_DERIVATIVE)      /* (G240) */
    // #define BLOCK_SIZE              0x0003C000 /* 240 KB size */
    // #elif(1 == FLASH_DERIVATIVE)    /* (G192) */
    // #define BLOCK_SIZE              0x00030000 /* 192 KB size */
    // #elif(2 == FLASH_DERIVATIVE)    /* (G128) */
    // #define BLOCK_SIZE              0x00020000 /* 128 KB size */
    // #elif(3 == FLASH_DERIVATIVE)    /* (G96) */
    // #define BLOCK_SIZE              0x0018000  /* 96 KB size */
    // #elif(4 == FLASH_DERIVATIVE)    /* (G64) */
    // #define BLOCK_SIZE              0x0010000  /* 64 KB size */
    // #elif(5 == FLASH_DERIVATIVE)    /* (G48/GN48) */
    // #define BLOCK_SIZE              0x000C000  /* 48 KB size */
    // #elif(6 == FLASH_DERIVATIVE)    /* (GN32) */
    // #define BLOCK_SIZE              0x0008000  /* 32 KB size */
    // #elif(7 == FLASH_DERIVATIVE)    /* (GN16) */
    // #define BLOCK_SIZE              0x0004000  /* 16 KB size */
    // #endif

    /* Flash end address: This is the final point address on P-Flash */
    #define FLASH_END_ADDR                 0x3B000  //0x3FFFF
    
    /* Flash block size */
    #define BLOCK_SIZE              (FLASH_END_ADDR-FLASH_START_ADDR)
    
    /* Flash sector size */
    #define FLASH_SECTOR_SIZE              0x00000200 /* 512Bytes size */

    /* Global address [17:16] to identify EEPROM block */
    #define GLOBAL_ADDRESS_EE           0x0000
    /* EEPROM block start address */
    #define EE_START_ADDR               0x0400

    #define EE_END_ADDR          0x12D0
    
    /* EEPROM sector size */
    #define EE_SECTOR_SIZE              0x0004  /* 4 Bytes size */
    /* EEPROM maximum size */
    #define EE_MAXIMUM_SIZE         (EE_END_ADDR - EE_START_ADDR)
        
    // #if(0 == FLASH_DERIVATIVE)      /* (G240) */
    // #define EE_MAXIMUM_SIZE        0x01000    /* 4 KB size */
    // #elif(1 == FLASH_DERIVATIVE)    /* (G192) */
    // #define EE_MAXIMUM_SIZE        0x01000    /* 4 KB size */ 
    // #elif(2 == FLASH_DERIVATIVE)    /* (G128) */
    // #define EE_MAXIMUM_SIZE        0x01000    /* 4 KB size */ 
    // #elif(3 == FLASH_DERIVATIVE)    /* (G96) */
    // #define EE_MAXIMUM_SIZE        0x00C00    /* 3 KB size */  
    // #elif(4 == FLASH_DERIVATIVE)    /* (G64) */
    // #define EE_MAXIMUM_SIZE        0x00800    /* 2 KB size */  
    // #elif(5 == FLASH_DERIVATIVE)    /* (G48/GN48) */
    // #define EE_MAXIMUM_SIZE        0x00600    /* 1.5 KB size */ 
    // #elif(6 == FLASH_DERIVATIVE)    /* (GN32) */
    // #define EE_MAXIMUM_SIZE        0x00400    /* 1 KB size */  
    // #elif(7 == FLASH_DERIVATIVE)    /* (GN16) */
    // #define EE_MAXIMUM_SIZE        0x00200    /* 0.5 KB size */ 
    // #endif


    /* Phrase size */
    #define FLASH_PHRASE_SIZE               0x0008  /* 8 bytes */
    #define EE_PROGRAM_OFFSET           0x0000
    /* Read EEPROM Operation Macro */
    #define READ_EE16(address)          ((UINT16)(*(VUINT16 *)(address-EE_PROGRAM_OFFSET)))

    /* page size calculation Macros */
    #define FLASH_START                    FLASH_START_ADDR
    #define FLASH_PAGE_SIZE                0x4000
    #define FLASH_PAGE_WINDOW_START        0x8000

    /* PPAGE offset Macro */
    #define MMC2_PPAGE_OFFSET               0x0015

    /* Address fetching mask */
    #define GLOBAL_ADDRESS_MASK             0x0030000
    #define ADDRESS_OFFSET_MASK             0x0000FFFF
    
    /* maximum bus clock in MHz */
    #if ((8 == FLASH_DERIVATIVE) || (9 == FLASH_DERIVATIVE))
    #define MAX_CLOCK_VALUE                 25
    #else
    #define MAX_CLOCK_VALUE                 32
    #endif

/* -------------------- Function Pointer ------------------------------- */
typedef UINT16 (* SSD_SGF18_NEAR pFLASHCOMMANDSEQUENCE)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                        UINT8 index, \
                                                        UINT16 *SSD_SGF18_NEAR PCommandArray);

/* Flash initialization */
typedef UINT16 (* SSD_SGF18_FAR pFLASHINIT)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig);

/* Flash get protection */
typedef UINT16 (* SSD_SGF18_FAR pFLASHGETPROTECTION)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                      UINT8 *SSD_SGF18_NEAR ProtectMode, \
                                                      UINT8 *SSD_SGF18_NEAR HighSize, \
                                                      UINT8 *SSD_SGF18_NEAR LowSize);

/* Flash set protection */
typedef UINT16 (*SSD_SGF18_FAR pFLASHSETPROTECTION)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                     UINT8  protectMode, \
                                                     UINT8  highSize, \
                                                     UINT8  lowSize);

/* Flash get interrupt enable */
typedef UINT16 (* SSD_SGF18_FAR pFLASHGETINTERRUPTENABLE)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                           UINT8 *SSD_SGF18_NEAR InterruptState);

/* Flash set interrupt enable */
typedef UINT16 (* SSD_SGF18_FAR pFLASHSETINTERRUPTENABLE)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                           UINT8 interruptState);

/* Flash get security state */
typedef UINT16 (* SSD_SGF18_FAR pFLASHGETSECURITYSTATE)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                         UINT8 *SSD_SGF18_NEAR SecurityState);

/* Flash security bypass */
typedef UINT16 (* SSD_SGF18_FAR pFLASHSECURITYBYPASS)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                       UINT16 *SSD_SGF18_NEAR KeyBuffer, \
                                                       pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash program */
typedef UINT16 (* SSD_SGF18_FAR pFLASHPROGRAM)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                 UINT32 targetAddrOffset, \
                                                 UINT32 size, \
                                                 UINT32 source, \
                                                 pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* F erase */
typedef UINT16 (* SSD_SGF18_FAR pFLASHERASE)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                               UINT32 destination, \
                                               UINT32 size, \
                                               pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash erase verify */
typedef UINT16 (* SSD_SGF18_FAR pFLASHERASEVERIFY)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                     UINT32 destination, \
                                                     UINT32 size, \
                                                     pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* EEPROM program */
typedef UINT16 (* SSD_SGF18_FAR pEEPROGRAM)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                 UINT16 destination, \
                                                 UINT16 size, \
                                                 UINT32 source, \
                                                 pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* EEPROM erase */
typedef UINT16 (* SSD_SGF18_FAR pEEERASE)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                               UINT16 destination, \
                                               UINT16 size, \
                                               pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* EEPROM erase verify */
typedef UINT16 (* SSD_SGF18_FAR pEEERASEVERIFY)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                     UINT16 destination, \
                                                     UINT16 size, \
                                                     pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash erase verify all */
typedef UINT16 (* SSD_SGF18_FAR pFLASHERASEVERIFYALL)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                       pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash program verify */
typedef UINT16 (* SSD_SGF18_FAR pFLASHPROGRAMVERIFY)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                      UINT32 destination, \
                                                      UINT32 size, \
                                                      UINT32 source, \
                                                      UINT32 *SSD_SGF18_NEAR PFailAddr, \
                                                      UINT16 *SSD_SGF18_NEAR PFailData, \
                                                      UINT16 *SSD_SGF18_NEAR PSrcData);

/* Flash checksum */
typedef UINT16 (* SSD_SGF18_FAR pFLASHCHECKSUM)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                 UINT32 destination, \
                                                 UINT32 size, \
                                                 UINT16 *SSD_SGF18_NEAR PSum);
/* Flash ECC check */
    typedef UINT16 (* SSD_SGF18_FAR pFLASHECCCHECK)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig);

/* Flash set user margin */
typedef UINT16 (* SSD_SGF18_FAR pFLASHSETUSERMARGIN)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                      UINT32 blockAddress, \
                                                      UINT16 marginValue, \
                                                      pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash read once */
typedef UINT16 (* SSD_SGF18_FAR pFLASHREADONCE)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                 UINT16 *SSD_SGF18_NEAR PDataArray, \
                                                 pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash program once */
typedef UINT16 (* SSD_SGF18_FAR pFLASHPROGRAMONCE)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                    UINT16 *SSD_SGF18_NEAR PDataArray, \
                                                    pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* SPECIAL MODE FUNCTIONS */


/* Flash set field margin */
typedef UINT16 (* SSD_SGF18_FAR pFLASHSETFIELDMARGIN)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                       UINT32 blockAddress, \
                                                       UINT16 marginValue, \
                                                       pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash erase all block */
typedef UINT16 (* SSD_SGF18_FAR pFLASHERASEALLBLOCK)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                      pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash unsecure */
typedef UINT16 (* SSD_SGF18_FAR pFLASHUNSECURE)(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                 pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/*---------------- Function Prototypes for Flash SSD --------------------*/

/* Flash initialization */
extern UINT16 SSD_SGF18_FAR FlashInit(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig);

/* Flash get protection */
extern UINT16 SSD_SGF18_FAR FlashGetProtection(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                UINT8 *SSD_SGF18_NEAR ProtectMode, \
                                                UINT8 *SSD_SGF18_NEAR HighSize, \
                                                UINT8 *SSD_SGF18_NEAR LowSize);

/* Flash set protection */
extern UINT16 SSD_SGF18_FAR FlashSetProtection(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                UINT8  protectMode, \
                                                UINT8  highSize, \
                                                UINT8  lowSize);

/* Flash get interrupt enable */
extern UINT16 SSD_SGF18_FAR FlashGetInterruptEnable(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                     UINT8 *SSD_SGF18_NEAR InterruptState);

/* Flash set interrupt enable */
extern UINT16 SSD_SGF18_FAR FlashSetInterruptEnable(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                     UINT8 interruptState);

/* Flash get security state */
extern UINT16 SSD_SGF18_FAR FlashGetSecurityState(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                   UINT8 *SSD_SGF18_NEAR SecurityState);

/* Flash security bypass */
extern UINT16 SSD_SGF18_FAR FlashSecurityBypass(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                 UINT16 *SSD_SGF18_NEAR KeyBuffer, \
                                                 pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash program */
extern UINT16 SSD_SGF18_FAR FlashProgram(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                           UINT32 targetAddrOffset, \
                                           UINT32 size, \
                                           UINT32 source, \
                                           pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash erase */
extern UINT16 SSD_SGF18_FAR FlashErase(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                         UINT32 destination, \
                                         UINT32 size, \
                                         pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash erase verify */
extern UINT16 SSD_SGF18_FAR FlashEraseVerify(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                               UINT32 destination, \
                                               UINT32 size, \
                                               pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* EEPROM program */
extern UINT16 SSD_SGF18_FAR EEProgram(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                           UINT16 destination, \
                                           UINT16 size, \
                                           UINT32 source, \
                                           pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* EEPROM erase */
extern UINT16 SSD_SGF18_FAR EEErase(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                         UINT16 destination, \
                                         UINT16 size, \
                                         pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* EEPROM erase verify */
extern UINT16 SSD_SGF18_FAR EEEraseVerify(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                               UINT16 destination, \
                                               UINT16 size, \
                                               pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash erase verify all */
extern UINT16 SSD_SGF18_FAR FlashEraseVerifyAll(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                 pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash program verify */
extern UINT16 SSD_SGF18_FAR FlashProgramVerify(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                UINT32 destination, \
                                                UINT32 size, \
                                                UINT32 source, \
                                                UINT32 *SSD_SGF18_NEAR PFailAddr, \
                                                UINT16 *SSD_SGF18_NEAR PFailData, \
                                                UINT16 *SSD_SGF18_NEAR PSrcData);

/* Flash checksum */
extern UINT16 SSD_SGF18_FAR FlashCheckSum(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                           UINT32 destination, \
                                           UINT32 size, \
                                           UINT16 *SSD_SGF18_NEAR PSum);

/* Flash ECC check */
    extern UINT16 SSD_SGF18_FAR FlashECCCheck(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig);

/* Flash set user margin */
extern UINT16 SSD_SGF18_FAR FlashSetUserMargin(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                UINT32 blockAddress, \
                                                UINT16 marginValue, \
                                                pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash read once */
extern UINT16 SSD_SGF18_FAR FlashReadOnce(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                           UINT16 *SSD_SGF18_NEAR PDataArray, \
                                           pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash program once */
extern UINT16 SSD_SGF18_FAR FlashProgramOnce(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                              UINT16 *SSD_SGF18_NEAR PDataArray, \
                                              pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* SPECIAL MODE FUNCTIONS */

/* Flash set field margin */
extern UINT16 SSD_SGF18_FAR FlashSetFieldMargin(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                                 UINT32 blockAddress, \
                                                 UINT16 marginValue, \
                                                 pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash erase all block */
extern UINT16 SSD_SGF18_FAR FlashEraseAllBlock(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Flash unsecure */
extern UINT16 SSD_SGF18_FAR FlashUnsecure(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                           pFLASHCOMMANDSEQUENCE FlashCommandSequence);

/* Internal function. Called by driver APIs only */
/* Flash command sequence */

extern UINT16 SSD_SGF18_NEAR FlashCommandSequence(FLASH_SSD_CONFIG *SSD_SGF18_NEAR PSSDConfig, \
                                                  UINT8 index, \
                                                  UINT16 *SSD_SGF18_NEAR PCommandArray);

#endif /* _SSD_SGF18_H_ */


