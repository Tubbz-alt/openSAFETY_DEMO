/**
********************************************************************************
\file   event.c

\brief  CN Application event handler

This file contains a demo CN application event handler.

\ingroup module_main
*******************************************************************************/

/*------------------------------------------------------------------------------
Copyright (c) 2017, B&R Industrial Automation GmbH
Copyright (c) 2013, SYSTEC electronic GmbH
Copyright (c) 2016, Kalycito Infotech Private Ltd
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holders nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
------------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------
#include <oplk/oplk.h>
#include <oplk/debugstr.h>

#include <pcptarget/target.h>

#include "event.h"
#include <psi/obdict.h>

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

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------
static tEventCb pfnEventCb_l = NULL;

//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static tOplkError processStateChangeEvent(const tEventNmtStateChange* pNmtStateChange_p,
                                          void* pUserArg_p);

static tOplkError processErrorWarningEvent(const tEventError* pInternalError_p,
                                           void* pUserArg_p);

static tOplkError processUserObdAccessEvent(tObdAlConHdl* pParam_p,
                                            void* pUserArg_p);

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//


//------------------------------------------------------------------------------
/**
\brief  Initialize applications event module

The function initializes the applications event module

\param  pfnEventCb_p            User event callback

\ingroup module_demo_cn_embedded
*/
//------------------------------------------------------------------------------
void initEvents (tEventCb pfnEventCb_p)
{
    pfnEventCb_l = pfnEventCb_p;
}

//------------------------------------------------------------------------------
/**
\brief  Process openPOWERLINK events

The function implements the applications stack event handler.

\param  eventType_p         Type of event
\param  pEventArg_p         Pointer to union which describes the event in detail
\param  pUserArg_p          User specific argument

\return The function returns a tOplkError error code.

\ingroup module_demo_cn_embedded
*/
//------------------------------------------------------------------------------
tOplkError processEvents(tOplkApiEventType eventType_p,
                         const tOplkApiEventArg* pEventArg_p,
                         void* pUserArg_p)
{
    tOplkError          ret = kErrorOk;

    switch (eventType_p)
    {
        case kOplkApiEventNmtStateChange:
            ret = processStateChangeEvent(&pEventArg_p->nmtStateChange, pUserArg_p);
            break;

        case kOplkApiEventCriticalError:
        case kOplkApiEventWarning:
            ret = processErrorWarningEvent(&pEventArg_p->internalError, pUserArg_p);
            break;

        case kOplkApiEventUserObdAccess:
            ret = processUserObdAccessEvent(pEventArg_p->userObdAccess.pUserObdAccHdl, pUserArg_p);
            break;

        default:
            break;
    }

    // call user event call back
    if ((ret == kErrorOk) && (pfnEventCb_l != NULL))
        ret = pfnEventCb_l(eventType_p, pEventArg_p, pUserArg_p);

    return ret;
}

//============================================================================//
//            P R I V A T E   F U N C T I O N S                               //
//============================================================================//
/// \name Private Functions
/// \{

//------------------------------------------------------------------------------
/**
\brief  Process state change events

The function processes state change events.

\param  eventType_p         Type of event
\param  pEventArg_p         Pointer to union which describes the event in detail
\param  pUserArg_p          User specific argument

\return The function returns a tOplkError error code.
*/
//------------------------------------------------------------------------------
static tOplkError processStateChangeEvent(const tEventNmtStateChange* pNmtStateChange_p,
                                          void* pUserArg_p)
{
    UNUSED_PARAMETER(pUserArg_p);

    PRINTF("StateChangeEvent(0x%X) originating event = 0x%X (%s)\n",
           pNmtStateChange_p->newNmtState,
           pNmtStateChange_p->nmtEvent,
           debugstr_getNmtEventStr(pNmtStateChange_p->nmtEvent));

    return kErrorOk;
}

//------------------------------------------------------------------------------
/**
\brief  Process error and warning events

The function processes error and warning events.

\param  eventType_p         Type of event
\param  pEventArg_p         Pointer to union which describes the event in detail
\param  pUserArg_p          User specific argument

\return The function returns a tOplkError error code.
*/
//------------------------------------------------------------------------------
static tOplkError processErrorWarningEvent(const tEventError* pInternalError_p,
                                           void* pUserArg_p)
{
    // error or warning occurred within the stack or the application
    // on error the API layer stops the NMT state machine

    UNUSED_PARAMETER(pUserArg_p);

    PRINTF("Err/Warn: Source = %s (%02X) EplError = %s (0x%03X)\n",
                debugstr_getEventSourceStr(pInternalError_p->eventSource),
                pInternalError_p->eventSource,
                debugstr_getRetValStr(pInternalError_p->oplkError),
                pInternalError_p->oplkError);

    PRINTF("Err/Warn: Source = %s (%02X) EplError = %s (0x%03X)\n",
                debugstr_getEventSourceStr(pInternalError_p->eventSource),
                pInternalError_p->eventSource,
                debugstr_getRetValStr(pInternalError_p->oplkError),
                pInternalError_p->oplkError);

    // check additional argument
    switch (pInternalError_p->eventSource)
    {
        case kEventSourceEventk:
        case kEventSourceEventu:
            // error occurred within event processing
            // either in kernel or in user part
            PRINTF(" OrgSource = %s %02X\n",
                     debugstr_getEventSourceStr(pInternalError_p->errorArg.eventSource),
                     pInternalError_p->errorArg.eventSource);

            PRINTF(" OrgSource = %s %02X\n",
                     debugstr_getEventSourceStr(pInternalError_p->errorArg.eventSource),
                     pInternalError_p->errorArg.eventSource);
            break;

        case kEventSourceDllk:
            // error occurred within the data link layer (e.g. interrupt processing)
            // the DWORD argument contains the DLL state and the NMT event
            PRINTF(" val = %X\n", pInternalError_p->errorArg.uintArg);
            PRINTF(" val = %X\n", pInternalError_p->errorArg.uintArg);
            break;

        default:
            PRINTF("\n");
            break;
    }
    return kErrorOk;
}

//------------------------------------------------------------------------------
/**
\brief  Process user specific object access Events

The function processes user specific non indexed Object access events.

\param  eventType_p         Type of event
\param  pEventArg_p         Pointer to union which describes the event in detail
\param  pUserArg_p          User specific argument

\return The function returns a tOplkError error code.
*/
//------------------------------------------------------------------------------
static tOplkError processUserObdAccessEvent(tObdAlConHdl* pParam_p,
                                            void* pUserArg_p)
{
    tOplkError               oplkret = kErrorOk;

    UNUSED_PARAMETER(pUserArg_p);

    switch (pParam_p->index)
    {
#if(((PSI_MODULE_INTEGRATION) & (PSI_MODULE_CC)) != 0)
        case 0x2000:
            oplkret = cc_obdAccessCb(pParam_p);
            break;
#endif
#if(((PSI_MODULE_INTEGRATION) & (PSI_MODULE_SSDO)) != 0)
        case 0x2130:
            oplkret = rssdo_obdAccessCb(pParam_p);
            break;
#endif
        default:
            oplkret = kErrorObdIndexNotExist;
    }
    return oplkret;
}

///\}
