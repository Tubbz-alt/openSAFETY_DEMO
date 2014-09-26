/**
********************************************************************************
\file   main.c

\brief Safety node firmware main instance

Entry point of the safety node firmware. Initializes all submodules and the
stack and processes the background task.

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

/*----------------------------------------------------------------------------*/
/* includes                                                                   */
/*----------------------------------------------------------------------------*/

#include <sn/global.h>

#include <shnf/shnf.h>
#include <sapl/sapl.h>

#include <SNMTSapi.h>
#include <SCFMapi.h>
#include <SSCapi.h>

/*============================================================================*/
/*            G L O B A L   D E F I N I T I O N S                             */
/*============================================================================*/

/*----------------------------------------------------------------------------*/
/* const defines                                                              */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* module global vars                                                         */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/* global function prototypes                                                 */
/*----------------------------------------------------------------------------*/


/*============================================================================*/
/*            P R I V A T E   D E F I N I T I O N S                           */
/*============================================================================*/

/*----------------------------------------------------------------------------*/
/* const defines                                                              */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* local types                                                                */
/*----------------------------------------------------------------------------*/

typedef struct
{
    BOOLEAN fShutdown_m;                   /**< TRUE if the SN should perform a shutdown */
    BOOLEAN fFailSafe_m;                   /**< TRUE if the SN should enter fail safe state */

    BOOLEAN fEnterOperational_m;           /**< TRUE if the SN should enter operational state */
    SNMTS_t_SN_STATE_MAIN lastSnState_m;   /**< Consists of the last SN state */
} tMainInstance;

/*----------------------------------------------------------------------------*/
/* local vars                                                                 */
/*----------------------------------------------------------------------------*/

static tMainInstance instance_l SAFE_INIT_SEKTOR;

#ifdef _DEBUG
static char *strSnStates[] = { "SNMTS_k_ST_INITIALIZATION",
                               "SNMTS_k_ST_PRE_OPERATIONAL",
                               "SNMTS_k_ST_OPERATIONAL"};
#endif

/*----------------------------------------------------------------------------*/
/* local function prototypes                                                  */
/*----------------------------------------------------------------------------*/
static BOOLEAN initOpenSafety(void);

static BOOLEAN process(void);
#ifdef _DEBUG
static void printSNState(void);
#endif
static BOOLEAN enterPreOperational(void);
static BOOLEAN enterOperational(void);

static BOOLEAN enterFailSafe(void);
static void shutdown(void);



/*============================================================================*/
/*            P U B L I C   F U N C T I O N S                                 */
/*============================================================================*/

/*----------------------------------------------------------------------------*/
/**
\brief    Entry function of CN API example

main() implements the main program function of the CN API example.
First all initialization is done, then the program runs in a loop where the
APs state machine will be updated and input/output ports will be processed.

\retval 0          On successful shutdown
\retval -1         Shutdown on error

\ingroup module_main
*/
/*----------------------------------------------------------------------------*/

int main (void)
{
    int retVal = -1;
    tErrHInitParam errhInitParam;

    MEMSET(&errhInitParam, 0, sizeof(tErrHInitParam));
    MEMSET(&instance_l, 0, sizeof(tMainInstance));

    DEBUG_TRACE(DEBUG_LVL_ALWAYS, "\n\n********************************************************************\n");
    DEBUG_TRACE(DEBUG_LVL_ALWAYS, "\n\topenSAFETY SafetyNode Demo \n\n ");
    DEBUG_TRACE(DEBUG_LVL_ALWAYS, "\tStack version: \t\t%s\n", EPLS_k_STACK_VERSION );
    DEBUG_TRACE(DEBUG_LVL_ALWAYS, "\tConfiguration: \t\t%s\n", EPLS_k_CONFIGURATION_STRING );
    DEBUG_TRACE(DEBUG_LVL_ALWAYS, "\tTarget System: \t\t%s\n", EPLS_k_TARGET_STRING );
    DEBUG_TRACE(DEBUG_LVL_ALWAYS, "********************************************************************\n");

    /* Initialize the error handler */
    errhInitParam.pShutdown_m = &instance_l.fShutdown_m;
    errhInitParam.pFailSafe_m = &instance_l.fFailSafe_m;

    if(errh_init(&errhInitParam))
    {
        /* Initialize the openSAFETY stack and change state to init */
        if(initOpenSafety())
        {
            /* Initialize the SHNF module */
            if(shnf_init())
            {
                /* Initialize the safe application module */
                DEBUG_TRACE(DEBUG_LVL_ALWAYS, "\nInitialize the SAPL -> ");
                if(sapl_init(&instance_l.fEnterOperational_m))
                {
                    DEBUG_TRACE(DEBUG_LVL_ALWAYS, "SUCCESS!\n");

                    /* Restore the SOD from NVS if possible */
                    if(sapl_restoreSod())
                    {
                        /* Change state of the openSAFETY stack to pre operational */
                        if(enterPreOperational())
                        {
                            DEBUG_TRACE(DEBUG_LVL_ALWAYS, "\n\nStart processing ... \n ");
                            if(process())
                            {
                                /* Shutdown triggered -> Terminate! */
                                retVal = 0;
                            }
                        }   /* no else: Error is already reported in the called function */
                    }   /* no else: Error is already reported in the called function */
                }
                else
                {
                    DEBUG_TRACE(DEBUG_LVL_ALWAYS, "ERROR!\n");
                    /* sapl_init() failed to initialize! */
                }
            }   /* no else: Error is already reported in the called function */
        }   /* no else: Error is already reported via SAPL_SERR_SignalErrorClbk */
    }   /* no else: Error is already reported in the called function */

    shutdown();

    return retVal;
}

/*============================================================================*/
/*            P R I V A T E   F U N C T I O N S                               */
/*============================================================================*/
/* \name Private Functions */
/* \{ */

/*----------------------------------------------------------------------------*/
/**
\brief    Initialize the openSAFETY stack

Initialize the openSAFETY stack and switch the SN to the Pre-operational state.

\retval TRUE    Stack initialization was successful
\retval FALSE   Error during stack initialization

\ingroup module_main
*/
/*----------------------------------------------------------------------------*/
static BOOLEAN initOpenSafety(void)
{
    BOOLEAN fReturn = FALSE; /* return value */
    SNMTS_t_SN_STATE_MAIN snState = 0;

    DEBUG_TRACE(DEBUG_LVL_ALWAYS, "\nInitialize the openSAFETY stack -> ");
    if(SSC_InitAll())
    {
        DEBUG_TRACE(DEBUG_LVL_ALWAYS, "SUCCESS!\n");

        /* if the initialization of the last SN state succeeded */
        if(SNMTS_GetSnState(B_INSTNUM_ &snState))
        {
            DEBUG_TRACE(DEBUG_LVL_ALWAYS, "\nCHANGE STATE: SN_BOOTING -> %s\n", strSnStates[snState]);
            fReturn = TRUE;
        }
    }

    /* call the Control Flow Monitoring */
    SCFM_TACK_PATH();

    return fReturn;
}

/*----------------------------------------------------------------------------*/
/**
\brief    Process all periodic actions

Initialize the openSAFETY stack and switch the SN to the Pre-operational state.

\retval TRUE    Shutdown is triggered
\retval FALSE   Processing failed due to error

\ingroup module_main
*/
/*----------------------------------------------------------------------------*/
static BOOLEAN process(void)
{
    BOOLEAN fReturn = FALSE;

    while(TRUE)
    {
        /* Periodically process the asynchronous task of the SHNF */
        if(shnf_process() == FALSE)
        {
            DEBUG_TRACE(DEBUG_LVL_ERROR,"ERROR: Processing of the SHNF failed!\n");
            break;
        }

        if(sapl_process() == FALSE)
        {
            DEBUG_TRACE(DEBUG_LVL_ALWAYS, "ERROR: Processing of the SAPL failed!\n");
            break;
        }

#ifdef _DEBUG
        printSNState();
#endif

        /* Check if operational flag is active */
        if(instance_l.fEnterOperational_m)
        {
            if(enterOperational() == FALSE)
            {
                break;
            }
        }

        /* Check if fail safe flag is active */
        if(instance_l.fFailSafe_m)
        {
            if(enterFailSafe() == FALSE)
            {
                break;
            }
        }

        if(instance_l.fShutdown_m)
        {
            fReturn = TRUE;
            break;
        }
    }

    return fReturn;
}

#ifdef _DEBUG
/*----------------------------------------------------------------------------*/
/**
\brief    Print the current state of the SN

If changed this function prints the current state of the SN to stdout.

\ingroup module_main
*/
/*----------------------------------------------------------------------------*/
static void printSNState(void)
{
    SNMTS_t_SN_STATE_MAIN actSnState;    /* to get the actual SN state */
    char * lastSnState = NULL;
    char * currSnState = NULL;

    /* if the actual SN state update succeeded */
    if(SNMTS_GetSnState(B_INSTNUM_ &actSnState))
    {
        /* if the SN state changed */
        if(actSnState != instance_l.lastSnState_m)
        {
            if(actSnState <= 2 && instance_l.lastSnState_m <= 2)
            {
                lastSnState = strSnStates[instance_l.lastSnState_m];
                currSnState = strSnStates[actSnState];

                /* signal the actual SN state */
                DEBUG_TRACE(DEBUG_LVL_ALWAYS, "\nCHANGE STATE: %s -> %s\n", lastSnState,
                                                                            currSnState);

                /*store the last SN state */
                instance_l.lastSnState_m = actSnState;
            }
        }
    }
}
#endif

/*----------------------------------------------------------------------------*/
/**
\brief    Perform state change to pre operational state

\retval TRUE    Change to pre operational successful
\retval FALSE   Failed to change the state

\ingroup module_main
*/
/*----------------------------------------------------------------------------*/
static BOOLEAN enterPreOperational(void)
{
    BOOLEAN fReturn = FALSE;
    UINT32 consTime = 0;

    consTime = shnf_getConsecutiveTime();

    /* transition to PreOperational */
    if(SNMTS_PerformTransPreOp(B_INSTNUM_ consTime))
    {
        /* Forward new state to SHNF */
        shnf_changeState(kShnfStatePreOperational);

#ifdef _DEBUG
        printSNState();
#endif

        fReturn = TRUE;
    }
    else
    {
        errh_postFatalError(kErrSourcePeriph, kErrorEnterPreOperationalFailed, 0);
    }

    return fReturn;
}

/*----------------------------------------------------------------------------*/
/**
\brief    Perform state change to operational state

\retval TRUE    Change to operational successful
\retval FALSE   Failed to change the state

\ingroup module_main
*/
/*----------------------------------------------------------------------------*/
static BOOLEAN enterOperational(void)
{
    BOOLEAN fReturn = FALSE;
    UINT8 errGrp = 0;
    UINT8 errCode = 0;

    /* Perform transition to operational */
    if(SNMTS_EnterOpState(B_INSTNUM_ TRUE, errGrp, errCode))
    {
        /* Forward new state to SHNF */
        shnf_changeState(kShnfStateOperational);

        fReturn = TRUE;
    }
    else
    {
        errh_postFatalError(kErrSourcePeriph, kErrorEnterOperationalFailed, (errGrp<<8 | errCode));
    }

    /* Reset the operation flag */
    instance_l.fEnterOperational_m = FALSE;

    return fReturn;
}

/*----------------------------------------------------------------------------*/
/**
\brief    Handle the switch to fail safe state

\retval TRUE    Change to fail safe successful
\retval FALSE   Failed to change the state

\ingroup module_main
*/
/*----------------------------------------------------------------------------*/
static BOOLEAN enterFailSafe(void)
{
    BOOLEAN fReturn = FALSE;
    UINT32 consTime = 0;

    consTime = shnf_getConsecutiveTime();

    /* Perform transition to pre operational */
    if(SNMTS_PerformTransPreOp(B_INSTNUM_ consTime))
    {
        instance_l.fFailSafe_m = FALSE;

        /* Transition to preop successful! */
        fReturn = TRUE;

        /* call the Control Flow Monitoring */
        SCFM_TACK_PATH();
    }
    else
    {
        errh_postFatalError(kErrSourcePeriph, kErrorEnterFailSafeFailed, 0);
    }

    return fReturn;
}

/*----------------------------------------------------------------------------*/
/**
\brief    Shutdown the SN and cleanup all structures

\ingroup module_main
*/
/*----------------------------------------------------------------------------*/
static void shutdown(void)
{
    shnf_exit();
    sapl_exit();

    errh_exit();
}

/* \} */