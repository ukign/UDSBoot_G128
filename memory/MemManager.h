

#ifndef MEM_MANAGER_H
#define MEM_MANAGER_H

#include "PE_Types.h"
#include "NVM_RAM_REMAP.h"
#include "SSD_Types.h"
#include "SSD_SGF18.h"
#include "SSD_SGF18_Internal.h"

#define RAM_SPACE_INDEX 0
#define EEPROM_SPACE_INDEX 1
#define FLASH_SPACE_INDEX 2

#define MEMORY_START_INDEX  0
#define MEMORY_END_INDEX 1

#define MEM_BUFFER_SIZE 1032

#define MEM_FINISH  0
#define MEM_WAIT    1
#define MEM_ERROR   2
#define MEM_BUSY     3
#define MEM_OVERFLOW 4
#define MEM_LOW_VOL  5
#define MEM_HIGH_VOL 6

/* Base address of MCU register block */
#define MCU_REGISTER_BASE               0x00000000

/* Bus clock. The unit is 10KHz */
#define BUS_CLOCK                       2400  /*24MHz bus clock*/

/* Ignore Single Fault Flag */
#define IGNORE_SINGLE_FAULT             FALSE

#define BDMENABLE                       FALSE      /* Background debug mode enabled or not.
                                                      Disable debug mode in this demo */
#define TARGET_ADDRESS_OFFSET           0x00000000

//#define FLASH_SECTOR_SIZE               0x200 /* 512 B */

/* Since S12G has only one P-FLASH block, FlashCommandSequence()(c-array)
has to be explicitly put on to the RAM to avoid RWW(read while write) error
Size of the c-array for FlashCommandSequence() can be obtained from C-array file  */
#define FLASH_COMMANDSEQUENCE_SIZE      143

/* User can specify this address based on availability of RAM space for his application */
#define RAM_ADDR_FCS                    0x3F60


/* Prototype of error trap funciton */
//extern void ErrorTrap(UINT16 returnCode);

#define CONV_FAR_DATA_TO_FUN_PTR(to, from)\
  *(UINT16*)&to = *(UINT16*)((unsigned char*)&from+1);\
  *((unsigned char*)&to+2) = *(unsigned char*)&from;

#define NVM_Driver_Fun_Num  23

#define RAM_START_ADDR            0x2000 //8KB
#define RAM_END_ADDR                0x4000

typedef struct{
    uint16_t length;
    uint16_t programed;
    uint8_t data[MEM_BUFFER_SIZE];    
    uint8_t need;
    uint8_t state;
    uint8_t type;
}MemProgram;

typedef struct{
    uint32_t address;
    uint32_t length;
    uint32_t erased;
    uint8_t need;
    uint8_t state;
    uint8_t type;
}MemErase;

typedef struct{
    uint32_t address;
    uint32_t length;
    uint32_t complete;
}MemRequest;

uint8_t MemManger_Program(uint16_t len,uint8_t* data);
uint8_t MemManger_Erase(uint32_t addr, uint32_t len);
uint8_t MemManger_Request(uint32_t addr, uint32_t len);
bool MemManger_CkeckIntegrity(uint32_t checkSum);
bool MemManger_CkeckDependency(void);
bool MemManger_ValidAddress(uint32_t address , uint32_t length);
bool MemManger_ValidRange(uint8_t index , uint32_t address , uint32_t length);
uint8_t MemManger_GetEraseStatus(void);
uint8_t MemManger_GetProgramStatus(void);
void MemManger_CleanCodeRam(void);

void MemManger_Init(void);
void MemManger_Task(void);


void Power_BattResolve(uint8_t voltageNumber);

#endif


