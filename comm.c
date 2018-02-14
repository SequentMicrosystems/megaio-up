/*
 * comm.c:
 *	Communication routines "platform specific" for Up-Board computer
 *	
 *	Copyright (c) 2016-2018 Sequent Microsystem
 *	<http://www.sequentmicrosystem.com>
 ***********************************************************************
 *	Author: Alexandru Burcea
 ***********************************************************************
 */
 
 
#include <stdio.h>
#include <termios.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <math.h>

#include <pthread.h>
#include <sched.h>

#include "megaio.h"

#define I2C_SLAVE	0x0703
#define I2C_SMBUS	0x0720	/* SMBus-level access */

#define I2C_SMBUS_READ	1
#define I2C_SMBUS_WRITE	0

// SMBus transaction types

#define I2C_SMBUS_QUICK		    0
#define I2C_SMBUS_BYTE		    1
#define I2C_SMBUS_BYTE_DATA	    2 
#define I2C_SMBUS_WORD_DATA	    3
#define I2C_SMBUS_PROC_CALL	    4
#define I2C_SMBUS_BLOCK_DATA	    5
#define I2C_SMBUS_I2C_BLOCK_BROKEN  6
#define I2C_SMBUS_BLOCK_PROC_CALL   7		/* SMBus 2.0 */
#define I2C_SMBUS_I2C_BLOCK_DATA    8

// SMBus messages

#define I2C_SMBUS_BLOCK_MAX	32	/* As specified in SMBus standard */	
#define I2C_SMBUS_I2C_BLOCK_MAX	32	/* Not specified but we use same structure */


static volatile int globalResponse = 0;
int gLed1HwAdd = 0x20;
int gLed2HwAdd = 0x21;

static pthread_mutex_t piMutexes [4];

union i2c_smbus_data
{
  uint8_t  byte ;
  uint16_t word ;
  uint8_t  block [I2C_SMBUS_BLOCK_MAX + 2] ;	// block [0] is used for length + one more for PEC
} ;

int upHiPri (const int pri);
int upThreadCreate (void *(*fn)(void *));


UP_THREAD (waitForKey)
{
 char resp;
 int respI = NO;

 
	struct termios info;
	tcgetattr(0, &info);          /* get current terminal attirbutes; 0 is the file descriptor for stdin */
	info.c_lflag &= ~ICANON;      /* disable canonical mode */
	info.c_cc[VMIN] = 1;          /* wait until at least one keystroke available */
	info.c_cc[VTIME] = 0;         /* no timeout */
	tcsetattr(0, TCSANOW, &info); /* set i */

	(void)upHiPri (10) ;	// Set this thread to be high priority
	resp = getchar();
	if((resp == 'y')||(resp == 'Y'))
	{
		respI = YES;
	}
	
    pthread_mutex_lock(&piMutexes[COUNT_KEY]);
	globalResponse = respI;
    pthread_mutex_unlock(&piMutexes[COUNT_KEY]);
	
	info.c_lflag |= ICANON;      /* disable canonical mode */
	info.c_cc[VMIN] = 0;          /* wait until at least one keystroke available */
	info.c_cc[VTIME] = 0;         /* no timeout */
	tcsetattr(0, TCSANOW, &info); /* set i */
	printf("\n");
	return &waitForKey;
}

/*
 * upHiPri:
 *	Attempt to set a high priority scheduling for the running program
 *********************************************************************************
 */

int upHiPri (const int pri)
{
  struct sched_param sched ;

  memset (&sched, 0, sizeof(sched)) ;

  if (pri > sched_get_priority_max (SCHED_RR))
    sched.sched_priority = sched_get_priority_max (SCHED_RR) ;
  else
    sched.sched_priority = pri ;

  return sched_setscheduler (0, SCHED_RR, &sched) ;
}

/*
 * upThreadCreate:
 *	Create and start a thread
 *********************************************************************************
 */

int upThreadCreate (void *(*fn)(void *))
{
  pthread_t myThread ;

  return pthread_create (&myThread, NULL, fn, NULL) ;
}

void startThread(void)
{
	upThreadCreate(waitForKey);
}

int checkThreadResult(void)
{
	int res;
	pthread_mutex_lock(&piMutexes[COUNT_KEY]);
	res = globalResponse;
	pthread_mutex_unlock(&piMutexes[COUNT_KEY]);
	return res;
}

/*
 * busyWait:
 *	Wait for some number of milliseconds
 *********************************************************************************
 */

void busyWait(int ms)
{
  struct timespec sleeper, dummy ;

  sleeper.tv_sec  = (time_t)(ms / 1000) ;
  sleeper.tv_nsec = (long)(ms % 1000) * 1000000 ;

  nanosleep (&sleeper, &dummy) ;
}


static inline int i2c_smbus_access (int fd, char rw, uint8_t command, int size, union i2c_smbus_data *data)
{
  struct i2c_smbus_ioctl_data args ;

  args.read_write = rw ;
  args.command    = command ;
  args.size       = size ;
  args.data       = data ;
  return ioctl (fd, I2C_SMBUS, &args) ;
}


int i2cSetup(int addr)
{
    int file;
    char filename[40];
    sprintf(filename,"/dev/i2c-1");
    
     if ((file = open(filename,O_RDWR)) < 0) 
     {
        printf("Failed to open the bus.");
        return -1;
     }
      if (ioctl(file,I2C_SLAVE,addr) < 0) {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        return -1;		
    }

     return file;
}

int readReg8(int  dev, int add)
{
   
    int ret;
    char buf[10];
    
    buf[0] = 0xff & add;
    
    if (write(dev, buf, 1) != 1)
    {
        printf("Fail to select mem add\n");
        return -1;
    }
    
    if (read(dev, buf, 1) != 1)
    {
        printf("Fail to read reg\n");
        return -1;
    }
	ret = 0xff & buf[0];
	return ret;
}


int readReg16(int dev, int add)
{
	int ret = 0;
    char buf[10];
    
    buf[0] = 0xff & add;
    
    if (write(dev, buf, 1) != 1)
    {
        printf("Fail to select mem add\n");
        return -1;
    }
    
    if (read(dev, buf, 2) != 2)
    {
        printf("Fail to read reg\n");
        return -1;
    }
	ret = 0xff & buf[1];
	ret+= 0xff00 & (buf[0] << 8);
	
	return ret;
}

int writeReg8(int dev, int add, int val)
{
    char buf[10];
    
    buf[0] = 0xff & add;
    buf[1] = 0xff & val;
    
    if (write(dev, buf, 2) < 0)
    {
        printf("Fail to w8\n");
        return -1;
    }
    return 0;

}

int writeReg16(int dev, int add, int val)
{
	char buf[10];
    
    buf[0] = 0xff & add;
    buf[2] = 0xff & val;
    buf[1] = 0xff & (val >> 8);
    
    if (write(dev, buf, 3) < 0)
    {
        printf("Fail to w16\n");
        return -1;
    } // todo: Solve
    return 0;
}

int doBoardInit(int hwAdd)
{
	int dev, bV = -1;
	dev = i2cSetup (hwAdd);
	if(dev == -1)
	{
		return -1;
	}
	bV = readReg8 (dev,REVISION_HW_MAJOR_MEM_ADD);
	if(bV == -1)
	{
		return -1;
	}
	return dev;
}

int readReg24(int dev, int add)
{
  int ret = 0;
    char buf[10];
    
    buf[0] = 0xff & add;
    
    if (write(dev, buf, 1) != 1)
    {
        printf("Fail to select mem add\n");
        return -1;
    }
    
    if (read(dev, buf, 3) != 2)
    {
        printf("Fail to read reg\n");
        return -1;
    }
	ret = 0xff & buf[2];
	ret+= 0xff00 & (buf[1] << 8);
	ret+= 0xff0000 & (buf[0] << 16);
	
#ifdef DEBUG_I	
	printbits(ret);
	printf("\n");
	printf("%#08x\n", ret);
#endif
	return ret;
}

int writeReg24(int dev, int add, int val)
{
    char buf[10];
    
    buf[0] = 0xff & add;
    buf[3] = 0xff & val;
    buf[2] = 0xff & (val >> 8);
    buf[1] = 0xff & (val >> 16);
    
    if (write(dev, buf, 4) < 0)
    {
        printf("Fail to w24\n");
        return -1;
    } // todo: Solve
    return 0;
	
}

/*
* getLedVal
* Get the value of leds 
* arg: chip 0 - 1
* ret: 0x0000 - 0xffff - success; -1 - fail 
*/
int getLedVal(int chip)
{
	int dev = -1;
	int ret = 0;
	u16 rVal = 0;
	u16 swapVal = 0;
	int i;
	
	if((chip < 0) || (chip > 1))
	{
		return -1;
	}
	dev = i2cSetup(gLed1HwAdd + chip); 
	if(dev <= 0)
	{
		return -1;
	}
	ret  = readReg16(dev, 0x02);
	if(ret < 0)
	{
		return -1;
	}
	rVal = 0xff00 & (ret << 4);
	rVal += 0x01 & (ret >> 3);
	rVal += 0x02 & (ret >> 1);
	rVal += 0x04 & (ret << 1);
	rVal += 0x08 & (ret << 3);
	rVal += 0x10 & (ret >> 11);
	rVal += 0x20 & (ret >> 9);
	rVal += 0x40 & (ret >> 7);
	rVal += 0x80 & (ret >> 5);
	
	swapVal = rVal;
	rVal = 0;
	for(i = 0; i< 8; i++)
	{
		rVal += ((u16)1 << i) & (swapVal >> (15 - 2*i));
		rVal += ((u16)0x8000 >> i) & (swapVal << (15 - 2*i));
	}
	
	return rVal;
}
	

/*
* setLedVal
* Get the value of leds 
* arg: chip 0 - 1
* arg: val 0x0000 - 0xffff 
* ret: 0 - success; -1 - fail 
*/
int setLedVal(int chip, int val)
{
	int dev = -1;
	int ret = 0;
	u16 wVal = 0;
	u16 swapVal = 0;
	int i;
	
	if((chip < 0) || (chip > 1))
	{
		return -1;
	}
	if((val < 0) || (val > 0xffff))
	{
		return -1;
	}
	
	dev = i2cSetup(gLed1HwAdd + chip); 
	if(dev <= 0)
	{
		return -1;
	}
	swapVal = 0;
	for(i = 0; i< 8; i++)
	{
		swapVal += ((u16)1 << i) & (val >> (15 - 2 * i));
		swapVal += ((u16)0x8000 >> i) & (val << (15 - 2 * i));
	}
	val = swapVal;// (0xff & (swapVal >> 8))+ (0xff00 & (swapVal << 8));
	
	wVal = 0x0ff0 & ( val >> 4);
	wVal += 0x1000 & (val << 5);
	wVal += 0x2000 & (val << 7);
	wVal += 0x4000 & (val << 9);
	wVal += 0x8000 & (val << 11);
	wVal += 0x0001 & (val >> 3);
	wVal += 0x0002 & (val >> 1);
	wVal += 0x0004 & (val << 1);
	wVal += 0x0008 & (val << 3);
	
	//wVal = 0xffff & val;
	ret = writeReg16(dev, 0x06, 0x0000); // set all to output
	if(ret > -1)
	{
		ret = writeReg16(dev, 0x02, wVal);
	}
	
	return ret;
}	