/************************************************************************
* (c) Copyright Freescale Semiconductor, Inc 2010, All Rights Reserved  *
*************************************************************************

************************************************************************
*                                                                      *
*        Standard Software Flash Driver For S12G                       *
*                                                                      *
* FILE NAME     :  SSD_Types.h                                         *
* DATE          :  April 10,2010                                       *
*                                                                      *
* AUTHOR        :  FPT Team			                                   *
* E-mail        :  b28216@freescale.com                                *
*                                                                      *
************************************************************************/
/************************** CHANGES ***********************************
 0.1.0    04.10.2010	 FPT Team			Port to S12G from S12P SSD
 1.1.1    04.15.2010     FPT Team           No change
***********************************************************************/

#ifndef _SSD_TYPES_H_
#define _SSD_TYPES_H_

#ifndef __PE_Types_H


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

typedef volatile signed char VINT8;
typedef volatile unsigned char VUINT8;

;
typedef volatile signed short VINT16;
typedef volatile unsigned short VUINT16;




typedef volatile unsigned long VUINT32;

#endif

typedef unsigned char BOOL;

typedef signed char INT8;
typedef unsigned char UINT8;

typedef signed short INT16;
typedef unsigned short UINT16;

typedef signed long INT32;
typedef unsigned long UINT32;
typedef volatile signed long VINT32;

#endif /* _SSD_TYPES_H_ */


