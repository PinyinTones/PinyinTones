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
#include "Pinyin.h"
#include "TextService.h"

//+---------------------------------------------------------------------------
//
// CKeyHandlerEditSession
//
//----------------------------------------------------------------------------

class CKeyHandlerEditSession : public CEditSessionBase
{
public:
    CKeyHandlerEditSession(CTextService *pTextService, ITfContext *pContext, WPARAM wParam, LPARAM lParam) : CEditSessionBase(pTextService, pContext)
    {
        _wVirtKey = (UINT)wParam;
        _wScanCode = (lParam & 0x0000ff00) >> 8;
    }

    // ITfEditSession
    STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
    UINT _wVirtKey;
    UINT _wScanCode;

};

//+---------------------------------------------------------------------------
//
// DoEditSession
//
//----------------------------------------------------------------------------

STDAPI CKeyHandlerEditSession::DoEditSession(TfEditCookie ec)
{
    switch (_wVirtKey)
    {
        case VK_RETURN:
            return _pTextService->_HandleReturnKey(ec, _pContext);

        case VK_ESCAPE:
            return _pTextService->_HandleEscapeKey(ec, _pContext);

        case VK_BACK:
            return _pTextService->_HandleBackspaceKey(ec, _pContext);

        default:
            return _pTextService->_HandleKey(ec, _pContext, _wVirtKey, _wScanCode);
            break;
    }

    return S_OK;

}

//+---------------------------------------------------------------------------
//
// IsRangeCovered
//
// Returns TRUE if pRangeTest is entirely contained within pRangeCover.
//
//----------------------------------------------------------------------------

BOOL IsRangeCovered(TfEditCookie ec, ITfRange *pRangeTest, ITfRange *pRangeCover)
{
    LONG lResult;

    if (pRangeCover->CompareStart(ec, pRangeTest, TF_ANCHOR_START, &lResult) != S_OK ||
        lResult > 0)
    {
        return FALSE;
    }

    if (pRangeCover->CompareEnd(ec, pRangeTest, TF_ANCHOR_END, &lResult) != S_OK ||
        lResult < 0)
    {
        return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// _HandleKey
//
// Handles a keystroke that is going into a new or existing composition.
//
//----------------------------------------------------------------------------

HRESULT CTextService::_HandleKey(TfEditCookie ec,
    ITfContext *pContext, UINT wVirtKey, UINT wScanCode)
{
    BYTE state[256];
    GetKeyboardState(state);
    BOOL fControlKey = state[VK_CONTROL] & 0x80;
    BYTE fRMenuKey = state[VK_RMENU] & 0x80;

    // Ignore control characters, but allow AltGr (Ctrl + RightAlt).  However,
    // not every application passes through AltGr, so we may not get a chance
    // to handle the character.
    if (fControlKey && !fRMenuKey)
    {
        return S_OK;
    }

    // Ignore keys that PinyinTones does not handle
    BOOL fKeyInsertable = _IsKeyInsertable(wVirtKey);
    if (!fKeyInsertable)
    {
        return S_OK;
    }

    // Convert virtual key to character
    WCHAR buf[16]; 
    int numChars = ToUnicodeEx(wVirtKey, wScanCode, state, (LPWSTR)buf, 16, 0, 0);
    switch (numChars)
    {
        case -1: // Dead key
        case 0:  // No Unicode character translation
            return S_OK;

        case 1:  // Possible Pinyin keystroke
            return _HandleCharacter(ec, pContext, buf[0]);

        default:
            if (numChars < -1)
            {
                // Undefined return value
                return E_FAIL;
            }

            // Ignore multi-character buffers, such as combining diacritics
            return S_OK;
    }

    // We should never get past the switch-case
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// _IsKeyInsertable
//
// Returns TRUE if the key can be inserted into a composition; FALSE otherwise.
//
//----------------------------------------------------------------------------

BOOL CTextService::_IsKeyInsertable(WPARAM wVirtKey)
{
    // Only specific keys are inserted by PinyinTones within a composition.
    // Other keys are ignored.
    BOOL fHandledKey = (wVirtKey == VK_SPACE)
        || ((wVirtKey >= L'0') && (wVirtKey <= L'9'))
        || ((wVirtKey >= L'A') && (wVirtKey <= L'Z'))
        || ((wVirtKey >= VK_NUMPAD0) && (wVirtKey <= VK_NUMPAD9))
        || ((wVirtKey >= VK_MULTIPLY) && (wVirtKey <= VK_DIVIDE))
        || ((wVirtKey >= VK_OEM_1) && (wVirtKey <= VK_OEM_3))
        || ((wVirtKey >= VK_OEM_4) && (wVirtKey <= VK_OEM_8))
        || (wVirtKey == VK_OEM_102)
        || ((wVirtKey & 0x00FF) == VK_PACKET);
    return fHandledKey;
}

//+---------------------------------------------------------------------------
//
// _HandleCharacter
//
// Handles a single character that is being inserted into a composition.
//
//----------------------------------------------------------------------------

HRESULT CTextService::_HandleCharacter(TfEditCookie ec,
    ITfContext *pContext, WCHAR ch)
{
    HRESULT hr = S_OK;

    // Start a composition if we're not already in one
    if (!_IsComposing())
    {
        hr = _StartComposition(pContext);
        EXIT_IF_FAILED(hr);
    }

    // Handle numerals
    if ((ch >= L'0') && (ch <= L'9'))
    {
        // Handle tones, but ignore other numerals.  It would be inconsistent
        // for those other numerals to be inserted into the composition.
        if ((ch >= L'1') && (ch <= L'4'))
        {
            hr = _SetTone(ch, ec, pContext);
        }
        goto Exit;
    }
    // Handle the 'v' character, which can be used to type the 'ü' character
    if ((ch == L'v') || (ch == L'V'))
    {
        hr = _HandleVCharacter(ch, ec, pContext);
        goto Exit;
    }

    // Insert character into composition
    hr = _InsertCharacter(ch, ec, pContext);
    EXIT_IF_FAILED(hr);

    // Non-Pinyin characters should terminate the composition
    if (!_IsPinyinCharacter(ch))
    {
        _TerminateComposition(ec, pContext);
    }

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
// _LookupChar
//
// Returns the index of a character in an array.
//
//----------------------------------------------------------------------------

int CTextService::_LookupChar(WCHAR ch, WCHAR *vowels, int cbVowels)
{
    WCHAR *pVowelsLim = vowels + cbVowels;
    for (WCHAR* p = vowels; p < pVowelsLim; p++)
    {
        if (*p == ch)
            return (int)(p - vowels);
    }
    return -1;
}

//+---------------------------------------------------------------------------
//
// _IsPinyinCharacter
//
// Determines whether a character can be used in a Pinyin syllable.
//
//----------------------------------------------------------------------------

BOOL CTextService::_IsPinyinCharacter(WCHAR ch)
{
    // Characters that we'll accept:
    //   - Alphabetic letters (including v)
    //   - Toned and untoned Pinyin vowels

    BOOL fPinyinCharacter =
        ((ch >= L'a') && (ch <= L'z')) ||
        ((ch >= L'A') && (ch <= L'Z')) ||
        (_LookupChar(ch, (WCHAR*)PinyinVowels::VOWELS,
            PinyinVowels::NUM_VOWELS) > -1);
    return fPinyinCharacter;
}

//+---------------------------------------------------------------------------
//
// _FindLastVowels
//
// Finds the last vowel or vowel combination in the buffer.
//
//----------------------------------------------------------------------------

void CTextService::_FindLastVowels(WCHAR* buffer, int cbBuffer, WCHAR** ppVowelFirst, WCHAR** ppVowelLast)
{
    // Initialize to invalid values
    *ppVowelFirst = NULL;
    *ppVowelLast = NULL;

    // Search from the end
    WCHAR* pBufferLast = buffer + cbBuffer - 1;
    for (WCHAR* p = pBufferLast; p >= buffer; p--)
    {
        // Vowel found
        if (_LookupChar(*p, (WCHAR*)PinyinVowels::VOWELS, PinyinVowels::NUM_VOWELS) > -1)
        {
            if (*ppVowelLast == NULL)
            {
                // Found the last vowel in the buffer
                *ppVowelLast = p;
                *ppVowelFirst = p;
            }
            else if (p == (*ppVowelFirst - 1))
            {
                // Found an adjacent vowel, so expand the vowel combination
                *ppVowelFirst = p;
            }
            else
            {
                // We really shouldn't ever get here, but quit if we do
                break;
            }
        }
        // Not a vowel
        else if (*ppVowelLast >= buffer)
        {
            // Quit if we've already found a vowel combination, otherwise keep
            // searching for vowels.
            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
// _RemoveTone
//
// Removes the tone mark from a Pinyin vowel.
//
//----------------------------------------------------------------------------

void CTextService::_RemoveTone(WCHAR* pVowelFirst, WCHAR* pVowelLast)
{
    for (WCHAR* p = pVowelFirst; p <= pVowelLast; p++)
    {
        int iVowel = _LookupChar(*p, (WCHAR*)PinyinVowels::VOWELS, PinyinVowels::NUM_VOWELS);
        if (iVowel > -1)
        {
            // Arranged in multiples of five: untoned vowel, then four tones.
            *p = PinyinVowels::VOWELS[iVowel - iVowel % 5];
        }
    }
}

//+---------------------------------------------------------------------------
//
// _SetTone
//
// Sets the tone mark on a Pinyin vowel or vowel combination.
//
//----------------------------------------------------------------------------

void CTextService::_SetTone(WCHAR* pVowelFirst, WCHAR* pVowelLast, WCHAR ch)
{
    if (pVowelFirst == NULL)
        return;

    // Remove tone before setting it (we don't want to have to account for
    // every tone in the vowel combination logic).
    _RemoveTone(pVowelFirst, pVowelLast);

    // Determine which vowel in the combination gets the tone.
    // Rules are as given at: http://www.pinyin.info/rules/where.html
    WCHAR* pToneVowel = NULL;
    for (WCHAR* p = pVowelLast; p >= pVowelFirst; p--)
    {
        // a and e always get the tone in a combination
        if ((*p == 'a') || (*p == 'A') || (*p == 'e') || (*p == 'E'))
        {
            pToneVowel = p;
            break;
        }
        // o gets the tone in an ou combination
        else if ((p > pVowelFirst) && ((*p == 'u') || (*p == 'U')))
        {
            WCHAR* pPred = p - 1;
            if ((*pPred == 'o') || (*pPred == 'O'))
            {
                pToneVowel = pPred;
                break;
            }
        }
    }

    // In all other cases, the tone mark goes on the last vowel.
    if (pToneVowel == NULL)
        pToneVowel = pVowelLast;

    // Place tone over the target vowel
    int iVowel = _LookupChar(*pToneVowel, (WCHAR*)PinyinVowels::VOWELS, PinyinVowels::NUM_VOWELS);
    if ((iVowel > -1) && (iVowel % 5 == 0))
    {
        int iTone = ch - '0';
        *pToneVowel = PinyinVowels::VOWELS[iVowel + iTone];
    }
}

//+---------------------------------------------------------------------------
//
// _ReplaceCompositionText
//
// Replaces the text in the composition.
//
//----------------------------------------------------------------------------

HRESULT CTextService::_ReplaceCompositionText(WCHAR* buffer, ULONG cbBuffer,
    ITfRange* pRangeComposition, TfEditCookie ec, ITfContext *pContext)
{
    HRESULT hr = S_OK;
    TF_SELECTION tfSelection;
    tfSelection.range = nullptr;

    // Get the current selection
    ULONG cFetched;
    hr = pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &cFetched);
    EXIT_IF_FAILED(hr);
    EXIT_IF_WITH(cFetched != 1, E_FAIL);

    // Replace the composition with the text in the buffer
    hr = pRangeComposition->SetText(ec, TF_ST_CORRECTION, buffer, cbBuffer);
    EXIT_IF_FAILED(hr);

    // SetText clears the composition display attribute, so restore it.
    _SetCompositionDisplayAttributes(ec, pContext, _gaDisplayAttributeInput);

    // SetText moves the cursor to the start of the composition.  Move it back
    // to its original location.
    hr = tfSelection.range->Collapse(ec, TF_ANCHOR_END);
    EXIT_IF_FAILED(hr);
    hr = pContext->SetSelection(ec, 1, &tfSelection);
    EXIT_IF_FAILED(hr);

Exit:
    if (tfSelection.range)
    {
        tfSelection.range->Release();
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// _SetTone
//
// Sets the tone mark on the last vowel or vowel combination, then terminates
// the current composition.
//
//----------------------------------------------------------------------------

HRESULT CTextService::_SetTone(WCHAR ch, TfEditCookie ec, ITfContext *pContext)
{
    HRESULT hr = S_OK;
    ITfRange *pRangeComposition = nullptr;

    hr = _pComposition->GetRange(&pRangeComposition);
    EXIT_IF_FAILED(hr);

    // Get the text in the composition
    WCHAR buffer[MAX_COMPOSITION_LENGTH];
    ULONG cbBuffer;
    hr = pRangeComposition->GetText(ec, 0, buffer, MAX_COMPOSITION_LENGTH, &cbBuffer);
    EXIT_IF_FAILED(hr);

    // Set the tone in the text buffer
    WCHAR* pVowelFirst = NULL;
    WCHAR* pVowelLast = NULL;
    _FindLastVowels(buffer, cbBuffer, &pVowelFirst, &pVowelLast);
    _SetTone(pVowelFirst, pVowelLast, ch);

    // Replace the composition text with the buffer text
    hr = _ReplaceCompositionText(buffer, cbBuffer, pRangeComposition,
        ec, pContext);
    EXIT_IF_FAILED(hr);

    // Every step has succeeded, so terminate the composition
    _TerminateComposition(ec, pContext);

Exit:
    if (pRangeComposition)
    {
        pRangeComposition->Release();
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// _HandleVCharacter
//
// Handles the 'v' character.  One 'v' will insert a Pinyin 'ü' into the
// composition, and a second 'v' will convert it back into a 'v'.
//
//----------------------------------------------------------------------------

HRESULT CTextService::_HandleVCharacter(WCHAR ch,
    TfEditCookie ec, ITfContext *pContext)
{
    HRESULT hr = S_OK;
    ITfRange *pRangeComposition = nullptr;

    EXIT_IF_WITH((ch != 'v') && (ch != L'V'), E_FAIL);

    hr = _pComposition->GetRange(&pRangeComposition);
    EXIT_IF_FAILED(hr);

    // Get the text in the composition
    WCHAR buffer[MAX_COMPOSITION_LENGTH];
    ULONG cbBuffer;
    hr = pRangeComposition->GetText(ec, 0,
        buffer, MAX_COMPOSITION_LENGTH, &cbBuffer);
    EXIT_IF_FAILED(hr);

    // Change the last character to a 'v' if it is already a 'ü'
    WCHAR chLast = (cbBuffer > 0)
        ? buffer[cbBuffer - 1]
        : L'\0';
    if ((chLast == PinyinVowels::uu0) || (chLast == PinyinVowels::UU0))
    {
        buffer[cbBuffer - 1] = ch;
        hr = _ReplaceCompositionText(buffer, cbBuffer,
            pRangeComposition, ec, pContext);
        EXIT_IF_FAILED(hr);

        // Terminate the composition, as 'v' is not a Pinyin character
        _TerminateComposition(ec, pContext);
        goto Exit;
    }

    // Insert a 'ü' with the matching case
    ch = (ch == L'v')
        ? PinyinVowels::uu0
        : PinyinVowels::UU0;
    hr = _InsertCharacter(ch, ec, pContext);
    EXIT_IF_FAILED(hr);

Exit:
    if (pRangeComposition)
    {
        pRangeComposition->Release();
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// _InsertCharacter
//
// Inserts the character into the composition.
//
//----------------------------------------------------------------------------

HRESULT CTextService::_InsertCharacter(WCHAR ch, TfEditCookie ec, ITfContext *pContext)
{
    ITfRange *pRangeComposition;
    TF_SELECTION tfSelection;
    ULONG cFetched;
    BOOL fCovered;

    // first, test where a keystroke would go in the document if an insert is done
    if (pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &cFetched) != S_OK || cFetched != 1)
        return S_FALSE;

    // is the insertion point covered by a composition?
    if (_pComposition->GetRange(&pRangeComposition) == S_OK)
    {
        fCovered = IsRangeCovered(ec, tfSelection.range, pRangeComposition);

        pRangeComposition->Release();

        if (!fCovered)
        {
            goto Exit;
        }
    }

    // insert the text
    // use SetText here instead of InsertTextAtSelection because a composition was already started
    //Don't allow to the app to adjust the insertion point inside the composition
    if (tfSelection.range->SetText(ec, 0, &ch, 1) != S_OK)
        goto Exit;

    // update the selection, make it an insertion point just past
    // the inserted text.
    tfSelection.range->Collapse(ec, TF_ANCHOR_END);
    pContext->SetSelection(ec, 1, &tfSelection);

    //
    // set the display attribute to the composition range.
    //
    _SetCompositionDisplayAttributes(ec, pContext, _gaDisplayAttributeInput);

Exit:
    tfSelection.range->Release();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _HandleReturnKey
//
//----------------------------------------------------------------------------

HRESULT CTextService::_HandleReturnKey(TfEditCookie ec, ITfContext *pContext)
{
    // just terminate the composition
    _TerminateComposition(ec, pContext);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _HandleEscapeKey
//
// Cancels the composition.
//
//----------------------------------------------------------------------------

HRESULT CTextService::_HandleEscapeKey(TfEditCookie ec, ITfContext *pContext)
{
    HRESULT hr = S_OK;
    ITfRange *pRangeComposition = nullptr;

    // Get the composition range
    hr = _pComposition->GetRange(&pRangeComposition);
    EXIT_IF_FAILED(hr);

    // Delete the text in the composition
    hr = pRangeComposition->SetText(ec, TF_ST_CORRECTION, NULL, 0);
    EXIT_IF_FAILED(hr);

    // Terminate the empty composition.  Some applications will treat this as
    // an insertion and set the dirty flag on the document.  This appears to be
    // unavoidable, as the Microsoft Pinyin IME has the same problem.
    _TerminateComposition(ec, pContext);

Exit:
    if (pRangeComposition)
    {
        pRangeComposition->Release();
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// _HandleBackspaceKey
//
//----------------------------------------------------------------------------

HRESULT CTextService::_HandleBackspaceKey(TfEditCookie ec, ITfContext *pContext)
{
    // Return value and COM pointers
    HRESULT hr = S_OK;
    ITfRange *pRangeComposition = nullptr;
    TF_SELECTION tfSelection;
    tfSelection.range = nullptr;
    ITfRange* pRangeToDelete = nullptr;

    // Other local variables
    LONG cch;
    BOOL fResult;
    ULONG cFetched;

    // Get the composition range and the current selection
    hr = _pComposition->GetRange(&pRangeComposition);
    EXIT_IF_FAILED(hr);
    hr = pContext->GetSelection(ec,
        TF_DEFAULT_SELECTION, 1, &tfSelection, &cFetched);
    EXIT_IF_FAILED(hr);

    // Ignore Backspace if the cursor is at the start of the composition.
    // This should never happen, since cursor movement keys are ignored and
    // empty compositions are terminated.
    hr = tfSelection.range->IsEqualEnd(ec,
        pRangeComposition, TF_ANCHOR_START, &fResult);
    EXIT_IF_FAILED(hr);
    EXIT_IF(fResult);

    // Use the current selection as the deletion range.  Move the start anchor
    // for empty ranges, so that one character will be deleted.
    hr = tfSelection.range->Clone(&pRangeToDelete);
    EXIT_IF_FAILED(hr);
    hr = pRangeToDelete->IsEmpty(ec, &fResult);
    EXIT_IF_FAILED(hr);
    if (fResult)
    {
        hr = pRangeToDelete->ShiftStart(ec, -1, &cch, NULL);
        EXIT_IF_FAILED(hr);
    }

    // Move the cursor to the start of the deletion range, so that it will
    // remain valid after the text is deleted.
    hr = tfSelection.range->ShiftStartToRange(ec,
        pRangeToDelete, TF_ANCHOR_START);
    EXIT_IF_FAILED(hr);
    hr = tfSelection.range->Collapse(ec, TF_ANCHOR_START);
    EXIT_IF_FAILED(hr);
    hr = pContext->SetSelection(ec, 1, &tfSelection);
    EXIT_IF_FAILED(hr);

    // Remove text from the deletion range
    hr = pRangeToDelete->SetText(ec, TF_ST_CORRECTION, NULL, 0);
    EXIT_IF_FAILED(hr);

    // Terminate the remaining composition if it is now empty
    hr = pRangeComposition->IsEmpty(ec, &fResult);
    EXIT_IF_FAILED(hr);
    if (fResult)
    {
        _TerminateComposition(ec, pContext);
    }

Exit:
    if (pRangeComposition)
    {
        pRangeComposition->Release();
    }
    if (tfSelection.range)
    {
        tfSelection.range->Release();
    }
    if (pRangeToDelete)
    {
        pRangeToDelete->Release();
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// _InvokeKeyHandler
//
// This text service is interested in handling keystrokes to demonstrate the
// use the compositions. Some apps will cancel compositions if they receive
// keystrokes while a compositions is ongoing.
//
//----------------------------------------------------------------------------

HRESULT CTextService::_InvokeKeyHandler(ITfContext *pContext, WPARAM wParam, LPARAM lParam)
{
    CKeyHandlerEditSession *pEditSession;
    HRESULT hr = E_FAIL;

    // Insert a char in place of this keystroke
    if ((pEditSession = new CKeyHandlerEditSession(this, pContext, wParam, lParam)) == NULL)
        goto Exit;

    // A lock is required.
    // nb: this method is one of the few places where it is legal to use
    // the TF_ES_SYNC flag
    hr = pContext->RequestEditSession(_tfClientId, pEditSession, TF_ES_SYNC | TF_ES_READWRITE, &hr);

    pEditSession->Release();

Exit:
    return hr;
}
