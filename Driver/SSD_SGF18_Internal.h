/************************************************************************
* (c) Copyright Freescale Semiconductor, Inc 2010, All Rights Reserved  *
*************************************************************************

************************************************************************
*                                                                      *
*        Standard Software Flash Driver For S12G                       *
*                                                                      *
* FILE NAME     :  SSD_SGF18_Internal.h                                *
* DATE          :  April 10,2010                                       *
*                                                                      *
* AUTHOR        :  FPT Team			                                   *
* E-mail        :  b28216@freescale.com                                *
*                                                                      *
************************************************************************/
/************************** CHANGES ***********************************
 0.1.0    04.10.2010	 FPT Team			Port to S12G from S12P SSD
 1.1.1	  04.15.2010	 FPT Team			No change
***********************************************************************/

#ifndef _SSD_SGF18_INTERNAL_H_
#define _SSD_SGF18_INTERNAL_H_

/*--------------- HCS12G Flash Module Memory Offset Map -----------------*/
/* Flash module base offset */
#define FLASH_REG_BASE                  0x0100

/* Flash clock divider register */
#define FLASH_FCLKDIV_OFFSET            (FLASH_REG_BASE + 0x0000)
#define FLASH_FCLKDIV_FDIVLD            0x80
#define FLASH_FCLKDIV_FDIVLCK           0x40


/* Flash security register */
#define FLASH_FSEC_OFFSET               (FLASH_REG_BASE + 0x0001)
#define FLASH_FSEC_KEYEN                0xC0
#define FLASH_FSEC_SEC                  0x03

/* Flash CCOB index register */
#define FLASH_FCCOBIX_OFFSET            (FLASH_REG_BASE + 0x0002)

/* Flash configuration register */
#define FLASH_FCNFG_OFFSET              (FLASH_REG_BASE + 0x0004)
#define FLASH_FCNFG_CCIE                0x80
#define FLASH_FCNFG_IGNSF               0x10
#define FLASH_FCNFG_FDFD                0x02
#define FLASH_FCNFG_FSFD                0x01

/* Flash error configuration register */
#define FLASH_FERCNFG_OFFSET            (FLASH_REG_BASE + 0x0005)
#define FLASH_FERCNFG_DFDIE             0x02
#define FLASH_FERCNFG_SFDIE             0x01

/* Flash status register */
#define FLASH_FSTAT_OFFSET              (FLASH_REG_BASE + 0x0006)
#define FLASH_FSTAT_CCIF                0x80
#define FLASH_FSTAT_ACCERR              0x20
#define FLASH_FSTAT_FPVIOL              0x10
#define FLASH_FSTAT_MGSTAT1             0x02
#define FLASH_FSTAT_MGSTAT0             0x01

/* Flash error status register */
#define FLASH_FERSTAT_OFFSET            (FLASH_REG_BASE + 0x0007)
#define FLASH_FERSTAT_DFDIF             0x02
#define FLASH_FERSTAT_SFDIF             0x01

/* Flash Protection register */
#define FLASH_FPROT_OFFSET              (FLASH_REG_BASE + 0x0008)
#define FLASH_FPROT_FPOPEN              0x80
#define FLASH_FPROT_FPHDIS              0x20
#define FLASH_FPROT_FPHS                0x18
#define FLASH_FPROT_FPLDIS              0x04
#define FLASH_FPROT_FPLS                0x03

/* Flash common command object register */
#define FLASH_FCCOB_OFFSET              (FLASH_REG_BASE + 0x000A)

/* Flash common command object high register */
#define FLASH_FCCOBHI_OFFSET            (FLASH_REG_BASE + 0x000A)

/* Flash common command object low register */
#define FLASH_FCCOBLO_OFFSET            (FLASH_REG_BASE + 0x000B)


/*------------- Flash hardware algorithm operation commands -------------*/
#define FLASH_ERASE_VERIFY_ALL          0x01
#define FLASH_ERASE_VERIFY_BLOCK        0x02
#define FLASH_ERASE_VERIFY_SECTION      0x03
#define FLASH_READ_ONCE                 0x04
#define FLASH_PROGRAM                   0x06
#define FLASH_PROGRAM_ONCE              0x07
#define FLASH_ERASE_ALL_BLOCK           0x08
#define FLASH_ERASE_BLOCK               0x09
#define FLASH_ERASE_SECTOR              0x0A
#define FLASH_UNSECURE                  0x0B
#define FLASH_VERIFY_BACKDOOR_ACCESS    0x0C
#define FLASH_SET_USER_MARGIN           0x0D
#define FLASH_SET_FIELD_MARGIN          0x0E
#define EE_ERASE_VERIFY                 0x10
#define EE_PROGRAM                      0x11
#define EE_ERASE_SECTOR                 0x12

/* Flash margin read settings */
#define FLASH_MARGIN_NORMAL             0x0000
#define FLASH_USER_MARGIN_LEVEL1        0x0001
#define FLASH_USER_MARGIN_LEVEL0        0x0002
#define FLASH_FIELD_MARGIN_LEVEL0       0x0003
#define FLASH_FIELD_MARGIN_LEVEL1       0x0004

/* Memory Memory Control Offset Map */
/* MMC Control Register (MMCCTL1) */
#ifndef  MMCCTL1
#define MMCCTL1                         0x0013 
#endif

/* IFRON bit of MMC Control Register (MMCCTL1) */
#ifndef  MMCCTL1_NVMRES
#define MMCCTL1_NVMRES                0x01
#endif
#endif  /* _SSD_SGF18_INTERNAL_H_ */