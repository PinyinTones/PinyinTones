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
#include "EditSession.h"
#include "TextService.h"

//+---------------------------------------------------------------------------
//
// CEndCompositionEditSession
//
//----------------------------------------------------------------------------

class CEndCompositionEditSession : public CEditSessionBase
{
public:
    CEndCompositionEditSession(CTextService *pTextService, ITfContext *pContext) : CEditSessionBase(pTextService, pContext)
    {
    }

    // ITfEditSession
    STDMETHODIMP DoEditSession(TfEditCookie ec)
    {
        _pTextService->_TerminateComposition(ec, _pContext);
        return S_OK;
    }

};

//+---------------------------------------------------------------------------
//
// _TerminateComposition
//
//----------------------------------------------------------------------------

void CTextService::_TerminateComposition(TfEditCookie ec, ITfContext *pContext)
{
    _CleanupComposition(ec, pContext, TRUE);
}

//+---------------------------------------------------------------------------
//
// _CleanupComposition
//
// Cleans up a composition that is being terminated.
//
//----------------------------------------------------------------------------

void CTextService::_CleanupComposition(TfEditCookie ec, ITfContext *pContext,
    BOOL fEndComposition)
{
    if (_pComposition != NULL)
    {
        // Remove the display attribute from the composition range
        _ClearCompositionDisplayAttributes(ec, pContext);

        // EndComposition may already have been called
        if (fEndComposition)
        {
            _pComposition->EndComposition(ec);
        }

        // Release the cached copy of the composition
        _pComposition->Release();
        _pComposition = NULL;
    }
}

//+---------------------------------------------------------------------------
//
// _EndComposition
//
//----------------------------------------------------------------------------

void CTextService::_EndComposition(ITfContext *pContext)
{
    CEndCompositionEditSession *pEditSession;
    HRESULT hr;
    HRESULT hrSession;

    if (pEditSession = new CEndCompositionEditSession(this, pContext))
    {
        hr = pContext->RequestEditSession(
            _tfClientId,
            pEditSession,
            TF_ES_ASYNCDONTCARE | TF_ES_READWRITE,
            &hrSession);

        pEditSession->Release();
    }
}
