#ifndef _DIAGNOSTIC_H
#define _DIAGNOSTIC_H
#include "PE_Types.h"

#ifndef NULL
	#define NULL (void*)(0)
#endif

#define	LEVEL_ZERO  0x0F
#define	LEVEL_ONE 1
#define	LEVEL_TWO 2
#define	LEVEL_THREE 4
#define	UNSUPPORT 0
//SecurityLevel;

#define HARD_RESET 1
#define KEY_OFF_ON_RESET 2
#define SOFT_RESET 3
#define ENABLE_RAPID_POWER_SHUTDOWN 4
#define DISABLE_RAPID_POWER_SHUTDOWN 5
//EcuResetType;

#define EEPROM_DID 0
#define REALTIME_DID 1
#define IO_DID 2
//DIDType;

#define READONLY 1
#define WRITEONLY 2
#define READWRITE 3
//ReadWriteAttr;

#define ERXTX 0//enableRxAndTx
#define ERXDTX 1//enableRxAndDisableTx
#define DRXETX 2//disableRxAndEnableTx
#define DRXTX 3//disableRxAndTx
//CommulicationType;

#define NCM 1//application message
#define NWMCM 2//network manage message
#define NWMCM_NCM 3//application and netwrok manage message
//communicationParam;

#define SUB_DEFAULT 1//sub function supported in default session
#define SUB_PROGRAM 2//sub function supported in program session
#define SUB_EXTENDED 4////sub function supported in extedned session
#define SUB_ALL 7//sub function supported in both of three session
//SubFunSuppInSession;

typedef uint8_t (*IoControl)(uint8_t ctrl, uint8_t param);
typedef uint32_t (*SecurityFun)(uint32_t);
typedef void (*ResetCallBack)(uint8_t);
typedef void (*CommCallBack)(uint8_t , uint8_t);
typedef uint8_t (*SendCANFun)(uint32_t ID, uint8_t *array, uint8_t length, uint8_t priority);
typedef void (*DiagRequest)(void);

uint32_t TestSecurityLevelBoot(uint32_t seed);
uint8_t SendFrame(uint32_t ID, uint8_t *array, uint8_t length, uint8_t priority);
void DiagRequested(void);
void SystemReset(uint8_t resetType);

void Diagnostic_Init(void);
void Diagnostic_RxFrame(uint32_t ID,uint8_t* data,uint8_t IDE,uint8_t DLC,uint8_t RTR);
void Diagnostic_1msTimer(void);
void Diagnostic_LoadAllData(void);
byte Diagnostic_EEProm_Read(word add, byte size, byte *data);
byte Diagnostic_EEProm_Write(word add, byte size, byte *data);
void Diagnostic_10_02_Response_AfterJump(void);
uint8_t Diagnostic_GetCurrentSession(void);
/************set netwrok layer parameters********/
void Diagnostic_SetNLParam(uint8_t TimeAs, uint8_t TimeBs, uint8_t TimeCr, uint8_t TimeAr, uint8_t TimeBr, uint8_t TimeCs, uint8_t BlockSize, uint8_t m_STmin, uint8_t FillData);
void Diagnostic_Proc(void);

#endif
