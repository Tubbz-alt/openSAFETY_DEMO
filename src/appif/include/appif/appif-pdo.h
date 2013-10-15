/**
********************************************************************************
\file   <appifff/appif-pdo.h

\brief  Application interface pdo module header

TODO

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

#ifndef _INC_APPIF_PDO_H_
#define _INC_APPIF_PDO_H_

//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------

#include <appif/appif-apglobal.h>

#include <appif/appif-tbuflayoutrpdo.h>
#include <appif/appif-tbuflayouttpdo.h>

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// typedef
//------------------------------------------------------------------------------

typedef BOOL (* tAppIfPdoCb) ( UINT32 rpdoRelTimeLow_p,
        tRpdoMappedObj* pRpdoImage_p,
        tTpdoMappedObj* pTpdoImage_p );  ///< Pdo user callback function

/**
 * \brief  Pdo module initialization structure
 */
typedef struct {
    tTbufNumLayout     buffIdRpdo_m;      ///< Id of the rpdo buffer
    tTbufNumLayout     buffIdTpdo_m;      ///< Id of the tpdo buffer
} tPdoInitParam;

//------------------------------------------------------------------------------
// function prototypes
//------------------------------------------------------------------------------
BOOL pdo_init(tAppIfPdoCb pfnPdoCb_p, tPdoInitParam* pPdoInitParam_p);
void pdo_exit(void);

#endif /* _INC_APPIF_PDO_H_ */





