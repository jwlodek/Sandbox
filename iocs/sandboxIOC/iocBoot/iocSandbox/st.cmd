#!../../bin/linux-x86_64/sandbox

#- You may have to change default to something else
#- everywhere it appears in this file

< /epics/common/xf31id1-ioc1-netsetup.cmd

< envPaths

epicsEnvSet("ENGINEER", "J. Wlodek")
epicsEnvSet("PORT", "SB1")
epicsEnvSet("P", "DEV:")
epicsEnvSet("R", "SB1:")
epicsEnvSet("PREFIX", "$(P)$(R)")

epicsEnvSet("STREAM_PROTOCOL_PATH", "$(SANDBOX)/sandboxApp/src")


## Register all support components
dbLoadDatabase("../../dbd/sandbox.dbd",0,0)
sandbox_registerRecordDeviceDriver(pdbbase) 

drvAsynIPPortConfigure("$(PORT)", "10.69.58.50:8888")
asynOctetSetOutputEos("$(PORT)", 0, "\n")
asynOctetSetOutputEos("$(PORT)", 0, "\n")

TSCSDConfig("DEV", "$(PORT)", 4)

# Load any additional specified databases.
dbLoadRecords("$(EPICS_BASE)/db/asynRecord.db", "PORT=$(PORT), P=$(PREFIX), R=Asyn, ADDR=0, OMAX=0, IMAX=0")

## Load record instances
#dbLoadTemplate("$(SANDBOX)/db/tscsd.substitutions")
dbLoadTemplate("$(SANDBOX)/db/tscsd-asyn.substitutions")


iocInit()

