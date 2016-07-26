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


#ifndef TEXTSERVICE_H
#define TEXTSERVICE_H

#define MAX_COMPOSITION_LENGTH 1024

class CLangBarIcon;
class CLangBarItemButton;

class CTextService : public ITfTextInputProcessorEx,
                     public ITfThreadMgrEventSink,
                     public ITfTextEditSink,
                     public ITfKeyEventSink,
                     public ITfCompositionSink,
                     public ITfDisplayAttributeProvider
{
public:
    CTextService();
    ~CTextService();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITfTextInputProcessor
    STDMETHODIMP ActivateEx(ITfThreadMgr *pThreadMgr, TfClientId tfClientId, DWORD dwFlags);
    STDMETHODIMP Activate(ITfThreadMgr *pThreadMgr, TfClientId tfClientId);
    STDMETHODIMP Deactivate();

    // ITfThreadMgrEventSink
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr *pDocMgr);
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr *pDocMgr);
    STDMETHODIMP OnSetFocus(ITfDocumentMgr *pDocMgrFocus, ITfDocumentMgr *pDocMgrPrevFocus);
    STDMETHODIMP OnPushContext(ITfContext *pContext);
    STDMETHODIMP OnPopContext(ITfContext *pContext);

    // ITfTextEditSink
    STDMETHODIMP OnEndEdit(ITfContext *pContext, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord);

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL fForeground);
    STDMETHODIMP OnTestKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnTestKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnPreservedKey(ITfContext *pContext, REFGUID rguid, BOOL *pfEaten);

    // ITfCompositionSink
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition);

    // ITfDisplayAttributeProvider
    STDMETHODIMP EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo **ppEnum);
    STDMETHODIMP GetDisplayAttributeInfo(REFGUID guidInfo, ITfDisplayAttributeInfo **ppInfo);

    // CClassFactory factory callback
    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);

    ITfThreadMgr *_GetThreadMgr() { return _pThreadMgr; }

    // utility function for compartment
    BOOL _IsKeyboardDisabled();
    BOOL _IsKeyboardOpen();
    HRESULT _SetKeyboardOpen(BOOL fOpen);

    // functions for the composition object.
    HRESULT _StartComposition(ITfContext *pContext);
    void _EndComposition(ITfContext *pContext);
    void _TerminateComposition(TfEditCookie ec, ITfContext *pContext);
    BOOL _IsComposing();
    void _SetComposition(ITfComposition *pComposition);

    // key event handlers.
    HRESULT _HandleKey(TfEditCookie ec, ITfContext *pContext, UINT wVirtKey, UINT wScanCode);
    HRESULT _HandleReturnKey(TfEditCookie ec, ITfContext *pContext);
    HRESULT _HandleEscapeKey(TfEditCookie ec, ITfContext *pContext);
    HRESULT _HandleBackspaceKey(TfEditCookie ec, ITfContext *pContext);
    HRESULT _InvokeKeyHandler(ITfContext *pContext, WPARAM wParam, LPARAM lParam);

    void _ClearCompositionDisplayAttributes(TfEditCookie ec, ITfContext *pContext);
    BOOL _SetCompositionDisplayAttributes(TfEditCookie ec, ITfContext *pContext, TfGuidAtom gaDisplayAttribute);
    BOOL _InitDisplayAttributeGuidAtom();

private:
    // initialize and uninitialize ThreadMgrEventSink.
    BOOL _InitThreadMgrEventSink();
    void _UninitThreadMgrEventSink();

    // initialize TextEditSink.
    BOOL _InitTextEditSink(ITfDocumentMgr *pDocMgr);

    // initialize and uninitialize LanguageBar Item.
    BOOL _InitLanguageBar();
    void _UninitLanguageBar();

    // initialize and uninitialize KeyEventSink.
    BOOL _InitKeyEventSink();
    void _UninitKeyEventSink();

    // initialize the keyboard.
    BOOL _InitKeyboard();

    // utility function for KeyEventSink
    BOOL _IsKeyEaten(WPARAM wParam, LPARAM lParam);

    // Functions for the composition object
    void _CleanupComposition(TfEditCookie ec, ITfContext *pContext,
        BOOL fEndComposition);

    // Utility functions to insert characters
    BOOL _IsKeyInsertable(WPARAM wVirtKey);
    HRESULT _HandleCharacter(TfEditCookie ec, ITfContext *pContext,
        WCHAR ch);
    HRESULT _InsertCharacter(WCHAR ch, TfEditCookie ec, ITfContext *pContext);

    // Utility functions to handle Pinyin characters
    int _LookupChar(WCHAR ch, WCHAR *vowels, int cbVowels);
    BOOL _IsPinyinCharacter(WCHAR ch);
    void _FindLastVowels(WCHAR* buffer, int cbBuffer, WCHAR** ppVowelFirst, WCHAR** ppVowelLast);
    void _RemoveTone(WCHAR* pVowelFirst, WCHAR* pVowelLast);
    void _SetTone(WCHAR* pVowelFirst, WCHAR* pVowelLast, WCHAR ch);
    HRESULT _ReplaceCompositionText(WCHAR* buffer, ULONG cbBuffer,
        ITfRange* pRangeComposition, TfEditCookie ec, ITfContext *pContext);
    HRESULT _SetTone(WCHAR ch, TfEditCookie ec, ITfContext *pContext);
    HRESULT _HandleVCharacter(WCHAR ch, TfEditCookie ec, ITfContext *pContext);

    //
    // state
    //
    ITfThreadMgr *_pThreadMgr;
    TfClientId _tfClientId;

    // The cookie of ThreadMgrEventSink
    DWORD _dwThreadMgrEventSinkCookie;

    //
    // private variables for TextEditSink
    //
    ITfContext   *_pTextEditSinkContext;
    DWORD _dwTextEditSinkCookie;

    CLangBarItemButton *_pLangBarItem;

    // the current composition object.
    ITfComposition *_pComposition;

    // guidatom for the display attibute.
    TfGuidAtom _gaDisplayAttributeInput;
    TfGuidAtom _gaDisplayAttributeConverted;

    LONG _cRef;     // COM ref count
};


#endif // TEXTSERVICE_H
