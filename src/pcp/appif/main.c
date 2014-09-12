/**
********************************************************************************
\file       main.c

\brief      Main module of AppIf interface

The main module of the application interface provides the gateway from the
openPOWERLINK stack to the application interface. It forwards data to the
buffers which can be accessed by the application processor.

\ingroup module_main
*******************************************************************************/

/*------------------------------------------------------------------------------
* License Agreement
*
* Copyright 2013 BERNECKER + RAINER, AUSTRIA, 5142 EGGELSBERG, B&R STRASSE 1
* All rights reserved.
*
* Redistribution and use in source and binary forms,
* with or without modification,
* are permitted provided that the following conditions are met:
*
*   * Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer
*     in the documentation and/or other materials provided with the
*     distribution.
*   * Neither the name of the B&R nor the names of its contributors
*     may be used to endorse or promote products derived from this software
*     without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
------------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------

#include <appif/appif.h>
#include <appif/status.h>
#include <appif/tpdo.h>

#include <oplk/oplk.h>

/* POWERLINK settings demo specific values */
#include <powerlink.h>

/* POWERLINK settings default values */
#include <powerlinkdefault.h>

#include <event.h>


//------------------------------------------------------------------------------
// defines
//------------------------------------------------------------------------------
#define NODEID          0x01                   ///< Initial NodeId. Should be NOT 0xF0 (=MN) in case of CN
#define CYCLE_LEN       1000                   ///< length of the cycle [us]

#define MAC_ADDR_LAST_BYTE          5          ///< Position of the last MAC address byte

//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// module global vars
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// global function prototypes
//------------------------------------------------------------------------------


//============================================================================//
//            P R I V A T E   D E F I N I T I O N S                           //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------

/**
 * \brief Main module instance structure
 */
typedef struct
{
    UINT8         aMacAddr[6];       ///< MAC address
    INT8          sHostName[33];     ///< The hostname of the node
    UINT8         nodeId;            ///< Node ID
    UINT32        subnetMask;        ///< The subnet mask of the node
    UINT32        defGateway;        ///< The default gateway of the node

    UINT8         fShutdown;         ///< Initialize CN shutdown sequence
    UINT32        cycleTime;         ///< Cycle time of the CN (set in kEplNmtGsResetConfiguration)
    tNmtState     plkState;          ///< The current state of the CN
} tMainInstance;

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------

static tMainInstance mainInstance_l;

//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static tAppIfStatus appif_initPlk(tMainInstance* pInstance_p);
static tAppIfStatus appif_processPlk(tMainInstance* pInstance_p);
static void appif_switchOffPlk(void);
static void appif_enterCriticalSection(UINT8 fEnable_p);
static tOplkError appif_userEventCb(tOplkApiEventType eventType_p,
                                    tOplkApiEventArg* pEventArg_p,
                                    void* pUserArg_p);

static tOplkError appif_syncCb(tSocTimeStamp* socTimeStamp_p) SECTION_MAIN_APP_CB_SYNC;

//------------------------------------------------------------------------------
// private functions
//------------------------------------------------------------------------------


//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

//------------------------------------------------------------------------------
/**
\brief    Entry point of the program

Initializes the application interface and the openPOWERLINK stack.

\return int
\retval 0          Successful termination
\retval Other      On error

\ingroup module_main
*/
//------------------------------------------------------------------------------
int main (void)
{
    UINT8    nodeId;
    tAppIfStatus ret = kAppIfSuccessful;
    const UINT8 aMacAddr[] = {MAC_ADDR};

    // reset local instance of main
    APPIF_MEMSET(&mainInstance_l, 0 , sizeof(tMainInstance));

    //Set state of stack to invalid value
    mainInstance_l.plkState = 0xFFFF;

    PRINTF("----------------------------------------------------------\n");
    PRINTF("Starting openPOWERLINK with external application interface\n");
    PRINTF("----------------------------------------------------------\n");

    // Read node id from switches
    nodeId = target_getNodeid();
    if(nodeId == 0)
    {
        nodeId = NODEID;
    }

    PRINTF("Initialize application interface...\n");
    ret = appif_init(nodeId, appif_enterCriticalSection);
    if(ret != kAppIfSuccessful)
    {
        goto Exit;
    }

    initEvents(&appif_userEventCb);

    // Set POWERLINK stack initialization parameters
    mainInstance_l.cycleTime    = CYCLE_LEN;
    mainInstance_l.subnetMask   = SUBNET_MASK;
    mainInstance_l.defGateway   = DEF_GATEWAY;
    mainInstance_l.nodeId       = (nodeId != 0) ? nodeId : NODEID;
    mainInstance_l.fShutdown    = FALSE;

    // set mac address (last byte is set to node ID)
    OPLK_MEMCPY(mainInstance_l.aMacAddr, aMacAddr, sizeof(aMacAddr));
    mainInstance_l.aMacAddr[MAC_ADDR_LAST_BYTE] = mainInstance_l.nodeId;

    // setup the hostname
    sprintf(mainInstance_l.sHostName, "%02x-%08x", (UINT32)NODEID, (UINT32)CONFIG_IDENT_VENDOR_ID);

    // Initialize the POWERLINK stack
    ret = appif_initPlk(&mainInstance_l);
    if(ret != kAppIfSuccessful)
    {
        goto Exit;
    }

    PRINTF("Configure application interface...\n");
    // Configure the application interface modules
    ret = appif_configureModules();
    if(ret != kAppIfSuccessful)
    {
        goto ExitShutdown;
    }

    // Process POWERLINK background task
    ret = appif_processPlk(&mainInstance_l);
    if(ret != kAppIfSuccessful)
    {
        goto ExitShutdown;
    }

ExitShutdown:
    // Shutdown POWERLINK stack
    oplk_shutdown();

Exit:
    appif_exit();

    return 0;
}

//============================================================================//
//            P R I V A T E   F U N C T I O N S                               //
//============================================================================//
/// \name Private Functions
/// \{


//------------------------------------------------------------------------------
/**
\brief  Initialize the openPOWERLINK stack

The function initializes the openPOWERLINK stack.

\param  pInstance_p             Pointer to demo instance

\return tAppIfStatus
\retval
*/
//------------------------------------------------------------------------------
static tAppIfStatus appif_initPlk(tMainInstance* pInstance_p)
{
    tAppIfStatus             ret = kAppIfSuccessful;
    tOplkError               oplkret = kErrorOk;
    static tOplkApiInitParam initParam;

    PRINTF("Initializing openPOWERLINK stack...\n");

    OPLK_MEMSET(&initParam, 0, sizeof(initParam));

    initParam.sizeOfInitParam             = sizeof(initParam);
    initParam.nodeId                      = pInstance_p->nodeId;
    initParam.ipAddress               = (0xFFFFFF00 & IP_ADDR) | initParam.nodeId;

    OPLK_MEMCPY(initParam.aMacAddress, pInstance_p->aMacAddr,
            sizeof(initParam.aMacAddress));

    initParam.fAsyncOnly                  = FALSE;
    initParam.featureFlags                = -1;
    initParam.cycleLen                    = pInstance_p->cycleTime;        // required for error detection
    initParam.isochrTxMaxPayload          = CONFIG_ISOCHR_TX_MAX_PAYLOAD;  // const
    initParam.isochrRxMaxPayload          = CONFIG_ISOCHR_RX_MAX_PAYLOAD;  // const
    initParam.presMaxLatency              = 2000;                          // const; only required for IdentRes
    initParam.asndMaxLatency              = 2000;                          // const; only required for IdentRes
    initParam.preqActPayloadLimit         = 36;                            // required for initialization (+28 bytes)
    initParam.presActPayloadLimit         = 40;                            // required for initialization of Pres frame (+28 bytes)
    initParam.multiplCylceCnt             = 0;                             // required for error detection
    initParam.asyncMtu                    = 300;                           // required to set up max frame size
    initParam.prescaler                   = 2;                             // required for sync
    initParam.lossOfFrameTolerance        = 5000000;
    initParam.asyncSlotTimeout            = 3000000;
    initParam.waitSocPreq                 = 0;
    initParam.syncNodeId                  = C_ADR_SYNC_ON_SOC;
    initParam.fSyncOnPrcNode              = FALSE;
    initParam.deviceType                  = CONFIG_IDENT_DEVICE_TYPE;      // NMT_DeviceType_U32
    initParam.vendorId                    = CONFIG_IDENT_VENDOR_ID;        // NMT_IdentityObject_REC.VendorId_U32
    initParam.productCode                 = CONFIG_IDENT_PRODUCT_CODE;     // NMT_IdentityObject_REC.ProductCode_U32
    initParam.revisionNumber              = CONFIG_IDENT_REVISION;         // NMT_IdentityObject_REC.RevisionNo_U32
    initParam.serialNumber                = CONFIG_IDENT_SERIAL_NUMBER;    // NMT_IdentityObject_REC.SerialNo_U32
    initParam.applicationSwDate           = 0;
    initParam.applicationSwTime           = 0;
    initParam.subnetMask                  = pInstance_p->subnetMask;
    initParam.defaultGateway              = pInstance_p->defGateway;

    strncpy((char *)initParam.sHostname, (char *)pInstance_p->sHostName,
            sizeof(initParam.sHostname));

    // set callback functions
    initParam.pfnCbEvent                  = processEvents;
    initParam.pfnCbSync                   = appif_syncCb;

    // initialize POWERLINK stack
    oplkret = oplk_init(&initParam);
    if(oplkret != kErrorOk)
    {
        PRINTF("oplk_init() failed (Error:0x%x!\n", ret);
        ret = kAppIfMainPlkStackInitError;
    }

    return kErrorOk;
}


//------------------------------------------------------------------------------
/**
\brief    Process the POWERLINK stack

Carry out kNmtEventSwReset and periodically process the POWERLINK background
task.

\param[in] pInstance_p       Module instance

\return tAppIfStatus
\retval kAppIfSuccessful                  Successful exit
\retval kAppIfMainPlkStackStartError      Unable to start the stack
\retval kAppIfMainPlkStackProcessError    Unable to process the stack
\retval other                             On other error

\ingroup module_main
*/
//------------------------------------------------------------------------------
static tAppIfStatus appif_processPlk(tMainInstance* pInstance_p)
{
    tOplkError oplkret = kErrorOk;
    tAppIfStatus ret = kAppIfSuccessful;

    // start processing
    oplkret = oplk_execNmtCommand(kNmtEventSwReset);
    if(oplkret != kErrorOk)
    {
        ret = kAppIfMainPlkStackStartError;
        goto Exit;
    }

    while(1)
    {
        // do background tasks
        oplkret = oplk_process();
        if(oplkret != kErrorOk)
        {
            ret = kAppIfMainPlkStackProcessError;
            break;
        }

        // Handle background task of application interface
        ret = appif_handleAsync();
        if(ret != kAppIfSuccessful)
            break;

        // trigger switch off
        if(pInstance_p->fShutdown != FALSE)
        {
            appif_switchOffPlk();

            // reset shutdown flag to generate only one switch off command
            pInstance_p->fShutdown = FALSE;
        }

        // exit loop if NMT is in off state
        if(pInstance_p->plkState == kNmtGsOff)
            break;
    }

Exit:
    return ret;
}

/**
 ********************************************************************************
\brief
 *******************************************************************************/

//------------------------------------------------------------------------------
/**
\brief    Switch off the openPOWERLINK stack

\ingroup module_main
*/
//------------------------------------------------------------------------------
static void appif_switchOffPlk(void)
{
    tOplkError ret;

    ret = oplk_execNmtCommand(kNmtEventSwitchOff);
    if(ret != kErrorOk)
    {
        DEBUG_TRACE(DEBUG_LVL_ERROR, "ERROR: Unable to carry out kNmtEventSwitchOff!\n");
    }
}

//------------------------------------------------------------------------------
/**
\brief    Enter the critical section

\param[in] fEnable_p       TRUE = Interrupts disabled
                           FALSE = Interrupts enabled

\ingroup module_main
*/
//------------------------------------------------------------------------------
static void appif_enterCriticalSection(UINT8 fEnable_p)
{
    target_criticalSection(fEnable_p);
}


//------------------------------------------------------------------------------
/**
\brief  openPOWERLINK event callback

The function implements the applications stack event handler.

\param  eventType_p         Type of event
\param  pEventArg_p         Pointer to union which describes the event in detail
\param  pUserArg_p          User specific argument

\return The function returns a tOplkError error code.

\ingroup module_main
*/
//------------------------------------------------------------------------------
static tOplkError appif_userEventCb(tOplkApiEventType eventType_p,
                                    tOplkApiEventArg* pEventArg_p,
                                    void* pUserArg_p)
{
    tOplkError  oplkret = kErrorOk;
    tAppIfStatus ret = kAppIfSuccessful;

    UNUSED_PARAMETER(pUserArg_p);

    switch(eventType_p)
    {
        case kOplkApiEventNmtStateChange:
        {
            // Make POWERLINK stack state global
            mainInstance_l.plkState = pEventArg_p->nmtStateChange.newNmtState;

            switch(pEventArg_p->nmtStateChange.newNmtState)
            {
                case kNmtGsOff:
                {
                    // NMT state machine was shut down
                    oplkret = kErrorShutdown;

                    break;
                }
                case kNmtGsResetConfiguration:
                {
                    UINT size = sizeof(mainInstance_l.cycleTime);;

                    oplkret = oplk_readLocalObject(0x1006, 0, &mainInstance_l.cycleTime, &size);
                    if (oplkret != kErrorOk)
                    {
                        mainInstance_l.cycleTime = 0;
                    }
                    else
                    {
                        // Forward current cycle to status module
                        status_setCycleTime(mainInstance_l.cycleTime);
                    }

                    break;
                }
                case kNmtCsPreOperational1:
                {
                    // Reset the relative time state machine
                    ret = status_resetRelTime();
                    if(ret != kAppIfSuccessful)
                    {
                        oplkret = kErrorNoResource;
                    }
                    break;
                }
                case kNmtGsResetCommunication:
                {
                    UINT8   nodeId = 0xF0;
                    UINT32  nodeAssignment = NMT_NODEASSIGN_NODE_EXISTS;

                    oplkret = oplk_writeLocalObject(0x1F81, nodeId, &nodeAssignment, sizeof(nodeAssignment));

                    break;
                }
                case kNmtCsOperational:
                {
                    status_enableSyncInt();

                    break;
                }
                default:
                    break;
            }
            break;
        }
        case kOplkApiEventSdo:
        {
            // Sdo access finished -> tell appif interface
            ret = appif_sdoAccFinished(&pEventArg_p->sdoInfo);
            if(ret != kAppIfSuccessful)
            {
                oplkret = kErrorApiInvalidParam;
            }
            break;
        }
        default:
            break;
    }

    return oplkret;
}

//------------------------------------------------------------------------------
/**
\brief    Sync event callback function

appif_syncCb() implements the event callback function called by event module
within kernel part (high priority). This function sets the outputs, reads the
inputs and runs the control loop.

\param  socTimeStamp_p          Timestamp of the current POWERLINK cycle

\return    tOplkError
\retval    kErrorOk            no error
\retval    otherwise                 post error event to API layer

\ingroup module_main
*/
//------------------------------------------------------------------------------
static tOplkError appif_syncCb(tSocTimeStamp* socTimeStamp_p)
{
    tOplkError         oplkret = kErrorOk;
    tAppIfStatus       ret = kAppIfSuccessful;
    tTimeInfo          time;
    tNetTime *         pNetTime = NULL;

    oplkret = oplk_copyRxPdoToApp();
    if(oplkret != kErrorOk)
        goto Exit;

    appif_pdoProcFinished(tPdoDirRpdo);

    // CN is configured (cycle time is set)
    if (mainInstance_l.cycleTime != 0 &&
            mainInstance_l.plkState >= kNmtCsReadyToOperate)
    {
        time.relativeTimeLow_m = (UINT32)socTimeStamp_p->relTime;
        time.relativeTimeHigh_m = (UINT32)(socTimeStamp_p->relTime>>32);
        time.fTimeValid_m = socTimeStamp_p->fSocRelTimeValid;
        time.fCnIsOperational_m = (mainInstance_l.plkState == kNmtCsOperational) ? TRUE : FALSE;

        ret = status_process(&time);
        if(ret != kAppIfSuccessful)
        {
            oplkret = kErrorInvalidOperation;
            goto Exit;
        }

        pNetTime = &socTimeStamp_p->netTime;

        // Handle synchronous task of application interface
        ret = appif_handleSync(pNetTime);
        if(ret != kAppIfSuccessful)
        {
            oplkret = kErrorInvalidOperation;
            goto Exit;
        }
    }

    oplkret = oplk_copyTxPdoFromApp();
    if(oplkret != kErrorOk)
        goto Exit;

    appif_pdoProcFinished(tPdoDirTpdo);

Exit:
    return oplkret;
}

/// \}

