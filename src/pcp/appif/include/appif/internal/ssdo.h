/**
********************************************************************************
\file   appif/internal/ssdo.h

\brief  Internal header file of the SSDO module

This file contains internal definitions for the SSDO module.

*******************************************************************************/

/*------------------------------------------------------------------------------
Copyright (c) 2013, Bernecker+Rainer Industrie-Elektronik Ges.m.b.H. (B&R)
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

#ifndef _INC_appif_int_ssdo_H_
#define _INC_appif_int_ssdo_H_

//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------

#include <appif/pcpglobal.h>

#include <appif/tbuf.h>
#include <appif/fifo.h>

#include <appifcommon/timeout.h>
#include <config/ssdo.h>

#include <Epl.h>

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// typedef
//------------------------------------------------------------------------------

/**
 * \brief State machine type for the consuming transmit buffer
 */
typedef enum {
    kConsTxStateInvalid            = 0x00,
    kConsTxStateWaitForFrame       = 0x01,
    kConsTxStateProcessFrame       = 0x02,
    kConsTxStateWaitForTxFinished  = 0x03,
    kConsTxStateWaitForNextArpRetry = 0x04,
    kConsTxStateTxFinished         = 0x05,
} tConsTxState;

/**
 * \brief State machine type for the producing receive buffer
 */
typedef enum {
    kProdRxStateInvalid            = 0x00,
    kProdRxStateWaitForFrame       = 0x01,
    kProdRxStateRepostFrame        = 0x02,
} tProdRxState;

/**
 * \brief
 */
typedef struct {
    UINT8    ssdoStubDataBuff_m[SSDO_STUB_DATA_DOM_SIZE];
    UINT32   ssdoStubSize_m;
} tProdRxBuffer;

/**
 * \brief Parameter type of the consuming transmit buffer
 */
typedef struct {
    tTbufInstance     pTbufConsTxInst_m;    ///< Instance pointer to the consuming transmit triple buffer
    tSeqNrValue       currConsSeq_m;        ///< Consuming buffer sequence number
    tConsTxState      consTxState_m;        ///< State of the consuming transmit buffer
    tEplSdoComConHdl  sdoComConHdl_m;       ///< SDO connection handler
    UINT8*            pConsTxPayl_m;        ///< Pointer to transmit buffer
    tTimeoutInstance  pArpTimeoutInst_m;    ///< Timer for ARP request retry
} tSsdoConsTx;

/**
 * \brief Parameter type of the producing receive buffer
 */
typedef struct {
    tTbufInstance     pTbufProdRxInst_m;    ///< Instance pointer to the producing receive triple buffer
    tFifoInstance     pRxFifoInst_m;        ///< producing receive FIFO instance pointer
    tProdRxState      prodRxState_m;        ///< State of the producing receive buffer
    tSeqNrValue       currProdSeq_m;        ///< Current producing buffer sequence number
    tProdRxBuffer     prodRecvBuff_m;       ///< Producing receive buffer for packet retransmission
    tTimeoutInstance  pTimeoutInst_m;       ///< Timer for SSDO transmissions over the tbuf
    UINT16            objSize_m;            ///< Size of incomming object
} tSsdoProdRx;

/**
\brief SSDO channel user instance

The SSDO instance holds configuration information of each SSDO channel.
*/
struct eSsdoInstance
{
    tSsdoChanNum   instId_m;             ///< Id of the SSDO instance
    tSsdoConsTx    consTxParam_m;        ///< Consuming receive buffer parameters
    tSsdoProdRx    prodRxParam_m;        ///< Producing transmit buffer parameters
};

//------------------------------------------------------------------------------
// function prototypes
//------------------------------------------------------------------------------

#endif /* _INC_appif_int_ssdo_H_ */


