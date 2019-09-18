  
/* Includes ------------------------------------------------------------------*/
#include "DiagnosticTimer.h"

/** @defgroup Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup Private_Defines
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup Private_Macros
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup Private_Variables
  * @{
  */ 
static uint32_t SystemTickCount = 0x781;
/**
  * @}
  */ 


/** @defgroup Private_FunctionPrototypes
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup Private_Functions
  * @{
  */ 

/**
  * @brief  Init Timer.
  * @param  None.
  * @retval None.
  */
void DiagTimer_Init(void)
{
	
}

/**
  * @brief  Sets a timer. timer must not equ 0 because 0 means timer is stop.
  * @param  STimer pointer to timer value.
  * @param  TimeLength - timer period.
  * @retval None.
  */
void DiagTimer_Set(DiagTimer *STimer, uint32_t TimeLength)
{
	STimer->TimerCounter = SystemTickCount + TimeLength;
	STimer->valid = TRUE;
	//if(STimer->TimerCounter == 0)	STimer->TimerCounter = 1; //not set timer to 0 for timer is running
}

/**
  * @brief  Stop timer.
  * @param  STimer pointer to timer value.
  * @retval None.
  */
void DiagTimer_Stop(DiagTimer *STimer)
{
	STimer->valid = FALSE;
}

/**
  * @brief  Checks whether given timer has stopped.
  * @param  STimer is timer value.
  * @retval TRUE if timer is stopped.
  */
bool DiagTimer_HasStopped(DiagTimer *STimer)
{
	return (STimer->valid == FALSE); 
}

/**
  * @brief  Checks whether given timer has expired
  *        With timer tick at 1ms maximum timer period is 10000000 ticks
  *        When *STimer is set (SoftTimer-*STimer) has a min value of 0xFFF0BDBF
  *            and will be more than this while the timer hasn't expired
  *        When the timer expires
  *                (SoftTimer-*STimer)==0
  *            and (SoftTimer-*STimer)<=7FFF FFFF for the next 60 hours
  * @param  STimer pointer to timer value.
  * @retval TRUE if timer has expired or timer is stopped, otherwise FALSE.
  */
bool DiagTimer_HasExpired(DiagTimer *STimer)
{
	if(STimer->valid == TRUE)
	{
		if(STimer->TimerCounter == 0)
		{
			STimer->valid = FALSE;
			return TRUE;
		}
		else if((SystemTickCount - STimer->TimerCounter) <= 0x7fffffff)
		{
			STimer->TimerCounter = 0;	//set timer to stop
			STimer->valid = FALSE;
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}
}

/**
  * @brief  don't wait if  given timer has expired.
  * @param  STimer pointer to timer value.
  * @retval None.
  */
void DiagTimer_WaitExpired(DiagTimer *STimer)
{
	while(1)
	{
		if(Timer_HasExpired(STimer))
			break;
		//WatchDog_Feed();
	}
}

/**
  * @brief  wait untill  given timer has expired.
  * @param  wait ms.
  * @retval None.
  */
void DiagTimer_DelayMs(uint32_t ms)
{
	uint32_t timer;
	
	DiagTimer_Set (&timer, ms);
	while (!DiagTimer_HasExpired (&timer))
	{
		//__ASM volatile ("nop");
		//WatchDog_Feed();
	}
}

void DiagTimer_DelayUs(uint32_t us)
{
	uint32_t i=0;

	for(i=0;i<us;i++)
	{
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
		__NOP();
	}
}
/**
  * @brief  get the system work time.
  * @param  None.
  * @retval None.
  */
uint32_t DiagTimer_GetTickCount(void)
{
	return (SystemTickCount);
}

/**
  * @brief  1ms interval.
  * @param  None.
  * @retval None.
  */
void DiagTimer_ISR_Proc(void)
{
	SystemTickCount++;
}

void DiagTimer_Increase(uint32_t count)
{
	SystemTickCount += count;
}

/**
  * @}
  */ 
 

