
#include <AP_HAL/AP_HAL.h>
#if CONFIG_HAL_BOARD == HAL_BOARD_REVOMINI

#include <AP_HAL_REVOMINI/AP_HAL_REVOMINI.h>
#include "AP_HAL_REVOMINI_Namespace.h"
#include "HAL_REVOMINI_Class.h"
#include "AP_HAL_REVOMINI_Private.h"
#include "Util.h"
#include <AP_HAL_Empty/AP_HAL_Empty.h>
#include <AP_HAL_Empty/AP_HAL_Empty_Private.h>
#include <pwm_in.h>
#include <usart.h>
#include <i2c.h>

using namespace AP_HAL;
using namespace REVOMINI;



// XXX make sure these are assigned correctly
static REVOMINIUARTDriver uartADriver(_USART1,1);
static REVOMINIUARTDriver uartBDriver(_USART6,0);
//static REVOMINIUARTDriver uartCDriver(_USART3,0);

static REVOMINISemaphore  i2cSemaphore;
static REVOMINISemaphore  i2c2Semaphore;
static REVOMINII2CDriver  i2cDriver(_I2C1,&i2cSemaphore);
static REVOMINII2CDriver  i2c2Driver(_I2C1,&i2c2Semaphore);
static REVOMINISPIDeviceManager spiDeviceManager;
static REVOMINIAnalogIn analogIn;
static REVOMINIStorage storageDriver;
static REVOMINIGPIO gpioDriver;
static REVOMINIRCInput rcinDriver;
static REVOMINIRCOutput rcoutDriver;
static REVOMINIScheduler schedulerInstance;
static REVOMINIUtil utilInstance;



HAL_REVOMINI::HAL_REVOMINI() :
    AP_HAL::HAL(
        &uartADriver,  /* uartA */
        &uartBDriver,  /* uartB */
        NULL,  /* no uartC */
        NULL,  /* no uartD */
        NULL,  /* no uartE */
        &i2cDriver, /* i2c */
        &i2c2Driver,   /* i2c 2 */
        NULL,   /* only 1 i2c */
        &spiDeviceManager, /* spi */
        &analogIn, /* analogin */
        &storageDriver, /* storage */
        &uartADriver, /* console */
        &gpioDriver, /* gpio */
        &rcinDriver,  /* rcinput */
        &rcoutDriver, /* rcoutput */
        &schedulerInstance, /* scheduler */
        &utilInstance) /* util */

/* 

*/

{}


/*
AP_HAL::HAL(
*/

void HAL_REVOMINI::init(int argc,char* const argv[]) const
{
  /* initialize all drivers and private members here.
   * up to the programmer to do this in the correct order.
   * Scheduler should likely come first. */

  scheduler->init(NULL);

  uartA->begin(57600);
  uartB->begin(57600);

  i2c->begin();
  i2c2->begin();
  spi->init(NULL);
  analogin->init(NULL);
  storage->init(NULL);
  rcin->init(NULL);
  rcout->init((void *)&_is_ppmsum);
}

const HAL_REVOMINI AP_HAL_REVOMINI;

#endif
