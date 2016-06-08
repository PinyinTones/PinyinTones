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
        case VK_LEFT:
        case VK_RIGHT:
            return _pTextService->_HandleArrowKey(ec, _pContext, _wVirtKey);

        case VK_RETURN:
            return _pTextService->_HandleReturnKey(ec, _pContext);

        default:
            return _pTextService->_HandleCharacterKey(ec, _pContext, _wVirtKey, _wScanCode);
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
// _HandleCharacterKey
//
// Handles a keystroke that is going into a new or existing composition.
//
//----------------------------------------------------------------------------

HRESULT CTextService::_HandleCharacterKey(TfEditCookie ec,
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

// Handles a single character
HRESULT CTextService::_HandleCharacter(TfEditCookie ec,
    ITfContext *pContext, WCHAR ch)
{
    // Start a composition if we're not already in one
    if (!_IsComposing())
    {
        _StartComposition(pContext);
    }

    // Handle numerals
    if ((ch >= L'0') && (ch <= L'9'))
    {
        // A tone number will complete a Pinyin syllable
        if ((ch >= L'1') && (ch <= L'4'))
        {
            HRESULT hrSetTone = _SetTone(ch, ec, pContext);
            _TerminateComposition(ec, pContext);
            return hrSetTone;
        }

        // Other numerals should be ignored.  It would be inconsistent for some
        // numerals to show up, but not others.
        return S_OK;
    }

    // Adjust the 'v' character, which does not exist in Pinyin
    if (ch == L'v')
    {
        ch = PinyinVowels::uu0;
    }
    else if (ch == L'V')
    {
        ch = PinyinVowels::UU0;
    }

    // Insert character into composition
    HRESULT hrInsert = _InsertCharacter(ch, ec, pContext);
    if (hrInsert != S_OK)
    {
        return hrInsert;
    }

    // Non-Pinyin characters should terminate the composition
    if (!_IsPinyinCharacter(ch))
    {
        _TerminateComposition(ec, pContext);
    }

    return S_OK;
}

// Scan an array for a character, and return the index
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

// Determines whether a character can be used when typing a Pinyin syllable
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

// Find the position of the last vowel combination
void CTextService::_FindLastVowels(WCHAR* buffer, int cbBuffer, WCHAR** ppVowelFirst, WCHAR** ppVowelLast)
{
  // Initialize to invalid values
  *ppVowelFirst = NULL;
  *ppVowelLast = NULL;

  // Search from the end
  WCHAR* pBufferLast = buffer + cbBuffer - 1;
  for (WCHAR* p = pBufferLast; p >= buffer; p--)
  {
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
    else if (*ppVowelLast >= buffer)
    {
      // Quit if we've already have a vowel combination, otherwise keep
      // searching for vowels.
      break;
    }
  }  
}

void CTextService::_RemoveTone(WCHAR* pVowelFirst, WCHAR* pVowelLast)
{
  for (WCHAR* p = pVowelFirst; p <= pVowelLast ; p++)
  {
    int iVowel = _LookupChar(*p, (WCHAR*)PinyinVowels::VOWELS, PinyinVowels::NUM_VOWELS);
    if (iVowel > -1)
    {
      // Arranged in multiples of five: untoned vowel, then four tones.
      *p = PinyinVowels::VOWELS[iVowel - iVowel % 5];
    }
  }
}

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

HRESULT CTextService::_SetTone(WCHAR ch, TfEditCookie ec, ITfContext *pContext)
{
  ITfRange *pRangeComposition;

  if (_pComposition->GetRange(&pRangeComposition) != S_OK)
    return S_FALSE;

  // Find the character to put a tone over
  const int MAX_COMPOSITION_LENGTH = 1024;
  WCHAR buffer[MAX_COMPOSITION_LENGTH];
  ULONG cbBuffer;
  HRESULT hr;
  hr = pRangeComposition->GetText(ec, 0, buffer, MAX_COMPOSITION_LENGTH, &cbBuffer);
  if (hr != S_OK)
    return S_FALSE;

  // Locate the last vowel combination
  WCHAR* pVowelFirst = NULL;
  WCHAR* pVowelLast = NULL;
  _FindLastVowels(buffer, cbBuffer, &pVowelFirst, &pVowelLast);
  _SetTone(pVowelFirst, pVowelLast, ch);

  // Update the selection point to just after the inserted text
  TF_SELECTION tfSelection;
  ULONG cFetched;
  if (pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &cFetched) != S_OK || cFetched != 1)
    goto Exit;

  pRangeComposition->SetText(ec, TF_ST_CORRECTION, buffer, cbBuffer);
  tfSelection.range->Collapse(ec, TF_ANCHOR_END);
  pContext->SetSelection(ec, 1, &tfSelection);
  _SetCompositionDisplayAttributes(ec, pContext, _gaDisplayAttributeInput);
  tfSelection.range->Release();
  
Exit:
  if (pRangeComposition) pRangeComposition->Release();
  return S_OK;
}

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
// _HandleArrowKey
//
// Update the selection within a composition.
//
//----------------------------------------------------------------------------

HRESULT CTextService::_HandleArrowKey(TfEditCookie ec, ITfContext *pContext, WPARAM wParam)
{
    ITfRange *pRangeComposition;
    LONG cch;
    BOOL fEqual;
    TF_SELECTION tfSelection;
    ULONG cFetched;

    // get the selection
    if (pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &cFetched) != S_OK ||
        cFetched != 1)
    {
        // no selection?
        return S_OK; // eat the keystroke
    }

    // get the composition range
    if (_pComposition->GetRange(&pRangeComposition) != S_OK)
        goto Exit;

    // adjust the selection
    if (wParam == VK_LEFT)
    {
        if (tfSelection.range->IsEqualStart(ec, pRangeComposition, TF_ANCHOR_START, &fEqual) == S_OK &&
            !fEqual)
        {
            tfSelection.range->ShiftStart(ec, -1, &cch, NULL);
        }
        tfSelection.range->Collapse(ec, TF_ANCHOR_START);
    }
    else
    {
        // VK_RIGHT
        if (tfSelection.range->IsEqualEnd(ec, pRangeComposition, TF_ANCHOR_END, &fEqual) == S_OK &&
            !fEqual)
        {
            tfSelection.range->ShiftEnd(ec, +1, &cch, NULL);
        }
        tfSelection.range->Collapse(ec, TF_ANCHOR_END);
    }

    pContext->SetSelection(ec, 1, &tfSelection);

    pRangeComposition->Release();

Exit:
    tfSelection.range->Release();
    return S_OK; // eat the keystroke
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

