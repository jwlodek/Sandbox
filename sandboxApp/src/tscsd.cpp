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

#include <asynOctetSyncIO.h>

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
extern "C" int TSCSDConfig(const char* portName, const char* deviceIPPort, int numChannels) {
    new TSCSD(portName, deviceIPPort, numChannels);
    return (asynSuccess);
}

static void exitCallbackC(void* pPvt) {
    TSCSD* pTSCSD = (TSCSD*)pPvt;
    delete pTSCSD;
}

asynStatus TSCSD::writeReadCmd(const char* cmd, char* ret, size_t maxChars, double timeout){

    size_t nwrite, nread;
    asynStatus status;
    int eomReason;
    const char *functionName="writeReadCmd";
    pasynOctetSyncIO->flush(this->pasynUserDeviceIP);
    status = pasynOctetSyncIO->writeRead(this->pasynUserDeviceIP, cmd,
                                      strlen(cmd), ret, maxChars, timeout,
                                      &nwrite, &nread, &eomReason);
    return status;
}


TSCSD::TSCSD(const char* portName, const char* deviceIPPort, int numChannels)
    : asynPortDriver(
          portName, 1, /* maxAddr */
          (int)NUM_TSCSD_PARAMS,
          asynInt32Mask | asynFloat64Mask | asynFloat64ArrayMask | asynDrvUserMask |
              asynOctetMask, /* Interface mask */
          asynInt32Mask | asynFloat64Mask | asynFloat64ArrayMask |
              asynOctetMask, /* Interrupt mask */
          0, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
          1, /* Autoconnect */
          0, /* Default priority */
          0) /* Default stack size*/

{
    const char* functionName = "TSCSD";
    createParam(TSCSDIDNSTR, asynParamOctet, &TSCSD_idn);
    createParam(TSCSDNCHANSSTR, asynParamInt32, &TSCSD_nchans);

    pasynOctetSyncIO->connect(deviceIPPort, 0, &this->pasynUserDeviceIP, NULL);

    char idn[48];
    this->writeReadCmd("*IDN?", idn, 48, TIMEOUT);
    setStringParam(TSCSD_idn, idn);

    char nchans[16];
    this->writeReadCmd("NCHAN?", nchans, 16, TIMEOUT);
    setIntegerParam(TSCSD_nchans, atoi(nchans));

    for(int i = 0; i < numChannels; i++) {
        char chanPortStr[32];
        snprintf(chanPortStr, 32, "%s_CHAN%d", portName, i + 1);
        TSCSDChannel* channel = new TSCSDChannel(this, chanPortStr, i + 1);
        channels.push_back(channel);
    }

    epicsAtExit(exitCallbackC, (void*) this);

}

TSCSD::~TSCSD(){
    const char* functionName = "~TSCSD";
    LOG("Shutting down simple device IOC...");
    for(int i = 0; i< this->channels.size(); i++){
        delete (TSCSDChannel*) this->channels[i];
    }
    LOG("Done");
}


void chanMonitorThreadC(void* chanPtr) {
    TSCSDChannel* pChan = (TSCSDChannel*) chanPtr;
    pChan->monitorThread();
}

void TSCSDChannel::monitorThread(){
    while(this->alive) {

        char rampRate[48], positionReadback[48], isAtRest[16];
        char rampRateCmd[16], positionReadbackCmd[16], isAtRestCmd[16];

        snprintf(rampRateCmd, 16, "RR? %d", this->chanNum);
        snprintf(positionReadbackCmd, 16, "READ? %d", this->chanNum);
        snprintf(isAtRestCmd, 16, "ATSP? %d", this->chanNum);

        // Send command to low level port here
        this->parent->writeReadCmd(rampRateCmd, rampRate, 48, TIMEOUT);
        this->parent->writeReadCmd(positionReadbackCmd, positionReadback, 48, TIMEOUT);
        this->parent->writeReadCmd(isAtRestCmd, isAtRest, 48, TIMEOUT);

        // Ramp rate command returns RR followed by CHAN# followed by = follwed by RR
        setDoubleParam(CHAN_rr, atof(strtok(rampRate, "=")));

        setDoubleParam(CHAN_pos, atof(positionReadback));
        setIntegerParam(CHAN_atsp, atoi(isAtRest));

        epicsThreadSleep(0.1);
    }
}


asynStatus TSCSDChannel::writeFloat64(asynUser* pasynUser, epicsFloat64 value){
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static const char* functionName = "writeFloat64";
    if (function == CHAN_pos){
        char positionSetCmd[48], positionRB[48];
        snprintf(positionSetCmd, 48, "SP %d %f", this->chanNum, value);
        status = this->parent->writeReadCmd(positionSetCmd, positionRB, 48, TIMEOUT);
    } else if (function == CHAN_rr) {
        char positionSetCmd[48], positionRB[48];
        snprintf(positionSetCmd, 48, "SP %d %f", this->chanNum, value);
        status = this->parent->writeReadCmd(positionSetCmd, positionRB, 48, TIMEOUT);
    }
    
    if (status) {
        ERR_ARGS("ERROR status=%d, function=%d, value=%f, msg=%s", status, function, value);
    } else {
        status = setDoubleParam(function, value);
        LOG_ARGS("function=%d value=%f", function, value);
    }

    callParamCallbacks();
    return status;
}

TSCSDChannel::TSCSDChannel(TSCSD* parentDevice, const char* portName, int chanNum) 
    : asynPortDriver(
          portName, 1, /* maxAddr */
          (int)NUM_CHAN_PARAMS,
          asynInt32Mask | asynFloat64Mask | asynFloat64ArrayMask | asynDrvUserMask |
              asynOctetMask, /* Interface mask */
          asynInt32Mask | asynFloat64Mask | asynFloat64ArrayMask |
              asynOctetMask, /* Interrupt mask */
          0, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
          1, /* Autoconnect */
          0, /* Default priority */
          0) /* Default stack size*/
{
    const char* functionName = "TSCSDChannel";
    createParam(CHANPOSSTR, asynParamFloat64, &CHAN_pos);
    createParam(CHANRRSTR, asynParamFloat64, &CHAN_rr);
    createParam(CHANATSPSTR, asynParamInt32, &CHAN_atsp);
    this->parent = parentDevice;
    this->chanNum = chanNum;

    LOG_ARGS("Initializing channel %d", chanNum);

    // Create our thread options struct
    epicsThreadOpts opts;
    opts.priority = epicsThreadPriorityMedium;
    opts.stackSize = epicsThreadGetStackSize(epicsThreadStackMedium);
    opts.joinable = 1;

    char chanThreadName[48];
    snprintf(chanThreadName, 48, "CHAN%d_MON_THREAD", chanNum);

    epicsThreadCreateOpt(chanThreadName, (EPICSTHREADFUNC) chanMonitorThreadC, this, &opts);
}


TSCSDChannel::~TSCSDChannel(){
    const char* functionName = "~TSCSDChannel";
    this->alive = false;
    epicsThreadMustJoin(this->monitorThreadId);
    LOG_ARGS("Shut down channel %d", this->chanNum);
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
