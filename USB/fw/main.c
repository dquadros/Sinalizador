/*
 *  Sinalizador USB HID
 *  Version 1.0
 *  Copyright (C) 2016, Daniel Quadros
 *	dqsoft.blogspot.com
 *
 *  (based on the sources of Simplibox IO by Hartmut Wendt - www.hwhardsoft.de)
 *  (based on the sources of USBLotIO by hobbyelektronik.org)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */
#include <avr/eeprom.h>

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "oddebug.h"        /* This is also an example for using debug macros */

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

#define SREG_OUT  2
#define SREG_IN   2
#define DEVICE_ID 1

#define nop()	asm("nop")

#define REPORT_COUNT (((SREG_OUT > SREG_IN) ? SREG_OUT : SREG_IN)+1)

const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {    /* USB report descriptor */
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, REPORT_COUNT,            //   REPORT_COUNT
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
    0xc0                           // END_COLLECTION
};

uint8_t sregreport[REPORT_COUNT];
uint8_t sregoutstate[SREG_OUT];
uint8_t sregpos;

void processCommand() {
	

	if (sregreport[0] == DEVICE_ID) {
        PORTA = (PORTA & 0xC3) | (sregreport[2] & 0x3C);
    }

}

/* ------------------------------------------------------------------------- */

/* usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar   usbFunctionRead(uchar *data, uchar len)
{
    uint8_t x;
	for(x = 0; x < len; x++) {
		*data = sregreport[sregpos];
		data++;
		sregpos++;
	}

    return len;
}

/* usbFunctionWrite() is called when the host sends a chunk of data to the
 * device. For more information see the documentation in usbdrv/usbdrv.h.
 */
uchar   usbFunctionWrite(uchar *data, uchar len) {
	while(len--) {
		sregreport[sregpos] = *data;
		data++;
		sregpos++;
		if(sregpos >= REPORT_COUNT) {
			processCommand();
			return 1;
		}
	}
    
    return 0;
}

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* HID class request */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* since we have only one report type, we can ignore the report-ID */
			sregpos = 0;


			sregreport[0] = DEVICE_ID;
			sregreport[1] = PINA & 0x80;   // PA7, pino 6
			sregreport[2] = 0;

            return USB_NO_MSG;  /* use usbFunctionRead() to obtain data */
        } else if(rq->bRequest == USBRQ_HID_SET_REPORT) {
			sregpos = 0;
            return USB_NO_MSG;  /* use usbFunctionWrite() to receive data from host */
        }
	} else if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR){
		if(rq->bRequest == 1){ //CUSTOM_RQ_SET_STATUS
	        //SPI_Put(rq->wValue.bytes[0]);
			//SPI_ENA_Oble();
		}
    }else {
        /* ignore vendor type requests, we don't use any */
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/* ----------------------------- Main functions ---------------------------- */
/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */

int main(void) {
uchar   i;
uchar   calibrationValue;


    calibrationValue = eeprom_read_byte(0); // calibration value from last time 
    if(calibrationValue != 0xff){
        OSCCAL = calibrationValue;
    } 
    

    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
	 DDRA = 0x3C;

	//Initialize the USB Connection with the host computer. 
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    for(i=0;i<20;i++){  /* 300 ms disconnect */
        _delay_ms(15);
    }
    usbDeviceConnect(); 

	wdt_enable(WDTO_1S);
    usbInit();
    sei();


    for(;;){                /* main event loop */
        wdt_reset();
        usbPoll();
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------ Oscillator Calibration ------------------------- */
/* ------------------------------------------------------------------------- */

static void calibrateOscillator(void)
{
uchar       step = 128;
uchar       trialValue = 0, optimumValue;
int         x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);
 
    /* do a binary search: */
    do{
        OSCCAL = trialValue + step;
        x = usbMeasureFrameLength();    // proportional to current real frequency
        if(x < targetValue)             // frequency still too low
            trialValue += step;
        step >>= 1;
    }while(step > 0);
    /* We have a precision of +/- 1 for optimum OSCCAL here */
    /* now do a neighborhood search for optimum value */
    optimumValue = trialValue;
    optimumDev = x; // this is certainly far away from optimum
    for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++){
        x = usbMeasureFrameLength() - targetValue;
        if(x < 0)
            x = -x;
        if(x < optimumDev){
            optimumDev = x;
            optimumValue = OSCCAL;
        }
    }
    OSCCAL = optimumValue;
}
 
void    usbEventResetReady(void)
{
    cli();  // usbMeasureFrameLength() counts CPU cycles, so disable interrupts.
    calibrateOscillator();
    sei();
    eeprom_write_byte(0, OSCCAL);   // store the calibrated value in EEPROM
}
