#!../../bin/linux-x86_64/sandboxApp

#- You may have to change default to something else
#- everywhere it appears in this file

< envPaths

## Register all support components
dbLoadDatabase("../../dbd/sandboxApp.dbd",0,0)
sandboxApp_registerRecordDeviceDriver(pdbbase) 

## Load record instances
dbLoadRecords("../../db/sandbox.db")

iocInit()

