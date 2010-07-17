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
// Copyright © 2010 Tao Yue.  All rights reserved.
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
// If the keystroke happens within a composition, eat the key and return S_OK.
//
//----------------------------------------------------------------------------

HRESULT CTextService::_HandleCharacterKey(TfEditCookie ec, ITfContext *pContext, UINT wVirtKey, UINT wScanCode)
{
    // Start the new compositon if there is no composition.
    if (!_IsComposing())
        _StartComposition(pContext);

    // Convert virtual key to character, adjusting v.
    BYTE state[256];
    GetKeyboardState(state);
    WCHAR buf[16]; 
    int numChars = ToUnicodeEx(wVirtKey, wScanCode, state, (LPWSTR)buf, 16, 0, 0);
    WCHAR ch = buf[0];
    if (ch == 'v') ch = PinyinVowels::v0;
    else if (ch == 'V') ch = PinyinVowels::V0;

    // If a tone character was entered, then 
    if ((ch >= '1') && (ch <= '4'))
    {
      return _AddTone(ch, ec, pContext);
    }
    else
    {
      return _InsertCharacter(ch, ec, pContext);
    }
}

int CTextService::_LookupPinyinVowel(WCHAR ch, WCHAR *vowels, int length)
{
  for (int i=0; i < length; i++)
  {
    if (vowels[i] == ch)
      return i;
  }
  return -1;
}

HRESULT CTextService::_AddTone(WCHAR ch, TfEditCookie ec, ITfContext *pContext)
{
  ITfRange *pRangeComposition;
  TF_SELECTION tfSelection;
  ULONG cFetched;
    
  if (_pComposition->GetRange(&pRangeComposition) != S_OK)
    return S_FALSE;

  // Find the character to put a tone over
  const int MAX_COMPOSITION_LENGTH = 1024;
  WCHAR buffer[MAX_COMPOSITION_LENGTH];
  ULONG numChars;
  pRangeComposition->GetText(ec, 0, buffer, MAX_COMPOSITION_LENGTH, &numChars);

  // Locate ending vowel combination
  int iStartVowel = -1;
  int iEndVowel = -1;
  for (int i = (int)numChars - 1; i >= 0; i--)
  {
    if ((_LookupPinyinVowel(buffer[i], (WCHAR*)PinyinVowels::lowercaseToned, PinyinVowels::NUM_TONED_VOWELS) > -1)
     || (_LookupPinyinVowel(buffer[i], (WCHAR*)PinyinVowels::uppercaseToned, PinyinVowels::NUM_TONED_VOWELS) > -1)
     || (_LookupPinyinVowel(buffer[i], (WCHAR*)PinyinVowels::lowercaseUntoned, PinyinVowels::NUM_UNTONED_VOWELS) > -1)
     || (_LookupPinyinVowel(buffer[i], (WCHAR*)PinyinVowels::uppercaseUntoned, PinyinVowels::NUM_UNTONED_VOWELS) > -1))
    {
      if (iEndVowel == -1)
      {
        iEndVowel = i;
        iStartVowel = i;
      }
      else if (i == (iStartVowel - 1))
        iStartVowel = i;
      else
        break;
    }
    else if (iEndVowel > -1)
      break;
  }
  
  if (iStartVowel > -1)
  {
    // Convert target vowels to lowercase
    for (int i = iStartVowel; i <= iEndVowel; i++)
    {
      int iLowerToned = _LookupPinyinVowel(buffer[i], (WCHAR*)PinyinVowels::lowercaseToned, PinyinVowels::NUM_TONED_VOWELS);
      if (iLowerToned > -1)
      {
        buffer[i] = PinyinVowels::lowercaseUntoned[iLowerToned / 4];
        continue;
      }

      int iUpperToned = _LookupPinyinVowel(buffer[i], (WCHAR*)PinyinVowels::uppercaseToned, PinyinVowels::NUM_TONED_VOWELS);
      if (iUpperToned > -1)
      {
        buffer[i] = PinyinVowels::uppercaseUntoned[iUpperToned / 4];
        continue;
      }
    }
    
    // Determine the appropriate vowel to place the mark over.
    // Rules are as given at: http://www.pinyin.info/rules/where.html
    int iToneVowel = -1;
    for (int i = (int)iEndVowel; i >= iStartVowel; i--)
    {
      // a and e always take the tone mark.
      if ((buffer[i] == 'a') || (buffer[i] == 'A') || (buffer[i] == 'e') || (buffer[i] == 'E'))
      {
        iToneVowel = i;
        break;
      }
      // Place ou tone mark on the o.
      else if ((buffer[i] == 'u') || (buffer[i] == 'U'))
      {
        if (i > iStartVowel)
        {
          if ((buffer[i-1] == 'o') || (buffer[i-1] == 'O'))
          {
            iToneVowel = i - 1;
            break;
          }
        }
      }
    }

    // All other combinations place tone mark on last vowel.
    if (iToneVowel == -1)
      iToneVowel = iEndVowel;

    // Place vowel over target vowel
    int toneNum = ch - '1';
    int iLowerUntoned = _LookupPinyinVowel(buffer[iToneVowel], (WCHAR*)PinyinVowels::lowercaseUntoned, PinyinVowels::NUM_UNTONED_VOWELS);
    if (iLowerUntoned > -1)
    {
      buffer[iToneVowel] = PinyinVowels::lowercaseToned[iLowerUntoned * 4 + toneNum];
    }
    else
    {
      int iUpperUntoned = _LookupPinyinVowel(buffer[iToneVowel], (WCHAR*)PinyinVowels::uppercaseUntoned, PinyinVowels::NUM_UNTONED_VOWELS);
      buffer[iToneVowel] = PinyinVowels::uppercaseToned[iUpperUntoned * 4 + toneNum];
    }
  }

  // Update the selection point to just after the inserted text
  if (pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &cFetched) != S_OK || cFetched != 1)
    goto Exit;

  pRangeComposition->SetText(ec, TF_ST_CORRECTION, buffer, numChars);
  tfSelection.range->Collapse(ec, TF_ANCHOR_END);
  pContext->SetSelection(ec, 1, &tfSelection);
  _SetCompositionDisplayAttributes(ec, pContext, _gaDisplayAttributeInput);
  tfSelection.range->Release();
  
Exit:
  pRangeComposition->Release();
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

