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
// CStartCompositinoEditSession
//
//----------------------------------------------------------------------------

class CStartCompositionEditSession : public CEditSessionBase
{
public:
    CStartCompositionEditSession(CTextService *pTextService, ITfContext *pContext) : CEditSessionBase(pTextService, pContext)
    {
    }

    // ITfEditSession
    STDMETHODIMP DoEditSession(TfEditCookie ec);
};

//+---------------------------------------------------------------------------
//
// DoEditSession
//
//----------------------------------------------------------------------------

STDAPI CStartCompositionEditSession::DoEditSession(TfEditCookie ec)
{
    ITfInsertAtSelection *pInsertAtSelection = NULL;
    ITfRange *pRangeInsert = NULL;
    ITfContextComposition *pContextComposition = NULL;
    ITfComposition *pComposition = NULL;
    HRESULT hr = E_FAIL;

    // A special interface is required to insert text at the selection
    hr = _pContext->QueryInterface(IID_ITfInsertAtSelection,
        (void **)&pInsertAtSelection);
    if (hr != S_OK)
    {
        goto Exit;
    }

    // insert the text
    hr = pInsertAtSelection->InsertTextAtSelection(ec,
        TF_IAS_QUERYONLY, NULL, 0, &pRangeInsert);
    if (hr != S_OK)
    {
        goto Exit;
    }

    // get an interface on the context to deal with compositions
    hr = _pContext->QueryInterface(IID_ITfContextComposition,
        (void **)&pContextComposition);
    if (hr != S_OK)
    {
        goto Exit;
    }

    // start the new composition
    hr = pContextComposition->StartComposition(ec,
        pRangeInsert, _pTextService, &pComposition);
    if ((hr != S_OK) || (pComposition == NULL))
    {
        goto Exit;
    }

    // Store the pointer of this new composition object in the instance 
    // of the CTextService class. So this instance of the CTextService 
    // class can know now it is in the composition stage.
    _pTextService->_SetComposition(pComposition);

    // 
    //  set selection to the adjusted range
    // 
    TF_SELECTION tfSelection;
    tfSelection.range = pRangeInsert;
    tfSelection.style.ase = TF_AE_NONE;
    tfSelection.style.fInterimChar = FALSE;
    hr = _pContext->SetSelection(ec, 1, &tfSelection);

Exit:
    SafeRelease(&pContextComposition);
    SafeRelease(&pRangeInsert);
    SafeRelease(&pInsertAtSelection);
    return hr;
}

//+---------------------------------------------------------------------------
//
// _StartComposition
//
// Starts the new composition at the selection of the current focus context
//
//----------------------------------------------------------------------------

HRESULT CTextService::_StartComposition(ITfContext *pContext)
{
    CStartCompositionEditSession *pStartCompositionEditSession;

    if (pStartCompositionEditSession = new CStartCompositionEditSession(this, pContext))
    {
        HRESULT hr;
        HRESULT hrSession;

        // A synchronous document write lock is required.
        // The CStartCompositionEditSession will do all the work when the
        // CStartCompositionEditSession::DoEditSession method is called by the context
        hr = pContext->RequestEditSession(
            _tfClientId,
            pStartCompositionEditSession,
            TF_ES_SYNC | TF_ES_READWRITE,
            &hrSession);

        if (hr != S_OK)
        {
            return hr;
        }
        if (hrSession != S_OK)
        {
            return hrSession;
        }
        SafeRelease(&pStartCompositionEditSession);
    }

    return S_OK;
}
