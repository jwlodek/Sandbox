#ifndef TSCSD_H
#define TSCSD_H

#define TSCSD_VERSION      0
#define TSCSD_REVISION     0
#define TSCSD_MODIFICATION 0


#include <vector>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <cantProceed.h>
#include <epicsExit.h>
#include <iocsh.h>
#include <epicsExport.h>

#include <asynPortDriver.h>

static const char* driverName = "TSCSD";


#define TSCSDIDNSTR "IDN"
#define TSCSDNCHANSSTR "NCHANS"

#define CHANPOSSTR "POS"
#define CHANRRSTR "RR"
#define CHANATSPSTR "ATSP"

#define TIMEOUT 5

class TSCSD : public asynPortDriver
{
public:
    TSCSD(const char* portName, const char* deviceIPPort, int numChans);
    ~TSCSD();
    asynStatus writeReadCmd(const char* cmd, char* ret, size_t maxChars, double timeout);
protected:
    int TSCSD_idn;
    int TSCSD_nchans;
private:
    std::vector<void*> channels;
    asynUser* pasynUserDeviceIP;
# define NUM_TSCSD_PARAMS 2

};


class TSCSDChannel : public asynPortDriver
{
public:

    TSCSDChannel(TSCSD* parentDevice, const char* portName, int chanNum);
    ~TSCSDChannel();
    void monitorThread();

    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

protected:
    int CHAN_pos;
    int CHAN_rr;
    int CHAN_atsp;
private:
    bool alive = true;
    int chanNum;
    TSCSD* parent;
    epicsThreadId monitorThreadId;
#define NUM_CHAN_PARAMS 3
};

#endif;
