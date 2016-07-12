//////////////////////////////////////////////////////////////////////
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
//  TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
// This code is released under the Microsoft Public License.  Please
// refer to LICENSE.TXT for the full text of the license.
//
// Copyright © 2010-2016 Tao Yue.  All rights reserved.
// Portions Copyright © 2003 Microsoft Corporation.  All rights reserved.
//
// Adapted from the Text Services Framework Sample Code, available under
// the Microsoft Public License from:
//    http://code.msdn.microsoft.com/tsf
//
//////////////////////////////////////////////////////////////////////


#include "Globals.h"
#include "TextService.h"

//+---------------------------------------------------------------------------
//
// OnCompositionTerminated
//
// Callback for ITfCompositionSink.  The system calls this method whenever
// someone other than this service ends a composition.
//----------------------------------------------------------------------------

STDAPI CTextService::OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition)
{
    HRESULT hr = S_OK;
    ITfRange* pRange = nullptr;
    ITfContext* pContext = nullptr;

    // Clean up the composition
    if (_pComposition)
    {
        hr = _pComposition->GetRange(&pRange);
        EXIT_IF_FAILED(hr);
        hr = pRange->GetContext(&pContext);
        EXIT_IF_FAILED(hr);

        _CleanupComposition(ecWrite, pContext, FALSE);
    }

Exit:
    if (pContext)
    {
        pContext->Release();
    }
    if (pRange)
    {
        pRange->Release();
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// _IsComposing
//
//----------------------------------------------------------------------------

BOOL CTextService::_IsComposing()
{
    return _pComposition != NULL;
}

//+---------------------------------------------------------------------------
//
// _SetComposition
//
//----------------------------------------------------------------------------

void CTextService::_SetComposition(ITfComposition *pComposition)
{
    _pComposition = pComposition;
}

