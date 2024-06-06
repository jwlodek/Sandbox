// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>

// EPICS includes
#include <epicsExit.h>
#include <epicsExport.h>
#include <epicsStdio.h>
#include <epicsString.h>
#include <epicsThread.h>
#include <epicsTime.h>
#include <iocsh.h>


#include "tscsd.h"

// Error message formatters
#define ERR(msg)                                                                                 \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "ERROR | %s::%s: %s\n", driverName, functionName, \
              msg)

#define ERR_ARGS(fmt, ...)                                                              \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "ERROR | %s::%s: " fmt "\n", driverName, \
              functionName, __VA_ARGS__);

// Warning message formatters
#define WARN(msg) \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "WARN | %s::%s: %s\n", driverName, functionName, msg)

#define WARN_ARGS(fmt, ...)                                                            \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "WARN | %s::%s: " fmt "\n", driverName, \
              functionName, __VA_ARGS__);

// Log message formatters
#define LOG(msg) \
    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s: %s\n", driverName, functionName, msg)

#define LOG_ARGS(fmt, ...)                                                                       \
    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s: " fmt "\n", driverName, functionName, \
              __VA_ARGS__);


/**
 * @brief External configuration function for TSCSD.
 *
 * Envokes the constructor to create a new TSCSD object
 * This is the function that initializes the driver, and is called in the IOC startup script
 *
 */
extern "C" int TSCSDConfig(const char* portName, const char* devicePortName, int numChannels) {
    new TSCSD(portName, devicePortName, numChannels);
    return (asynSuccess);
}

static void exitCallbackC(void* pPvt) {
    TSCSD* pTSCSD = (TSCSD*)pPvt;
    delete pTSCSD;
}


TSCSD::TSCSD(const char* portName, const char* devicePortName, int numChannels) {
    const char* functionName = "TSCSD";
    createParam(TSCSDIDNSTR, asynParamOctet, &TSCSD_idn);
    createParam(TSCSDNCHANSSTR, asynParamInt32, &TSCSD_nchans);

    channels.

    epicsAtExit(exitCallbackC, (void*) this);

}

void chanMonitorThreadC(void* chanPtr) {
    TSCSDChannel* pChan = (TSCSDChannel*) chanPtr;
    pChan->monitorThread();
}

void TSCSDChannel::monitorThread(){
    while(alive) {

        // Send command to low level port here
        epicsThreadSleep(1);
    }
}

TSCSDChannel::TSCSDChannel(TSCSD parentDevice, const char* portName, const char* deviceIPPort, int chanNum) {
    const char* functionName = "TSCSDChannel";
    createParam(CHANPOSSTR, asynParamFloat64, &CHAN_pos);
    createParam(CHANRRSTR, asynParamFloat64, &CHAN_rr);
    createParam(CHANATSPSTR, asynParamInt32, &CHAN_atsp);

    epicsThreadCreateOpt()
}



//-------------------------------------------------------------
// TSCSD ioc shell registration
//-------------------------------------------------------------

/* TSCSDConfig -> These are the args passed to the constructor in the epics config function */
static const iocshArg TSCSDConfigArg0 = {"portName", iocshArgString};

// This parameter must be customized by the driver author. Generally a URL, Serial Number, ID, IP
// are used to connect.
static const iocshArg TSCSDConfigArg1 = {"deviceIPPortName", iocshArgString};

// This parameter must be customized by the driver author. Generally a URL, Serial Number, ID, IP
// are used to connect.
static const iocshArg TSCSDConfigArg2 = {"numChannels", iocshArgInt};

/* Array of config args */
static const iocshArg* const TSCSDConfigArgs[] = {&TSCSDConfigArg0, &TSCSDConfigArg1,
                                                  &TSCSDConfigArg2};

/* what function to call at config */
static void configTSCSDCallFunc(const iocshArgBuf* args) {
    TSCSDConfig(args[0].sval, args[1].sval, args[2].ival);
}

/* information about the configuration function */
static const iocshFuncDef configTSCSD = {"TSCSDConfig", 3, TSCSDConfigArgs};

/* IOC register function */
static void TSCSDRegister(void) { iocshRegister(&configTSCSD, configTSCSDCallFunc); }

/* external function for IOC register */
extern "C" {
epicsExportRegistrar(TSCSDRegister);
}
