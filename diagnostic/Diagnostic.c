#define _DIAGNOSTIC_C
#include "cpu.h"
#include "DiagnosticTimer.h"
#include "NetworkLayer.h"
#include "diagnostic.h"
#include "MemManager.h"
#include "config.h"
#include "EEpromDriver.h"

/* Private typedef -----------------------------------------------------------*/
#define MAX_RESPONSE_LENGTH 30

#define PHY_REQ_ID  0x751
#define FUN_REQ_ID  0x7DF
#define UDS_RES_ID  0x759

#define P2CanServerMax 0x0032
#define P2ECanServerMax 0x00C8

/* Private define ------------------------------------------------------------*/
typedef void (*ServiceHandler)(uint8_t N_TAType,uint16_t length, uint8_t *MessageData);
typedef far byte* far Memory_TAddress;

typedef enum{
    PR = 0x00,//positive response
    GR = 0x10,//general reject
    SNS = 0x11,//service not supported
    SFNS = 0x12,//sub-function not supported
    IMLOIF = 0x13,//incorrect message length or invalid format
    RTL = 0x14,//response too long
    BRR = 0x21,//busy repeat request
    CNC = 0x22,//condifitons not correct
    RSE = 0x24,//request sequence error
    NRFSC = 0x25,
    FPEORA = 0x26,
    ROOR = 0x31,//reqeust out of range
    SAD = 0x33,//security access denied
    IK = 0x35,//invalid key
    ENOA = 0x36,//exceed number of attempts
    RTDNE = 0x37,//required time delay not expired
    UDNA = 0x70,//upload download not accepted
    TDS = 0x71,//transfer data suspended
    GPF = 0x72,//general programming failure
    WBSC = 0x73,//wrong block sequence coutner
    RCRRP = 0x78,//request correctly received-respone pending
    SFNSIAS = 0x7e,//sub-function not supported in active session
    SNSIAS  = 0x7F,//service not supported in active session
    VTH = 0x92,//voltage too high
    VTL = 0x93,//voltage too low
}NegativeResposeCode;

typedef enum{
    ECU_DEFAULT_SESSION = 1,
    ECU_PAOGRAM_SESSION = 2,
    ECU_EXTENED_SESSION = 3,
}SessionType;

typedef enum{
    WAIT_SEED_REQ,
    WAIT_KEY,
    WAIT_DELAY,
    UNLOCKED,
}SecurityUnlockStep;

typedef enum
{
    PRE_STEP,
    WRITE_FINGER_PRINT,
    DOWNLOAD_FLASH_DRIVER_34,
    DOWNLOAD_FLASH_DRIVER_36,
    DOWNLOAD_FLASH_DRIVER_37,
    ENABLE_PROGRAM,
    CHECK_DRIVER_CRC,
    ERASE_FLASH,
    TRANSFER_APP_DATA_34,
    TRANSFER_APP_DATA_36,
    TRANSFER_APP_DATA_37,
    CHECK_APP_CRC,
    CHECK_VALID_APP,
}UpdateStep;

typedef struct _DidNode{
    uint16_t ID;
    uint8_t dataLength;
    uint8_t* dataPointer;
    uint8_t didType;
    IoControl Callback;
    uint8_t RWAttr;
    uint32_t EEpromAddr;
}DIDNode;

typedef struct{
    bool support;
    uint8_t serviceName;
    uint8_t PHYDefaultSession_Security:4;//security suppport in default session physical address
    uint8_t PHYProgramSeesion_Security:4;//security suppport in program session physical address
    uint8_t PHYExtendedSession_Security:4;//security suppport in extened session physical address
    uint8_t FUNDefaultSession_Security:4;//security suppport in default session function address
    uint8_t FUNProgramSeesion_Security:4;//security suppport in program session function address
    uint8_t FUNExtendedSession_Security:4;//security suppport in extened session function address
    ServiceHandler serviceHandle;
}SessionService;

typedef struct{
    bool valid;
    uint8_t level;
    SecurityFun UnlockFunction;
    uint8_t seedID;
    uint8_t keyID;
    uint8_t *FaultCounter;
    uint16_t FaultCounterAddr;
    uint8_t FaultLimitCounter;
    uint32_t UnlockFailedDelayTime;
    uint8_t subFunctionSupported;
    DiagTimer *SecurityLockTimer;                        //解锁失败3次后再次解锁延时定时器
    uint8_t KeySize;
}SecurityUnlock;

typedef struct{
    bool valid;
    uint16_t RID;
    uint8_t OptionRecordLength;
    uint8_t SecuritySupport;
    uint8_t support;
}Routine;

/* Private macro -------------------------------------------------------------*/
typedef struct{
    uint16_t  Service10Sub01Supported:1;
    uint16_t Service10Sub02Supported:1;
    uint16_t Service10Sub03Supported:1;
    uint16_t Service10Sub01To02OK:1;
    uint16_t Service10Sub02To03OK:1;
    uint16_t Service10SupressSupproted:1;
    
    uint16_t Service3ESupressSupported:1;
    uint16_t Service85SupressSupported:1;
    
    uint16_t Service11SupressSupported:1;
    uint16_t Service11Sub01Supported:1;
    uint16_t Service11Sub02Supported:1;
    uint16_t Service11Sub03Supported:1;
    uint16_t Service11Sub04Supported:1;
    uint16_t Service11Sub05Supported:1;
    
    uint16_t Service28Sub00Suppoted:1;
    uint16_t Service28Sub01Suppoted:1;
    uint16_t Service28Sub02Suppoted:1;
    uint16_t Service28Sub03Suppoted:1;
    uint16_t Service28Type01Suppoted:1;
    uint16_t Service28Type02Suppoted:1;
    uint16_t Service28Type03Suppoted:1;
    uint16_t Service28SupressSupported:1;
}UdsConfigState;

const UdsConfigState g_udsCfgState = 
{
    TRUE , TRUE , TRUE , TRUE, TRUE, TRUE ,
    TRUE, TRUE,
    TRUE ,TRUE , FALSE , TRUE , FALSE, FALSE,
    TRUE , TRUE , TRUE , TRUE, TRUE, TRUE , TRUE, TRUE ,
};

/* Private function prototypes -----------------------------------------------*/

void ServiceNegReponse(uint8_t serviceName,uint8_t RejectCode);
void Diagnostic_ReadDTCPositiveResponse(uint8_t DTCSubFunction,uint8_t DTCStatausMask);
void Service10Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData);
void Service11Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData);
void Service27Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData);
void Service28Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData);
void Service3EHandle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData);
void Service85Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData);
void Service22Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData);
void Service2EHandle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData);
void Service31Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData);
void Service34Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData);
void Service36Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData);
void Service37Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData);
DIDNode* SearchDidNode(uint16_t DID);
bool CheckProgramComplete(void);

/* Private variables ---------------------------------------------------------*/

/*========================about security access================================*/
static uint8_t Seed[5];                                        //保存解锁种子
static uint8_t key[4];                                        //保存解锁密匙
static uint32_t retLen;
static uint8_t m_SecurityLevel = LEVEL_ZERO;            //当前解锁等级
static SecurityUnlockStep m_UnlockStep = WAIT_SEED_REQ;    //当前解锁步骤
static uint8_t FaultCntLevel1;
static DiagTimer TimerLevel1;

const SecurityUnlock UnlockList[] = 
{
    {TRUE,LEVEL_ONE,TestSecurityLevelBoot,0x01,0x02 , &FaultCntLevel1 , 0x13F0, 3 , 180000, SUB_PROGRAM , &TimerLevel1, 4},
};

#define MAX_SECURITY_NUM (sizeof(UnlockList)/sizeof(UnlockList[0]))
/*========================about security access================================*/

/*========================sesion , id , buf and so on================================*/
static NegativeResposeCode m_NRC;                    //否定响应码
uint8_t m_CurrSessionType;                            //当前回话类型
uint16_t ResponseLength;
uint8_t CurrentService;
uint8_t PendingService;
uint32_t BlockCRC;
static uint8_t DiagnosticBuffTX[MAX_RESPONSE_LENGTH];                            //发送数据的缓存
static DiagTimer S3serverTimer;                             //S3定时器
static uint8_t N_Ta;
static uint8_t N_Sa;
static DiagTimer PendingTimer;

 /*========================sesion , id , buf and so on================================*/

/*========================about program================================*/
typedef struct
{
    uint16_t IsCalibrationDataValid:1;
    uint16_t IsAppDataValid:1;
    uint16_t IsNeedIncreaseAttempt:1;
    uint16_t ResponsePending:1;
    uint16_t WaitConfimBeforeReset:1;
    uint16_t suppressResponse:1;
    uint16_t WaitErase:1;//擦除前等待78负反馈的确认信息
    uint16_t WaitConfirmOfErasePending:1;
    uint16_t WaitConfirmOfExitPending:1;
    uint16_t WaitProgram:1;
    uint16_t Program78ResponseConfirmed:1;
}UdsRunningState;

static uint8_t m_BlockIndex = 0;
static uint32_t ProgramAddress = 0;
static uint32_t ProgramLength = 0;
static uint32_t EraseAddress = 0;
static uint32_t EraseLength = 0;
static uint32_t ProgramLengthComplete = 0;
static uint16_t LastRequestLength = 0;
static UdsRunningState g_udsRunState;
static uint8_t EraseIndex;

const Routine m_currRoutine[5] = 
{
    {TRUE , 0xFF00 , 8 , LEVEL_ONE , SUB_PROGRAM},
    {TRUE , 0xDF00 , 0 , LEVEL_ZERO, SUB_EXTENDED},
    {TRUE , 0xDFFF , 4 , LEVEL_ONE , SUB_PROGRAM},
    {TRUE , 0xFF01 , 0 , LEVEL_ONE , SUB_PROGRAM},
    {FALSE},
};

static UpdateStep m_UpdateStep;
/*========================about program================================*/

/*========================about DTC and DIDs================================*/    

extern uint8_t FBL_SOFT_VERSION[4];

const DIDNode DIDS[] = {
    {0xF193, 2 ,  NULL , EEPROM_DID , NULL , READONLY , HARDWARE_VERSION_ADDRESS},
    {0xF195, 2 ,  NULL , EEPROM_DID , NULL , READONLY , SOFTWARE_VERSION_ADDRESS},
    {0xF190, 17 ,  NULL , EEPROM_DID , NULL , READONLY , VIN_ADDRESS},
    {0xF18A, 7 ,  NULL , EEPROM_DID , NULL , READONLY , SUPPLIER_CODE_ADDRESS},
    {0xF1CB, 4 ,  NULL , EEPROM_DID , NULL , READONLY , PART_NUMBER_ADDRESS},
    {0xF191, 4 ,  NULL , EEPROM_DID , NULL , READONLY , SGMW_HARD_NUM_ADDRESS},
    {0xF188, 4 ,  NULL , EEPROM_DID , NULL , READONLY , SGMW_SOFT_NUM_ADDRESS},
    {0xF180, 4 ,  FBL_SOFT_VERSION , REALTIME_DID , NULL , READONLY , 0},
    {0xF186, 1 , &m_CurrSessionType ,  REALTIME_DID , NULL , READONLY , 0},
};

#define MAX_DID_NUMBER (sizeof(DIDS)/sizeof(DIDS[0]))

/*========================about DTC and DIDs================================*/

/*========================about reset================================*/
static uint8_t m_EcuResetType;                        //ECU复位类型
/*========================about reset===============================*/

const SessionService ServiceList[] = {
    {TRUE, 0x10, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, Service10Handle},//0X10
    {TRUE, 0x11, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, Service11Handle},//0X11
    {TRUE, 0x27, UNSUPPORT, LEVEL_ZERO, LEVEL_ZERO, UNSUPPORT, UNSUPPORT, UNSUPPORT, Service27Handle},//0X27
    {TRUE, 0x28, UNSUPPORT, UNSUPPORT, LEVEL_ZERO, UNSUPPORT, UNSUPPORT, LEVEL_ZERO, Service28Handle},//0X28
    {TRUE, 0x3E, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, Service3EHandle},//0X3E
    {TRUE, 0x85, UNSUPPORT, UNSUPPORT, LEVEL_ZERO, UNSUPPORT, UNSUPPORT, LEVEL_ZERO, Service85Handle},//0X85
    {TRUE, 0x22, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO, LEVEL_ZERO,Service22Handle},//0X22
    {TRUE, 0x2E, UNSUPPORT, LEVEL_ONE, UNSUPPORT, UNSUPPORT, UNSUPPORT, UNSUPPORT,Service2EHandle},//0X2E
    {TRUE, 0x31, UNSUPPORT, LEVEL_ONE, LEVEL_ZERO, UNSUPPORT, UNSUPPORT, UNSUPPORT,Service31Handle},//0X31
    {TRUE, 0x34, UNSUPPORT, LEVEL_ONE, UNSUPPORT, UNSUPPORT, UNSUPPORT, UNSUPPORT,Service34Handle},//0X34
    {TRUE, 0x36, UNSUPPORT, LEVEL_ONE, UNSUPPORT, UNSUPPORT, UNSUPPORT, UNSUPPORT,Service36Handle},//0X36
    {TRUE, 0x37, UNSUPPORT, LEVEL_ONE, UNSUPPORT, UNSUPPORT, UNSUPPORT, UNSUPPORT,Service37Handle},//0X37
};

#define SERVICE_NUMBER (sizeof(ServiceList)/sizeof(ServiceList[0]))

/* Private functions ---------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/

/************set netwrok layer parameters********/
void Diagnostic_SetNLParam(uint8_t TimeAs, uint8_t TimeBs, uint8_t TimeCr, uint8_t TimeAr, uint8_t TimeBr, uint8_t TimeCs, 
    uint8_t BlockSize, uint8_t m_STmin, uint8_t FillData)
{
    NetworkLayer_SetParam(TimeAs , TimeBs , TimeCr , TimeAr , TimeBr , TimeCs , BlockSize , m_STmin , HALF_DUPLEX , DIAGNOSTIC , N_Sa ,  N_Ta , PHYSICAL , 0 , FillData);
}

void Diagnostic_Init(void)
{
   
    N_Ta = (uint8_t)UDS_RES_ID;
    N_Sa = (uint8_t)(UDS_RES_ID >> 8);
    NetworkLayer_InitParam(PHY_REQ_ID, FUN_REQ_ID, UDS_RES_ID, SendFrame);
    
    m_CurrSessionType = ECU_DEFAULT_SESSION;
    
    m_UpdateStep = PRE_STEP;
    
    g_udsRunState.WaitConfimBeforeReset = FALSE; 
    g_udsRunState.ResponsePending = FALSE;
    g_udsRunState.IsNeedIncreaseAttempt = FALSE;
    g_udsRunState.IsCalibrationDataValid = FALSE;
    g_udsRunState.IsAppDataValid = FALSE;
}
/*========interface for application layer setting diagnostic parameters==============*/


void GenerateSeed(uint8_t *seed, uint32_t length)
{
    uint32_t SystemTick = (uint32_t)DiagTimer_GetTickCount();
    uint32_t seedVaraint;
    Diagnostic_EEProm_Read( SEED_VARAINT_CODE_ADDRESS , 4 , &seedVaraint);
    SystemTick = seedVaraint - (SystemTick ^ (seedVaraint << (SystemTick % 32)));
    Diagnostic_EEProm_Write( SEED_VARAINT_CODE_ADDRESS , 4 , &SystemTick);

    seed[0] = (uint8_t)SystemTick ^ (uint8_t)(SystemTick >> 3);
    seed[1] = (uint8_t)SystemTick ^ (uint8_t)(SystemTick >> 7);
    seed[2] = (uint8_t)SystemTick ^ (uint8_t)(SystemTick >> 11);
    seed[3] = (uint8_t)(SystemTick>>3) ^ (uint8_t)(SystemTick >> 11);
}

void GotoSession(SessionType session)
{
    if(session != ECU_DEFAULT_SESSION)
    {
        DiagTimer_Set(&S3serverTimer, 5000);
    }
    else
    {
        if(m_CurrSessionType != ECU_DEFAULT_SESSION)
        {
            g_udsRunState.WaitConfimBeforeReset = TRUE;
            PendingService = 0x10;
            m_EcuResetType = 1;
        }
    }
    
    m_CurrSessionType = session;
    m_SecurityLevel = LEVEL_ZERO;//session change ECU lock even if from extended session to extended session
    if(m_UnlockStep != WAIT_DELAY)
    {
        m_UnlockStep = WAIT_SEED_REQ;//by ukign 2016.04.01
    }
}

void Service10PosResponse(SessionType session)
{
    DiagnosticBuffTX[0] = 0x50;
    DiagnosticBuffTX[1] = session;
    DiagnosticBuffTX[2] = (uint8_t)(P2CanServerMax >> 8);
    DiagnosticBuffTX[3] = (uint8_t)P2CanServerMax;
    DiagnosticBuffTX[4] = (uint8_t)(P2ECanServerMax >> 8);
    DiagnosticBuffTX[5] = (uint8_t)P2ECanServerMax;
    ResponseLength = 6;
}

void Service10Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    uint8_t SubFunction;
    uint8_t suppressPosRspMsgIndicationBit;
    m_NRC = PR;
    if(length == 2)
    {
        if(g_udsCfgState.Service10SupressSupproted)
        {
            SubFunction = *(MessageData + 1) & 0x7F;
            suppressPosRspMsgIndicationBit = *(MessageData + 1) & 0x80;
        }
        else
        {
            SubFunction = *(MessageData + 1);
            suppressPosRspMsgIndicationBit = 0;
        }
        
        switch(SubFunction)/* get sub-function parameter value without bit 7 */
        {
            case ECU_DEFAULT_SESSION: /* test if sub-function parameter value is supported */
                if(!g_udsCfgState.Service10Sub01Supported)
                {
                    m_NRC = SFNS;
                }
                break;
            case ECU_EXTENED_SESSION: /* test if sub-function parameter value is supported */
                if(!g_udsCfgState.Service10Sub03Supported)
                {
                    m_NRC = SFNS;
                }
                else
                {
                    if(m_CurrSessionType == ECU_PAOGRAM_SESSION && !g_udsCfgState.Service10Sub02To03OK)
                    {
                        m_NRC = SFNSIAS;
                    }
                }
                break;
            case ECU_PAOGRAM_SESSION: /* test if sub-function parameter value is supported */
                if(!g_udsCfgState.Service10Sub02Supported)
                {
                    m_NRC = SFNS;
                }
                else
                {
                    if(N_TAType == FUNCTIONAL)
                    {
                        m_NRC = SFNS;
                    }
                    else if(m_CurrSessionType == ECU_DEFAULT_SESSION && !g_udsCfgState.Service10Sub01To02OK)
                    {
                        m_NRC = SFNSIAS;
                    }
                }
                break;
            default:
                m_NRC = SFNS; /* NRC 0x12: sub-functionNotSupported *///
        }
    }
    else
    {
        m_NRC = IMLOIF;
    }
    
    if ( (suppressPosRspMsgIndicationBit) && (m_NRC == 0x00) && (g_udsRunState.ResponsePending == FALSE))
    {
        g_udsRunState.suppressResponse = TRUE; /* flag to NOT send a positive response message */
    }
    else
    {
        g_udsRunState.suppressResponse = FALSE; /* flag to send the response message */
        Service10PosResponse(SubFunction);
    }

    if(m_NRC == PR)
    {
        GotoSession(SubFunction);
    }
}

void Service11Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    uint8_t SubFunction;
    uint8_t suppressPosRspMsgIndicationBit;
    m_NRC = PR;
    if(length == 2)
    {
        if(g_udsCfgState.Service11SupressSupported)
        {
            SubFunction = *(MessageData + 1) & 0x7F;
            m_EcuResetType = SubFunction;
            suppressPosRspMsgIndicationBit = *(MessageData + 1) & 0x80;
        }
        else
        {
            SubFunction = *(MessageData + 1);
            m_EcuResetType = SubFunction;
            suppressPosRspMsgIndicationBit = 0;
        }
        
        switch(SubFunction)/* get sub-function parameter value without bit 7 */
        {
            case HARD_RESET:
                {
                    if(!g_udsCfgState.Service11Sub01Supported)
                    {
                        m_NRC = SFNS;
                    }
                }
                break;
            case KEY_OFF_ON_RESET: /* test if sub-function parameter value is supported */
                {
                    if(!g_udsCfgState.Service11Sub02Supported)
                    {
                        m_NRC = SFNS;
                    }
                }
                break;
            case SOFT_RESET: /* test if sub-function parameter value is supported */
                {
                    if(!g_udsCfgState.Service11Sub03Supported)
                    {
                        m_NRC = SFNS;
                    }
                }
                break;
            case ENABLE_RAPID_POWER_SHUTDOWN:
                {
                    if(!g_udsCfgState.Service11Sub04Supported)
                    {
                        m_NRC = SFNS;
                    }
                }
                break;
            case DISABLE_RAPID_POWER_SHUTDOWN:
                {
                    if(!g_udsCfgState.Service11Sub05Supported)
                    {
                        m_NRC = SFNS;
                    }
                }
                break;
            default:
                m_NRC = SFNS; /* NRC 0x12: sub-functionNotSupported */
        }
    }
    else
    {
        m_NRC = IMLOIF;
    }
    
    if ( (suppressPosRspMsgIndicationBit) && (m_NRC == 0x00) && (g_udsRunState.ResponsePending == FALSE))
    {
        g_udsRunState.suppressResponse = TRUE; /* flag to NOT send a positive response message */
    }
    else
    {
        g_udsRunState.suppressResponse = FALSE; /* flag to send the response message */        
        DiagnosticBuffTX[0] = CurrentService + 0x40;
        DiagnosticBuffTX[1] = SubFunction;
        ResponseLength = 2;
    }

    if(m_NRC == PR)
    {
        if(g_udsRunState.suppressResponse == FALSE)//需要正响应时，等级响应结束
        {
            g_udsRunState.WaitConfimBeforeReset = TRUE;
            PendingService = 0x11;
        }
        else//不需要正响应时直接复位
        {
            SystemReset(m_EcuResetType);
        }
    }
}

void Service27Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    uint8_t SubFunction ;
    uint8_t suppressPosRspMsgIndicationBit;
    m_NRC = PR;
    if(length >= 2)//min length check
    {
        uint8_t index = 0;
        bool subFunctionExist = FALSE;
        bool subFunctionSupInSession = FALSE;
        SubFunction = *(MessageData + 1) & 0x7F;
        suppressPosRspMsgIndicationBit = *(MessageData + 1) & 0x80;
        while(index < MAX_SECURITY_NUM && (!subFunctionExist))
        {
            if(UnlockList[index].valid == TRUE && (UnlockList[index].seedID == SubFunction || UnlockList[index].keyID == SubFunction))
            {
                subFunctionExist = TRUE;
                if(m_CurrSessionType == ECU_DEFAULT_SESSION)
                {
                    if(UnlockList[index].subFunctionSupported & SUB_DEFAULT)
                    {
                        subFunctionSupInSession = TRUE;
                    }
                    else
                    {
                        subFunctionSupInSession = FALSE;
                    }
                }
                else if(m_CurrSessionType == ECU_PAOGRAM_SESSION)
                {
                    if(UnlockList[index].subFunctionSupported & SUB_PROGRAM)
                    {
                        subFunctionSupInSession = TRUE;
                    }
                    else
                    {
                        subFunctionSupInSession = FALSE;
                    }
                }
                else if(m_CurrSessionType == ECU_EXTENED_SESSION)
                {
                    if(UnlockList[index].subFunctionSupported & SUB_EXTENDED)
                    {
                        subFunctionSupInSession = TRUE;
                    }
                    else
                    {
                        subFunctionSupInSession = FALSE;
                    }
                }
            }
            else
            {
                index++;
            }
        }
        
        if(subFunctionExist && subFunctionSupInSession)//sub function check ok
        {
            if(UnlockList[index].seedID == SubFunction)//request seed
            {
                if(length == 2)//length check again
                {
                    if(m_UnlockStep == WAIT_DELAY)
                    {
                        m_NRC = RTDNE;
                    }
                    else if(m_UnlockStep == UNLOCKED && m_SecurityLevel == UnlockList[index].level)//by ukign 20160401,when ECU unlocked,retrun seed is all zero
                    {
                        DiagnosticBuffTX[0] = 0x67;
                        DiagnosticBuffTX[1] = SubFunction;
                        DiagnosticBuffTX[2] = 0;
                        DiagnosticBuffTX[3] = 0;
                        DiagnosticBuffTX[4] = 0;
                        DiagnosticBuffTX[5] = 0;
                        ResponseLength = UnlockList[index].KeySize + 2;
                    }
                    else
                    {
                        if(m_UnlockStep == WAIT_KEY)
                            {
                                            (*UnlockList[index].FaultCounter)++;
                                            if((*UnlockList[index].FaultCounter) >= UnlockList[index].FaultLimitCounter)
                                            {
                                                m_UnlockStep = WAIT_DELAY;
                                                DiagTimer_Set(UnlockList[index].SecurityLockTimer , UnlockList[index].UnlockFailedDelayTime);
                                            }
                                       }
                                       else
                                       {
                               GenerateSeed(Seed,4);
                                           m_UnlockStep = WAIT_KEY;                                            
                                       }
                        DiagnosticBuffTX[0] = 0x67;
                        DiagnosticBuffTX[1] = SubFunction;
                        DiagnosticBuffTX[2] = Seed[0];
                        DiagnosticBuffTX[3] = Seed[1];
                        if(UnlockList[index].KeySize == 2)
                        {
                            
                        }
                        else if(UnlockList[index].KeySize == 4)
                        {
                            DiagnosticBuffTX[4] = Seed[2];
                            DiagnosticBuffTX[5] = Seed[3];
                        }
                        ResponseLength = UnlockList[index].KeySize + 2;
                    }
                }
                else
                {
                    m_NRC = IMLOIF;
                }
            }
            else if(SubFunction ==  UnlockList[index].keyID)//send key
            {
                if(length == UnlockList[index].KeySize + 2)
                {
                    if(m_UnlockStep == WAIT_KEY)
                    {
                        //uint32_t key1 = UnlockList[index].UnlockFunction(*(uint32_t*)Seed); 
                        *((uint32_t*)key) = UnlockList[index].UnlockFunction(*(uint32_t*)Seed);
                        Diagnostic_EEProm_Read(UnlockList[index].FaultCounterAddr, 1 ,UnlockList[index].FaultCounter);
                        //if(key[3] == *(MessageData + 2) && key[2] == *(MessageData + 3) && key[1] == *(MessageData + 4) && key[0] == *(MessageData + 5))
                        //if((uint8_t)(key1 >> 24) == *(MessageData + 2) && (uint8_t)(key1 >> 16) == *(MessageData + 3) && (uint8_t)(key1 >> 8) == *(MessageData + 4) && (uint8_t)key1 == *(MessageData + 5))
                        if(key[0] == *(MessageData + 2) && key[1] == *(MessageData + 3) && key[2] == *(MessageData + 4) && key[3] == *(MessageData + 5))
                        {
                            m_UnlockStep = UNLOCKED;
                            (*UnlockList[index].FaultCounter) = 0;
                            m_SecurityLevel = UnlockList[index].level;
                            DiagnosticBuffTX[0] = 0x67;
                            DiagnosticBuffTX[1] = SubFunction;
                            ResponseLength = 2;
                            m_UpdateStep = DOWNLOAD_FLASH_DRIVER_34;
                        }
                        else 
                        {
                            (*UnlockList[index].FaultCounter)++;
                            if((*UnlockList[index].FaultCounter) >= UnlockList[index].FaultLimitCounter)
                            {
                                m_NRC = ENOA;
                                m_UnlockStep = WAIT_DELAY;
                                DiagTimer_Set(UnlockList[index].SecurityLockTimer , UnlockList[index].UnlockFailedDelayTime);
                            }
                            else
                            {
                                m_NRC = IK;
                                m_UnlockStep = WAIT_SEED_REQ;
                            }
                        }
                        Diagnostic_EEProm_Write(UnlockList[index].FaultCounterAddr, 1 ,UnlockList[index].FaultCounter);                    
                    }
                    else if(m_UnlockStep == WAIT_DELAY)
                    {
                        m_NRC = RTDNE;
                    }
                    else if(m_UnlockStep == WAIT_SEED_REQ)//send key before request seed order error
                    {
                        m_NRC = RSE;
                    }
                    else//unlocked condition error
                    {
                        m_NRC = RSE;
                    }
                }
                else
                {
                    m_NRC = IMLOIF;
                }
            }
        }
        else
        {
            if(!subFunctionExist)
            {
                m_NRC = SFNS;
            }
            else if(!subFunctionSupInSession)
            {
                m_NRC = SFNSIAS;
            }
        }
    }
    else
    {
        m_NRC = IMLOIF;
    }
    
    if ( (suppressPosRspMsgIndicationBit) && (m_NRC == 0x00) && (g_udsRunState.ResponsePending == FALSE))
    {
        g_udsRunState.suppressResponse = TRUE; /* flag to NOT send a positive response message */
    }
    else
    {
        g_udsRunState.suppressResponse = FALSE; /* flag to send the response message */
    }

}

void Service28Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    uint8_t SubFunction;
    uint8_t ControlParam;
    uint8_t suppressPosRspMsgIndicationBit;
    m_NRC = PR;
    if(length == 3)
    {
        if(g_udsCfgState.Service28SupressSupported)
        {
            SubFunction = *(MessageData + 1) & 0x7F;
            suppressPosRspMsgIndicationBit = *(MessageData + 1) & 0x80;
        }
        else
        {
            SubFunction = *(MessageData + 1);
            suppressPosRspMsgIndicationBit = 0;
        }
        ControlParam = *(MessageData + 2);
        
        switch(SubFunction)/* get sub-function parameter value without bit 7 */
        {
            case ERXTX:
                {
                    if(!g_udsCfgState.Service28Sub00Suppoted)
                    {
                        m_NRC = SFNS;
                    }
                }
                break;
            case ERXDTX: /* test if sub-function parameter value is supported */
                {
                    if(!g_udsCfgState.Service28Sub01Suppoted)
                    {
                        m_NRC = SFNS;
                    }
                }
                break;
            case DRXETX: /* test if sub-function parameter value is supported */
                {
                    if(!g_udsCfgState.Service28Sub02Suppoted)
                    {
                        m_NRC = SFNS;
                    }
                }
                break;
            case DRXTX: /* test if sub-function parameter value is supported */
                {
                    if(!g_udsCfgState.Service28Sub03Suppoted)
                    {
                        m_NRC = SFNS;
                    }
                }
                break;
            default:
                m_NRC = SFNS; /* NRC 0x12: sub-functionNotSupported */
        }
        
        if(m_NRC == 0)
        {
            #if 1
            switch(ControlParam)
            {
                case NCM:
                    {
                        if(!g_udsCfgState.Service28Type01Suppoted)
                        {
                            m_NRC = ROOR;
                        }
                    }
                    break;
                case NWMCM: /* test if sub-function parameter value is supported */
                    {
                        if(!g_udsCfgState.Service28Type02Suppoted)
                        {
                            m_NRC = ROOR;
                        }
                    }
                    break;
                case NWMCM_NCM: /* test if sub-function parameter value is supported */
                    {
                        if(!g_udsCfgState.Service28Type03Suppoted)
                        {
                            m_NRC = ROOR;
                        }
                    }
                    break;
                default:
                    m_NRC = ROOR; /* NRC 0x12: sub-functionNotSupported */
            }
            #endif
        }
        
    }
    else
    {
        m_NRC = IMLOIF;
    }
    
    if ( (suppressPosRspMsgIndicationBit) && (m_NRC == 0x00) && (g_udsRunState.ResponsePending == FALSE))
    {
        g_udsRunState.suppressResponse = TRUE; /* flag to NOT send a positive response message */
    }
    else
    {
        g_udsRunState.suppressResponse = FALSE; /* flag to send the response message */
    }

    if(m_NRC == 0x00)
    {
        //there is no app msg,it's no need to control
        DiagnosticBuffTX[0] = CurrentService + 0x40;
        DiagnosticBuffTX[1] = SubFunction;
        ResponseLength = 2;
    }
}

void Service3EHandle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    uint8_t SubFunction;
    uint8_t suppressPosRspMsgIndicationBit;
    m_NRC = PR;
    if(length == 2)
    {
        if(g_udsCfgState.Service3ESupressSupported)
        {
            SubFunction = *(MessageData + 1) & 0x7F;
            suppressPosRspMsgIndicationBit = *(MessageData + 1) & 0x80;
        }
        else
        {
            SubFunction = *(MessageData + 1);
            suppressPosRspMsgIndicationBit = 0;
        }
        
        if(SubFunction != 0)
        {
            m_NRC = SFNS;
        }
        else
        {
            DiagnosticBuffTX[0] = CurrentService+ 0x40;
            DiagnosticBuffTX[1] = SubFunction;
            ResponseLength = 2;
        }
    }
    else
    {
        m_NRC = IMLOIF;
    }
    
    if ( (suppressPosRspMsgIndicationBit) && (m_NRC == 0x00) && (g_udsRunState.ResponsePending == FALSE || PendingService != CurrentService))
    {
        g_udsRunState.suppressResponse = TRUE; /* flag to NOT send a positive response message */
    }
    else
    {
        g_udsRunState.suppressResponse = FALSE; /* flag to send the response message */
    }
}

void Service85Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    uint8_t SubFunction;
    uint8_t suppressPosRspMsgIndicationBit;
    m_NRC = PR;
    if(length == 2)
    {
        if(g_udsCfgState.Service85SupressSupported)
        {
            SubFunction = *(MessageData + 1) & 0x7F;
            suppressPosRspMsgIndicationBit = *(MessageData + 1) & 0x80;
        }
        else
        {
            SubFunction = *(MessageData + 1);
            suppressPosRspMsgIndicationBit = 0;
        }
        
        switch(SubFunction)/* get sub-function parameter value without bit 7 */
        {
            case 0x01:
                {
                    DiagnosticBuffTX[0] = CurrentService + 0x40;
                    DiagnosticBuffTX[1] = SubFunction;
                    ResponseLength = 2;
                }
                break;
            case 0x02: /* test if sub-function parameter value is supported */
                {
                    DiagnosticBuffTX[0] = CurrentService + 0x40;
                    DiagnosticBuffTX[1] = SubFunction;
                    ResponseLength = 2;
                }
                break;
            default:
                m_NRC = SFNS; /* NRC 0x12: sub-functionNotSupported */
        }
    }
    else
    {
        m_NRC = IMLOIF;
    }
    
    if ( (suppressPosRspMsgIndicationBit) && (m_NRC == 0x00) && (g_udsRunState.ResponsePending == FALSE))
    {
        g_udsRunState.suppressResponse = TRUE; /* flag to NOT send a positive response message */
    }
    else
    {
        g_udsRunState.suppressResponse = FALSE; /* flag to send the response message */
    }
}

DIDNode* SearchDidNode(uint16_t DID)
{
    uint8_t i;
    for(i = 0; i < MAX_DID_NUMBER ; i++)
    {
        if(DIDS[i].ID == DID)
        {
            return DIDS + i;
        }
    }
    return NULL;
}

void Service22Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    bool suppressPosRspMsgIndicationBit = FALSE;
    m_NRC = PR;
    if(length == 3)
    {
        uint16_t DID = (*(MessageData + 1) << 8) + *(MessageData + 2);
        DIDNode* didNode = SearchDidNode(DID);
        if(didNode == NULL)
        {
            m_NRC = ROOR;//PID not in our PID list
        }
        else if(didNode->didType == IO_DID)
        {
            if(didNode->RWAttr == WRITEONLY)
            {
                m_NRC = ROOR;//mabe a IO Control DID
            }
            else
            {
                DiagnosticBuffTX[0] = 0x62;
                DiagnosticBuffTX[1] = *(MessageData + 1);
                DiagnosticBuffTX[2] = *(MessageData + 2);
                memcpy(DiagnosticBuffTX + 3 , didNode->dataPointer ,didNode->dataLength);
                
                ResponseLength = didNode->dataLength + 3;
            }
        }
        else
        {
            if(didNode->RWAttr == WRITEONLY)
            {
                m_NRC = ROOR;//this DID maybe supported by 22 service but not supported by 2E service
            }
            else
            {
                DiagnosticBuffTX[0] = 0x62;
                DiagnosticBuffTX[1] = *(MessageData + 1);
                DiagnosticBuffTX[2] = *(MessageData + 2);
                if(didNode->didType == EEPROM_DID)
                {
                    Diagnostic_EEProm_Read(didNode->EEpromAddr, didNode->dataLength , DiagnosticBuffTX+3);
                }
                else
                {
                    memcpy(DiagnosticBuffTX + 3 , didNode->dataPointer ,didNode->dataLength);
                }
                ResponseLength = didNode->dataLength + 3;
            }
        }
    }
    else
    {
        m_NRC = IMLOIF;
    }

    if ( (suppressPosRspMsgIndicationBit) && (m_NRC == 0x00) && (g_udsRunState.ResponsePending == FALSE))
    {
        g_udsRunState.suppressResponse = TRUE; /* flag to NOT send a positive response message */
    }
    else
    {
        g_udsRunState.suppressResponse = FALSE; /* flag to send the response message */
    }
}

void Service23Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    
}

void Service2EHandle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    bool suppressPosRspMsgIndicationBit = FALSE;
    m_NRC = PR;
    if(length >= 3)
    {
        uint16_t DID = (*(MessageData + 1) << 8) + *(MessageData + 2);
        DIDNode* didNode = SearchDidNode(DID);
        if(didNode == NULL)
        {
            m_NRC = ROOR;//PID not in our PID list
        }
        else if(didNode->didType == IO_DID)
        {
            m_NRC = ROOR;//mabe a IO Control DID
        }
        else if(didNode->RWAttr == READONLY)
        {
            m_NRC = ROOR;//this DID maybe supported by 22 service but not supported by 2E service
        }
        else
        {
            if(didNode->dataLength + 3 == length)
            {
                if(didNode->didType == EEPROM_DID)
                {
                    Diagnostic_EEProm_Write(didNode->EEpromAddr , didNode->dataLength , (uint8_t*)(MessageData + 3));
                    if(0xF199 == DID)
                    {
                        m_UpdateStep = DOWNLOAD_FLASH_DRIVER_34;
                    }
                }
                else
                {
                    memcpy(didNode->dataPointer , MessageData + 3, didNode->dataLength);
                }
                DiagnosticBuffTX[0] = 0x6E;
                DiagnosticBuffTX[1] = *(MessageData + 1);
                DiagnosticBuffTX[2] = *(MessageData + 2);
                ResponseLength = 3;
            }
            else
            {
                m_NRC = IMLOIF;
            }
        }
    }
    else
    {
        m_NRC = IMLOIF;
    }
    
    if ( (suppressPosRspMsgIndicationBit) && (m_NRC == 0x00) && (g_udsRunState.ResponsePending == FALSE))
    {
        g_udsRunState.suppressResponse = TRUE; /* flag to NOT send a positive response message */
    }
    else
    {
        g_udsRunState.suppressResponse = FALSE; /* flag to send the response message */
    }
}


void Service3DHandle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    
}

bool CheckRountineSupportInSession(Routine routine)
{
    if(m_CurrSessionType == ECU_DEFAULT_SESSION)
    {
        if((routine.support & SUB_DEFAULT) == SUB_DEFAULT)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else if(m_CurrSessionType == ECU_PAOGRAM_SESSION)
    {
        if((routine.support & SUB_PROGRAM) == SUB_PROGRAM)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else if(m_CurrSessionType == ECU_EXTENED_SESSION)
    {
        if((routine.support & SUB_EXTENDED) == SUB_EXTENDED)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

void SetAppVliadFlag(bool Valid)
{
    if(Valid == FALSE)
    {
        #if(1 == APP_VALID_FLAG_SIZE)
        uint8_t validFlag = APP_IS_INVALID;//app invalid
        #elif(2 == APP_VALID_FLAG_SIZE)
        uint16_t validFlag = APP_IS_INVALID;//app invalid
        #elif(4 == APP_VALID_FLAG_SIZE)
        uint32_t validFlag = APP_IS_INVALID;//app invalid
        #endif
        
        Diagnostic_EEProm_Write(APP_VALID_FLAG_ADDRESS , APP_VALID_FLAG_SIZE , (uint8_t*)(&validFlag));
        if(g_udsRunState.IsNeedIncreaseAttempt == TRUE)
        {
            uint8_t programAttemptCounter ;
            g_udsRunState.IsNeedIncreaseAttempt = FALSE;
            Diagnostic_EEProm_Read(PROGRAM_ATTEMPT_ADDRESS, 1, &programAttemptCounter);
            if(programAttemptCounter < 254)//max 254
            {
                programAttemptCounter++;
                Diagnostic_EEProm_Write(PROGRAM_ATTEMPT_ADDRESS, 1, &programAttemptCounter);
            }
            else if(programAttemptCounter == 255)//default state set to 1
            {
                programAttemptCounter = 1;
                Diagnostic_EEProm_Write(PROGRAM_ATTEMPT_ADDRESS, 1, &programAttemptCounter);
            }
        }
    }
    else if(Valid == TRUE)
    {
        #if(1 == APP_VALID_FLAG_SIZE)
        uint8_t validFlag = APP_IS_VALID;//app valid
        #elif(2 == APP_VALID_FLAG_SIZE)
        uint16_t validFlag = APP_IS_VALID;//app valid
        #elif(4 == APP_VALID_FLAG_SIZE)
        uint32_t validFlag = APP_IS_VALID;//app valid
        #endif
        
        uint8_t programCounter;
        uint8_t programAttemptCounter = 0;
        Diagnostic_EEProm_Read(PROGRAM_SUCCESS_ADDRESS, 1, &programCounter);
        
        Diagnostic_EEProm_Write(APP_VALID_FLAG_ADDRESS , APP_VALID_FLAG_SIZE , (uint8_t*)(&validFlag));
        if(programCounter < 254)//max 254
        {
            programCounter++;
            Diagnostic_EEProm_Write(PROGRAM_SUCCESS_ADDRESS, 1, &programCounter);
        }
        else if(programCounter == 255)//default state set to 1
        {
            programCounter = 1;
            Diagnostic_EEProm_Write(PROGRAM_SUCCESS_ADDRESS, 1, &programCounter);
        }
        Diagnostic_EEProm_Write(PROGRAM_ATTEMPT_ADDRESS, 1, &programAttemptCounter);
    }
}

void EraseRoutineHandle(uint16_t length, uint8_t *MessageData)
{
    uint8_t SubFunction = *(MessageData + 1);
    if((m_currRoutine[0].SecuritySupport & m_SecurityLevel) == m_SecurityLevel)
    {
        if(CheckRountineSupportInSession(m_currRoutine[0]))
        {
            if(SubFunction == 0x01)
            {
                #if 1
                if(length == (m_currRoutine[0].OptionRecordLength + 4))
                {
                    if(g_udsRunState.WaitErase == FALSE)
                    {
                        if(m_UpdateStep >= ERASE_FLASH)
                        {
                            uint8_t eraseResult;
                            EraseAddress = *((uint32_t*)(MessageData + 4));
                            EraseLength = *((uint32_t*)(MessageData + 8));
                            eraseResult = MemManger_Erase(EraseAddress , EraseLength);
                            if(eraseResult == MEM_WAIT)
                            {
                                if(EraseAddress <= 0x13FF)
                                {
                                    g_udsRunState.IsCalibrationDataValid = FALSE;
                                }
                                else
                                {
                                    g_udsRunState.IsAppDataValid = FALSE;
                                }
                                SetAppVliadFlag(FALSE);
                                m_NRC = RCRRP;
                                g_udsRunState.WaitErase = TRUE;
                            }
                            else if(eraseResult == MEM_ERROR)
                            {
                                m_NRC = ROOR;
                            }
                        }
                        else
                        {
                            m_NRC = CNC;
                        }
                    }
                    else
                    {
                        DiagnosticBuffTX[0] = 0x71;
                        DiagnosticBuffTX[1] = 0x01;
                        DiagnosticBuffTX[2] = *(MessageData + 2);
                        DiagnosticBuffTX[3] = *(MessageData + 3);
                        DiagnosticBuffTX[4] = 0x02;//already started
                        ResponseLength = 5;
                    }
                }
                else
                {
                    m_NRC = IMLOIF;
                }
                #else 
                if(length >= 5)
                {
                    uint8_t AddressLength = (*(MessageData + 4) & 0x0F);
                    uint8_t SizeLength = (*(MessageData + 4) >> 4);
                    if(length == (AddressLength + SizeLength + 5) && (AddressLength != 0) && (SizeLength != 0))
                    {
                        if(g_udsRunState.WaitErase == FALSE)
                        {
                            if(m_UpdateStep >= ERASE_FLASH)
                            {
                                uint8_t eraseResult;
                                EraseAddress = *((uint32_t*)(MessageData + 5)) >> ((4 - AddressLength) * 8);
                                EraseLength = *((uint32_t*)(MessageData + 9)) >> ((4 - SizeLength) * 8);
                                eraseResult = MemManger_Erase(EraseAddress , EraseLength);
                                if(eraseResult == MEM_WAIT)
                                {
                                    if(EraseAddress <= 0x13FF)
                                    {
                                        g_udsRunState.IsCalibrationDataValid = FALSE;
                                    }
                                    else
                                    {
                                        g_udsRunState.IsAppDataValid = FALSE;
                                    }
                                    SetAppVliadFlag(FALSE);
                                    m_NRC = RCRRP;
                                    g_udsRunState.WaitErase = TRUE;
                                }
                                else if(eraseResult == MEM_ERROR)
                                {
                                    m_NRC = ROOR;
                                }
                            }
                            else
                            {
                                m_NRC = CNC;
                            }
                        }
                        else
                        {
                            DiagnosticBuffTX[0] = 0x71;
                            DiagnosticBuffTX[1] = 0x01;
                            DiagnosticBuffTX[2] = *(MessageData + 2);
                            DiagnosticBuffTX[3] = *(MessageData + 3);
                            DiagnosticBuffTX[4] = 0x02;//already started
                            ResponseLength = 5;
                        }
                    }
                    else
                    {
                        m_NRC = IMLOIF;
                    }
                }
                else
                {
                    m_NRC = IMLOIF;
                }
                #endif
            }
            else if(SubFunction == 0x03)
            {
                if(length == 4)
                {
                    DiagnosticBuffTX[0] = 0x71;
                    DiagnosticBuffTX[1] = 0x03;
                    DiagnosticBuffTX[2] = *(MessageData + 2);
                    DiagnosticBuffTX[3] = *(MessageData + 3);
                    if(g_udsRunState.WaitErase == TRUE)
                    {
                        DiagnosticBuffTX[4] = 0x02;//still runnig
                    }
                    else
                    {
                        DiagnosticBuffTX[4] = 0x00;//finished
                    }
                    ResponseLength = 5;
                }
                else
                {
                    m_NRC = IMLOIF;
                }
            }
            else
            {
                m_NRC = SFNS;
            }
        }
        else
        {
            m_NRC = ROOR;
        }
    }
    else
    {
        m_NRC = SAD ;
    }
}

void ProgramIntegrityRoutineHandle(uint16_t length, uint8_t *MessageData)
{
    uint8_t SubFunction = *(MessageData + 1);
    if(length == (m_currRoutine[2].OptionRecordLength + 4))
    {
        if(m_UpdateStep == CHECK_DRIVER_CRC || m_UpdateStep == CHECK_APP_CRC)
        {
            if(m_SecurityLevel == (m_currRoutine[2].SecuritySupport & m_SecurityLevel))
            {
                if(CheckRountineSupportInSession(m_currRoutine[2]))
                {
                    if(SubFunction == 0x01)
                    {
                        if(length == (m_currRoutine[2].OptionRecordLength + 4))
                        {
                            DiagnosticBuffTX[0] = 0x71;
                            DiagnosticBuffTX[1] = 0x01;
                            DiagnosticBuffTX[2] = (uint8_t)(m_currRoutine[2].RID >> 8);
                            DiagnosticBuffTX[3] = (uint8_t)(m_currRoutine[2].RID);
                            ResponseLength = 5;
                            BlockCRC = *(uint32_t*)(MessageData + 4);
                            if(MemManger_CkeckIntegrity(BlockCRC))
                            {
                                DiagnosticBuffTX[4] = 0x00;
                                if(m_UpdateStep == CHECK_DRIVER_CRC)
                                {
                                    m_UpdateStep = ERASE_FLASH;
                                    g_udsRunState.IsNeedIncreaseAttempt = TRUE;//program attempt counter will increase

                                }
                                else if(m_UpdateStep == CHECK_APP_CRC)
                                {
                                    m_UpdateStep = CHECK_VALID_APP;
                                    if(MemManger_ValidRange(EEPROM_SPACE_INDEX , ProgramAddress , ProgramLength))
                                    {
                                        g_udsRunState.IsCalibrationDataValid = TRUE;
                                    }
                                    else if(MemManger_ValidRange(FLASH_SPACE_INDEX , ProgramAddress , ProgramLength))
                                    {
                                        g_udsRunState.IsAppDataValid = TRUE;
                                    }
                                }
                                else
                                {
                                    m_UpdateStep = PRE_STEP;
                                }
                            }
                            else 
                            {
                                DiagnosticBuffTX[4] = 0x01;
                                m_UpdateStep = PRE_STEP;
                            }
                        }
                        else
                        {
                            m_NRC = IMLOIF; //0x13
                        }
                    }
                    else if(SubFunction == 0x03)
                    {
                        m_NRC = SFNS; //0x12
                    }
                    else
                    {
                        m_NRC = SFNS; //0x12
                    }
                }
                else
                {
                    m_NRC = ROOR; //0x31
                }
            }
            else
            {
                m_NRC = SAD; //0x33
            }
        }
        else
        {
            m_NRC = RSE;//0x24
        }
    }
    else
    {
      m_NRC = IMLOIF; //0x13
    }
}

void ProgramDependencyRoutineHandle(uint16_t length, uint8_t *MessageData)
{
    uint8_t SubFunction = *(MessageData + 1);
    if(m_SecurityLevel == (m_currRoutine[3].SecuritySupport & m_SecurityLevel))
    {
        if(CheckRountineSupportInSession(m_currRoutine[3]))
        {
            if(SubFunction == 0x01)
            {
                if(length == (m_currRoutine[3].OptionRecordLength + 4))
                {
                    //if((VerifyProjectName() == TRUE) && (CheckProgramComplete()))
                    if(MemManger_CkeckDependency())
                    {
                        SetAppVliadFlag(TRUE);
                        
                        DiagnosticBuffTX[0] = 0x71;
                        DiagnosticBuffTX[1] = 0x01;
                        DiagnosticBuffTX[2] = *(MessageData + 2);
                        DiagnosticBuffTX[3] = *(MessageData + 3);
                        DiagnosticBuffTX[4] = 0x00;
                        ResponseLength = 5;                            
                    }
                    else
                    {
                        DiagnosticBuffTX[0] = 0x71;
                        DiagnosticBuffTX[1] = 0x01;
                        DiagnosticBuffTX[2] = *(MessageData + 2);
                        DiagnosticBuffTX[3] = *(MessageData + 3);
                        DiagnosticBuffTX[4] = 0x01;
                        ResponseLength = 5;    
                    }
                    m_UpdateStep = PRE_STEP;
                }
                else
                {
                    m_NRC = IMLOIF;
                }
            }
            else if(SubFunction == 0x03)
            {
                m_NRC = SFNS;
            }
            else
            {
                m_NRC = SFNS;
            }
        }
        else
        {
            m_NRC = ROOR;
        }
    }
    else
    {
        m_NRC = SAD ;
    }
}

void EnableProgramRoutineHandle(uint16_t length, uint8_t *MessageData)
{
    uint8_t SubFunction = *(MessageData + 1);
    if(m_SecurityLevel == (m_currRoutine[4].SecuritySupport & m_SecurityLevel))
    {
        if(CheckRountineSupportInSession(m_currRoutine[4]))
        {
            if(SubFunction == 0x01)
            {
                if(length == (m_currRoutine[4].OptionRecordLength + 4))
                {
                    uint32_t address  = *(uint32_t*)(MessageData + 4);
                    if((address == ProgramAddress) && (CheckFblFlashCrcValid()))
                    {
                        if(m_UpdateStep == ENABLE_PROGRAM)
                        {
                            DiagnosticBuffTX[0] = 0x71;
                            DiagnosticBuffTX[1] = 0x01;
                            DiagnosticBuffTX[2] = *(MessageData + 2);
                            DiagnosticBuffTX[3] = *(MessageData + 3);
                            ResponseLength = 4;                        
                            m_UpdateStep = ERASE_FLASH;

                        }
                        else
                        {
                            m_NRC = RSE;
                        }
                    }
                    else
                    {
                        m_NRC = CNC;
                    }
                }
                else
                {
                    m_NRC = IMLOIF;
                }
            }
            else if(SubFunction == 0x03)
            {
                m_NRC = SFNS;
            }
            else
            {
                m_NRC = SFNS;
            }
        }
        else
        {
            m_NRC = ROOR;
        }
    }
    else
    {
        m_NRC = SAD ;
    }
}

void Service31Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    bool suppressPosRspMsgIndicationBit = FALSE;
    m_NRC = PR;
    if(length >= 4)
    {
        uint8_t SubFunction = *(MessageData + 1);
        uint16_t RID = (*(MessageData + 2) << 8) + *(MessageData + 3);
        if(RID == m_currRoutine[1].RID)//check precondition, bootloader always positive response
        {
            if(CheckRountineSupportInSession(m_currRoutine[1]))
            {
                if(length == (m_currRoutine[1].OptionRecordLength + 4))
                {
                    DiagnosticBuffTX[0] = 0x71;
                    DiagnosticBuffTX[1] = 0x01;
                    DiagnosticBuffTX[2] = *(MessageData + 2);
                    DiagnosticBuffTX[3] = *(MessageData + 3);
                    DiagnosticBuffTX[4] = 0x00;
                    ResponseLength = 5;
                }
                else
                {
                    m_NRC = IMLOIF;
                }
            }
            else
            {
                m_NRC = ROOR;
            }
        }
        else if(RID == m_currRoutine[0].RID)//erase memory routine
        {
            EraseRoutineHandle(length , MessageData);
        }
        else if(RID == m_currRoutine[2].RID)//check program integrity
        {
            ProgramIntegrityRoutineHandle(length , MessageData);
        }
        else if(RID == m_currRoutine[3].RID)//check program dependency
        {
            ProgramDependencyRoutineHandle( length , MessageData);
        }
        else if(RID == m_currRoutine[4].RID)//enable flash progaming
        {
            EnableProgramRoutineHandle(length , MessageData);
        }
        else
        {
            m_NRC = ROOR;
        }
    }
    else
    {
        m_NRC = IMLOIF;
    }

    if(m_NRC != 0x00 && m_NRC != RCRRP)
    {
        m_UpdateStep = PRE_STEP;
    }

    if ( (suppressPosRspMsgIndicationBit) && (m_NRC == 0x00) && (g_udsRunState.ResponsePending == FALSE))
    {
        g_udsRunState.suppressResponse = TRUE; /* flag to NOT send a positive response message */
    }
    else
    {
        g_udsRunState.suppressResponse = FALSE; /* flag to send the response message */
    }
}

void Service34Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    bool suppressPosRspMsgIndicationBit = FALSE;
    m_NRC = PR;
    if(length == 11)
    {
        uint8_t SubFunction = *(MessageData + 1);
        uint8_t FormatID = *(MessageData + 2);
        if(SubFunction == 0 && FormatID == 0x44)
        {
            ProgramAddress = *((uint32_t*)(MessageData + 3));
            ProgramLength = *((uint32_t*)(MessageData + 7));
            if(MemManger_ValidRange(RAM_SPACE_INDEX , ProgramAddress , ProgramLength))
            {
                if(m_UpdateStep == DOWNLOAD_FLASH_DRIVER_34)
                {
                    DiagnosticBuffTX[0] = 0x74;
                    DiagnosticBuffTX[1] = 0x20;
                    DiagnosticBuffTX[2] = (uint8_t)(MAX_REQUEST_SIZE >> 8);
                    DiagnosticBuffTX[3] = (uint8_t)(MAX_REQUEST_SIZE);
                    ResponseLength = 4;
                    m_BlockIndex = 1;
                    ProgramLengthComplete = 0;
                    MemManger_Request(ProgramAddress , ProgramLength);
                    m_UpdateStep = DOWNLOAD_FLASH_DRIVER_36;
                }
                else
                {
                    m_NRC = RSE;
                }
            }
            else if(MemManger_ValidAddress(ProgramAddress , ProgramLength) == TRUE)
            {
                if(m_UpdateStep == TRANSFER_APP_DATA_34)
                {
                    DiagnosticBuffTX[0] = 0x74;
                    DiagnosticBuffTX[1] = 0x20;
                    DiagnosticBuffTX[2] = (uint8_t)(MAX_REQUEST_SIZE >> 8);
                    DiagnosticBuffTX[3] = (uint8_t)(MAX_REQUEST_SIZE);
                    ResponseLength = 4;
                    m_BlockIndex = 1;
                    ProgramLengthComplete = 0;
                    MemManger_Request(ProgramAddress , ProgramLength);
                    m_UpdateStep = TRANSFER_APP_DATA_36;
                }
                else
                {
                    m_NRC = UDNA;
                }
            }
            else
            {
                m_NRC = ROOR;
            }
        }
        else
        {
            m_NRC = ROOR;
        }
    }
    else
    {
        m_NRC = IMLOIF;
    }

    if(m_NRC != 0x00 && m_NRC != RCRRP)
    {
        m_UpdateStep = PRE_STEP;
        m_UpdateStep = DOWNLOAD_FLASH_DRIVER_34; //Added on 2018.10.19
    }
    
    if ( (suppressPosRspMsgIndicationBit) && (m_NRC == 0x00) && (g_udsRunState.ResponsePending == FALSE))
    {
        g_udsRunState.suppressResponse = TRUE; /* flag to NOT send a positive response message */
    }
    else
    {
        g_udsRunState.suppressResponse = FALSE; /* flag to send the response message */
    }
}

void Service35Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    
}

void Service36Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    bool suppressPosRspMsgIndicationBit = FALSE;
    m_NRC = PR;
    if(length <= MAX_REQUEST_SIZE && length >= 3)//min one byte data
    {
        uint8_t BLockIndex = *(MessageData + 1);
        if(m_UpdateStep == DOWNLOAD_FLASH_DRIVER_36 || m_UpdateStep == TRANSFER_APP_DATA_36)
        {
            if(BLockIndex == m_BlockIndex)
            {
                uint16_t errorCode = 0;
                errorCode = MemManger_Program( length - 2 , MessageData + 2);
                if(errorCode == MEM_FINISH)
                {                                                        
                    ProgramLengthComplete += length - 2;
                    LastRequestLength = length - 2;
                    DiagnosticBuffTX[0] = 0x76;
                    DiagnosticBuffTX[1] = m_BlockIndex;
                    ResponseLength = 2;
                    
                    m_BlockIndex++;
                    if(ProgramLengthComplete == ProgramLength)
                    {
                        m_UpdateStep = DOWNLOAD_FLASH_DRIVER_37;
                    }
                }
                else if(errorCode == MEM_WAIT)
                {
                    g_udsRunState.WaitProgram = TRUE;
                    ProgramLengthComplete += length - 2;
                    LastRequestLength = length - 2;
                    m_NRC = RCRRP;

                    if(ProgramLengthComplete == ProgramLength)
                    {
                        m_UpdateStep = TRANSFER_APP_DATA_37;
                    }
                }
                else
                {
                    m_NRC = GPF;//program error
                }
            }
            /*已编程字节不为0(不是34服务后的第一个36服务)时，进行重复传输处理*/
            else if((m_BlockIndex == (BLockIndex + 1)) && (ProgramLengthComplete != 0))
            {
                /*如果本次传输的数据大于上次传输的数据，
                则将大于的部分数据写入FLASH*/
                if((length - 2) > LastRequestLength)
                {
                    uint16_t errorCode = 0;
                    uint16_t thisRequestLength = length - 2 - LastRequestLength;
                    errorCode = MemManger_Program( thisRequestLength , MessageData + 2 + LastRequestLength);
                    if(errorCode == MEM_FINISH)
                    {                                                        
                        ProgramLengthComplete += thisRequestLength;
                        LastRequestLength += thisRequestLength;
                        DiagnosticBuffTX[0] = 0x76;
                        DiagnosticBuffTX[1] = m_BlockIndex;
                        ResponseLength = 2;
                        
                        if(ProgramLengthComplete == ProgramLength)
                        {
                            m_UpdateStep = DOWNLOAD_FLASH_DRIVER_37;
                        }
                    }
                    else if(errorCode == MEM_WAIT)
                    {
                        g_udsRunState.WaitProgram = TRUE;
                        LastRequestLength += thisRequestLength;
                        ProgramLengthComplete += thisRequestLength;
                        m_NRC = RCRRP;

                        if(ProgramLengthComplete == ProgramLength)
                        {
                            m_UpdateStep = TRANSFER_APP_DATA_37;
                        }
                    }
                    else
                    {
                        m_NRC = GPF;//program error
                    }
                }
                else/*否则(本次传输的数据小于等于上一次传输的数据)，直接给肯定响应*/
                {
                    DiagnosticBuffTX[0] = 0x76;
                    DiagnosticBuffTX[1] = m_BlockIndex;
                    ResponseLength = 2;
                }
            }
            else
            {
                m_NRC = WBSC;
            }
        }
        else if((m_UpdateStep == DOWNLOAD_FLASH_DRIVER_37 || m_UpdateStep == TRANSFER_APP_DATA_37))//add 20190920 for geely 
        {
            /*如果数据已传输完成，收到上次重复的36服务，且长度相等，直接给肯定响应*/
            if(m_BlockIndex == (BLockIndex + 1) && ((length - 2) == LastRequestLength))
            {
                DiagnosticBuffTX[0] = 0x76;
                DiagnosticBuffTX[1] = *(MessageData + 1) ;
                ResponseLength = 2;
            }
            else/*否则认为请求顺序错误，给出NRC24*/
            {
                m_NRC = RSE;
            }
        }
        else
        {
            m_NRC = RSE;
        }
    }
    else
    {
        m_NRC = IMLOIF;
    }
    
    if(m_NRC != 0x00 && m_NRC != RCRRP)
    {
        m_UpdateStep = PRE_STEP;
        m_UpdateStep = DOWNLOAD_FLASH_DRIVER_34; //Added on 2018.10.19
    }

    if ( (suppressPosRspMsgIndicationBit) && (m_NRC == 0x00) && (g_udsRunState.ResponsePending == FALSE))
    {
        g_udsRunState.suppressResponse = TRUE; /* flag to NOT send a positive response message */
    }
    else
    {
        g_udsRunState.suppressResponse = FALSE; /* flag to send the response message */
    } 
}

void Service37Handle(uint8_t N_TAType, uint16_t length, uint8_t *MessageData)
{
    bool suppressPosRspMsgIndicationBit = FALSE;
    m_NRC = PR;
    if(length == 1)
    {
        if(m_UpdateStep == DOWNLOAD_FLASH_DRIVER_37)
        {
            m_UpdateStep = CHECK_DRIVER_CRC;
            DiagnosticBuffTX[0] = 0x77;
            ResponseLength = 1;
        }
        else if(m_UpdateStep == TRANSFER_APP_DATA_37)
        {
            m_UpdateStep = CHECK_APP_CRC;
            DiagnosticBuffTX[0] = 0x77;
            ResponseLength = 1;
        }
        else
        {
            m_NRC = RSE;
        }
    }
    else
    {
        m_NRC = IMLOIF;
    }

    if(m_NRC != 0x00 && m_NRC != RCRRP)
    {
        m_UpdateStep = PRE_STEP;
    }

    if ( (suppressPosRspMsgIndicationBit) && (m_NRC == 0x00) && (g_udsRunState.ResponsePending == FALSE))
    {
        g_udsRunState.suppressResponse = TRUE; /* flag to NOT send a positive response message */
    }
    else
    {
        g_udsRunState.suppressResponse = FALSE; /* flag to send the response message */
    }
}

void ServiceNegReponse(uint8_t serviceName,uint8_t RejectCode)
{
    DiagnosticBuffTX[0] = 0x7F;
    DiagnosticBuffTX[1] = serviceName;
    DiagnosticBuffTX[2] = RejectCode;
    N_USData_request(DIAGNOSTIC , N_Sa ,  N_Ta , PHYSICAL , 0 , DiagnosticBuffTX , 3);
}

void Diagnostic_ServiceHandle(uint8_t N_SA , uint8_t N_TA , uint8_t N_TAtype , uint16_t length , uint8_t *MessageData)
{
    uint8_t  SIDIndex;
    bool ValidSid;
    uint16_t ServiceIndex;
    uint8_t DataIndex;
    ValidSid = FALSE;
    ServiceIndex = 0;
    CurrentService = MessageData[0];    

    while((ServiceIndex < SERVICE_NUMBER) && (!ValidSid))
    {
        if(ServiceList[ServiceIndex].serviceName == CurrentService)
        {
            if(ServiceList[ServiceIndex].support == TRUE)
            {
                ValidSid = TRUE;
            }
            else//found service but service not enable by application layer
            {
                ValidSid = FALSE;
                break;
            }
        }
        else
        {
            ServiceIndex++;
        }
    }
    
    if(ValidSid == TRUE)
    {
        if(N_TAtype == PHYSICAL)
        {
            g_udsRunState.suppressResponse = FALSE;
            if(ECU_DEFAULT_SESSION == m_CurrSessionType)
            {
                if(ServiceList[ServiceIndex].PHYDefaultSession_Security == UNSUPPORT)
                {
                    m_NRC = SNSIAS;//ServiceNegReponse(ServiceName,SNSIAS);
                }
                else
                {
                    if((ServiceList[ServiceIndex].PHYDefaultSession_Security & m_SecurityLevel) == m_SecurityLevel)
                    {
                        ServiceList[ServiceIndex].serviceHandle(N_TAtype,length,MessageData);
                        //LastService = ServiceList[ServiceIndex];
                    }
                    else
                    {
                        m_NRC = SAD;//ServiceNegReponse(ServiceName,SAD);
                    }
                }
            }
            else if(ECU_EXTENED_SESSION == m_CurrSessionType)
            {
                if(ServiceList[ServiceIndex].PHYExtendedSession_Security == UNSUPPORT)
                {
                    m_NRC = SNSIAS;//ServiceNegReponse(ServiceName,SNSIAS);
                }
                else
                {
                    if((ServiceList[ServiceIndex].PHYExtendedSession_Security & m_SecurityLevel) == m_SecurityLevel)
                    {
                        ServiceList[ServiceIndex].serviceHandle(N_TAtype,length,MessageData);
                        //LastService = ServiceList[ServiceIndex];
                    }
                    else
                    {
                        m_NRC = SAD;//ServiceNegReponse(ServiceName,SAD);
                    }
                }
            }
            else if(ECU_PAOGRAM_SESSION == m_CurrSessionType)
            {
                if(ServiceList[ServiceIndex].PHYProgramSeesion_Security == UNSUPPORT)
                {
                    m_NRC = SNSIAS;//ServiceNegReponse(ServiceName,SNSIAS);
                }
                else
                {
                    if((ServiceList[ServiceIndex].PHYProgramSeesion_Security & m_SecurityLevel) == m_SecurityLevel)
                    {
                        ServiceList[ServiceIndex].serviceHandle(N_TAtype,length,MessageData);
                        //LastService = ServiceList[ServiceIndex];
                    }
                    else
                    {
                        m_NRC = SAD;//ServiceNegReponse(ServiceName,SAD);
                    }
                }
            }
        }
        else if(N_TAtype == FUNCTIONAL)
        {
            if(ECU_DEFAULT_SESSION == m_CurrSessionType)
            {
                if(ServiceList[ServiceIndex].FUNDefaultSession_Security == UNSUPPORT)
                {
                    m_NRC = SNSIAS;//ServiceNegReponse(ServiceName,SNSIAS);
                }
                else
                {
                    if((ServiceList[ServiceIndex].FUNDefaultSession_Security & m_SecurityLevel) == m_SecurityLevel)
                    {
                        ServiceList[ServiceIndex].serviceHandle(N_TAtype,length,MessageData);
                        //LastService = ServiceList[ServiceIndex];
                    }
                    else
                    {
                        m_NRC = SAD;//ServiceNegReponse(ServiceName,SAD);
                    }
                }
            }
            else if(ECU_EXTENED_SESSION == m_CurrSessionType)
            {
                if(ServiceList[ServiceIndex].FUNExtendedSession_Security == UNSUPPORT)
                {
                    m_NRC = SNSIAS;//ServiceNegReponse(ServiceName,SNSIAS);
                }
                else
                {
                    if((ServiceList[ServiceIndex].FUNExtendedSession_Security & m_SecurityLevel) == m_SecurityLevel)
                    {
                        ServiceList[ServiceIndex].serviceHandle(N_TAtype,length,MessageData);
                        //LastService = ServiceList[ServiceIndex];
                    }
                    else
                    {
                        m_NRC = SAD;//ServiceNegReponse(ServiceName,SAD);
                    }
                }
            }
            else if(ECU_PAOGRAM_SESSION == m_CurrSessionType)
            {
                if(ServiceList[ServiceIndex].FUNProgramSeesion_Security == UNSUPPORT)
                {
                    m_NRC = SNSIAS;//ServiceNegReponse(ServiceName,SNSIAS);
                }
                else
                {
                    if((ServiceList[ServiceIndex].FUNProgramSeesion_Security & m_SecurityLevel) == m_SecurityLevel)
                    {
                        ServiceList[ServiceIndex].serviceHandle(N_TAtype,length,MessageData);
                        //LastService = ServiceList[ServiceIndex];
                    }
                    else
                    {
                        m_NRC = SAD;//ServiceNegReponse(ServiceName,SAD);
                    }
                }
            }
        }
    }
    else
    {
        if(N_TAtype == PHYSICAL)//功能寻址无效SED不响应
        {
            m_NRC = SNS;//ServiceNegReponse(ServiceName,SNS);
            g_udsRunState.suppressResponse = FALSE;
        }
        else
        {
            g_udsRunState.suppressResponse = TRUE;
        }
    }
}

void Diagnostic_MainProc(void)
{
    uint32_t rxId = 0;
    bool ExistValidNotify = FALSE;
    
    if(!IsIndicationListEmpty())
    {
        NetworkNotification temp = PullIndication();
        rxId = ((temp.N_SA << 8) + temp.N_TA);
        
        if(temp.NotificationType == INDICATION)
        {
            if((((rxId & 0xFFFF) == (PHY_REQ_ID & 0xFFFF)) && !g_udsRunState.ResponsePending) || ((rxId & 0xFFFF) == (FUN_REQ_ID & 0xFFFF)))
            {
                if(temp.N_Resut == N_OK ||temp.N_Resut == N_UNEXP_PDU)
                {
                    Diagnostic_ServiceHandle(temp.N_SA,temp.N_TA,temp.N_TAtype,temp.length, temp.MessageData);

                    if((temp.N_TAtype == FUNCTIONAL) && ((m_NRC == SNS) || (m_NRC == SFNS) || (m_NRC == SFNSIAS) || (m_NRC == ROOR)) && (g_udsRunState.ResponsePending == FALSE || PendingService != CurrentService))
                    {
                        /* suppress negative response message */
                    }
                    else if (g_udsRunState.suppressResponse == TRUE)
                    {    
                        /* suppress positive response message */
                    }
                    else
                    {
                        if(m_NRC == PR)
                        {
                            N_USData_request(DIAGNOSTIC , N_Sa ,  N_Ta , PHYSICAL , 0 , DiagnosticBuffTX , ResponseLength);
                        }
                        else
                        {
                            ServiceNegReponse(CurrentService,m_NRC);
                            if(m_NRC == RCRRP)
                            {
                                DiagTimer_Set(&PendingTimer, P2ECanServerMax*10);
                                PendingService = CurrentService;
                                g_udsRunState.ResponsePending = TRUE;
                            }
                        }
                    }
                    
                    if(m_CurrSessionType  != ECU_DEFAULT_SESSION)
                    {
                        DiagTimer_Set(&S3serverTimer, 5000);
                    }
                    DiagRequested();
                    
                }
            }
        }
        else if(temp.NotificationType == CONFIRM)
        {
            if((rxId & 0xFFFF) == (UDS_RES_ID & 0xFFFF))
            {
                if(PendingService == 0x11)
                {
                    if(g_udsRunState.WaitConfimBeforeReset == TRUE)
                    {
                        g_udsRunState.WaitConfimBeforeReset = FALSE;
                        PendingService = 0;
                        SystemReset(m_EcuResetType);
                    }
                }
                else if(PendingService == 0x10)
                {
                    if(g_udsRunState.WaitConfimBeforeReset == TRUE)
                    {
                        g_udsRunState.WaitConfimBeforeReset = FALSE;
                        PendingService = 0;
                        SystemReset(m_EcuResetType);
                    }
                }
            }
        }
        else if(temp.NotificationType == FF_INDICATION)
        {
        }
    }
    else if(g_udsRunState.ResponsePending)
    {
        if(PendingService == 0x31)
        {
            if(g_udsRunState.WaitErase == TRUE)
            {
                if(MemManger_GetEraseStatus() == MEM_FINISH)
                {
                    DiagnosticBuffTX[0] = 0x71;
                    DiagnosticBuffTX[1] = 0x01;
                    DiagnosticBuffTX[2] = (uint8_t)(m_currRoutine[0].RID >> 8);
                    DiagnosticBuffTX[3] = (uint8_t)(m_currRoutine[0].RID);
                    ResponseLength = 4;
                    N_USData_request(DIAGNOSTIC , N_Sa ,  N_Ta , PHYSICAL , 0 , DiagnosticBuffTX , ResponseLength);
                    PendingService = 0;
                    m_UpdateStep = TRANSFER_APP_DATA_34;
                    g_udsRunState.ResponsePending = FALSE;
                    DiagTimer_Stop(&PendingTimer);
                    g_udsRunState.WaitErase = FALSE;
                }
                else if(MemManger_GetEraseStatus() == MEM_ERROR)
                {
                    m_NRC = GPF;
                    ServiceNegReponse(PendingService,m_NRC);
                    PendingService = 0;
                    m_UpdateStep = PRE_STEP;
                    g_udsRunState.ResponsePending = FALSE;
                    DiagTimer_Stop(&PendingTimer);
                    g_udsRunState.WaitErase = FALSE;
                }
            }
        }
        else if(PendingService == 0x36)
        {
            if(g_udsRunState.WaitProgram == TRUE)
            {
                if(MemManger_GetProgramStatus() == MEM_FINISH)
                {
                    DiagnosticBuffTX[0] = 0x76;
                    DiagnosticBuffTX[1] = m_BlockIndex;
                    ResponseLength = 2;
                    
                    m_BlockIndex++;
                    N_USData_request(DIAGNOSTIC , N_Sa ,  N_Ta , PHYSICAL , 0 , DiagnosticBuffTX , ResponseLength);
                   
                    PendingService = 0;
                    g_udsRunState.ResponsePending = FALSE;
                    g_udsRunState.WaitProgram = FALSE;
                    DiagTimer_Stop(&PendingTimer);
                }
                else if(MemManger_GetProgramStatus() == MEM_ERROR)
                {
                    m_NRC = GPF;
                    ServiceNegReponse(PendingService,m_NRC);
                    PendingService = 0;
                    m_UpdateStep = PRE_STEP;
                    g_udsRunState.ResponsePending = FALSE;
                    g_udsRunState.WaitProgram = FALSE;
                    DiagTimer_Stop(&PendingTimer);
                }                
                else if(MemManger_GetProgramStatus() == MEM_LOW_VOL)
                {
                    m_NRC = VTL;            
                    ServiceNegReponse(PendingService,m_NRC);
                    PendingService = 0;
                    g_udsRunState.ResponsePending = FALSE;
                    g_udsRunState.WaitProgram = FALSE;
                    DiagTimer_Stop(&PendingTimer);
                }
                else if(MemManger_GetProgramStatus() == MEM_HIGH_VOL)
                {
                    m_NRC = VTH;            
                    ServiceNegReponse(PendingService,m_NRC);
                    PendingService = 0;
                    g_udsRunState.ResponsePending = FALSE;
                    g_udsRunState.WaitProgram = FALSE;
                    DiagTimer_Stop(&PendingTimer);
                }
            }
        }

        if(DiagTimer_HasExpired(&PendingTimer))
        {
            DiagTimer_Set(&PendingTimer, P2ECanServerMax*10);
            m_NRC = RCRRP;
            ServiceNegReponse(PendingService,m_NRC);
        }
    }
}

void Diagnostic_TimeProc(void)
{
    if(m_CurrSessionType != ECU_DEFAULT_SESSION)
    {
        if(DiagTimer_HasExpired(&S3serverTimer))
        {
            GotoSession(ECU_DEFAULT_SESSION);
            m_EcuResetType = 1;
            SystemReset(m_EcuResetType);
        }
    }

    if(m_UnlockStep == WAIT_DELAY)
    {
        uint8_t i;
        for(i = 0 ; i < MAX_SECURITY_NUM ; i ++)
        {
            if(UnlockList[i].valid == TRUE)
            {
                if(DiagTimer_HasExpired(UnlockList[i].SecurityLockTimer))
                {
                    (*UnlockList[i].FaultCounter)--;
                    Diagnostic_EEProm_Write(UnlockList[i].FaultCounterAddr, 1 ,UnlockList[i].FaultCounter);
                    m_UnlockStep = WAIT_SEED_REQ;
                }
            }
        }
    }
}

void Diagnostic_Proc(void)
{
    NetworkLayer_Proc();
    Diagnostic_MainProc();
    Diagnostic_TimeProc();
}

void Diagnostic_RxFrame(uint32_t ID,uint8_t* data,uint8_t IDE,uint8_t DLC,uint8_t RTR)
{
    NetworkLayer_RxFrame(ID,data,IDE,DLC,RTR);
}

void Diagnostic_1msTimer(void)
{
    DiagTimer_ISR_Proc();
}

void Diagnostic_LoadSecuriyFaultCounter(void)
{
    uint8_t i;
    for(i = 0 ; i < MAX_SECURITY_NUM ; i++)
    {
        if(UnlockList[i].valid != FALSE)
        {
            Diagnostic_EEProm_Read(UnlockList[i].FaultCounterAddr, 1 ,UnlockList[i].FaultCounter);
            if((*UnlockList[i].FaultCounter) == 0xFF)
            {
                (*UnlockList[i].FaultCounter )= 0;
            }
            else if((*UnlockList[i].FaultCounter) >= UnlockList[i].FaultLimitCounter)
            {
                (*UnlockList[i].FaultCounter) = 0;
                m_UnlockStep = WAIT_DELAY;
                DiagTimer_Set(UnlockList[i].SecurityLockTimer , UnlockList[i].UnlockFailedDelayTime);
            }
            Diagnostic_EEProm_Write(UnlockList[i].FaultCounterAddr, 1 ,UnlockList[i].FaultCounter);
        }
    }
}

void Diagnostic_LoadAllData(void)
{
    Diagnostic_LoadSecuriyFaultCounter();
}


void Diagnostic_10_02_Response_AfterJump(void)
{
    Service10PosResponse(2);
    GotoSession(ECU_PAOGRAM_SESSION);
    N_USData_request(DIAGNOSTIC , N_Sa ,  N_Ta , PHYSICAL , 0 , DiagnosticBuffTX , ResponseLength);
    g_udsRunState.IsCalibrationDataValid = TRUE;
    g_udsRunState.IsAppDataValid = TRUE;
}

uint8_t Diagnostic_GetCurrentSession(void)
{
    return m_CurrSessionType;
}

bool CheckProgramComplete(void)
{
    //if(TRUE == g_udsRunState.IsCalibrationDataValid && TRUE == g_udsRunState.IsAppDataValid)
    if(TRUE == g_udsRunState.IsAppDataValid)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

