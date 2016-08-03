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


#ifndef GLOBALS_H
#define GLOBALS_H

#include <windows.h>
#include <ole2.h>
#include <olectl.h>
#include <assert.h>
#include "msctf.h"

void DllAddRef();
void DllRelease();

#define TEXTSERVICE_DESC    L"PinyinTones"
#define TEXTSERVICE_DESC_A   "PinyinTones Text Service"
#define TEXTSERVICE_MODEL   TEXT("Apartment")

#define TEXTSERVICE_ICON_INDEX  1
#define LANGBAR_ITEM_DESC   L"Options"

extern HINSTANCE g_hInst;

extern LONG g_cRefDll;

extern CRITICAL_SECTION g_cs;

extern const CLSID c_clsidTextService;

extern const GUID c_guidProfile;

extern const GUID c_guidLangBarIcon;
extern const GUID c_guidLangBarItemButton;

extern const GUID c_guidDisplayAttributeInput;

#define EXIT_IF_FAILED(hr) if (FAILED(hr)) { goto Exit; }
#define EXIT_IF(f) if (f) { goto Exit; }
#define EXIT_IF_FAILED_WITH(hr, hrReturnValue) if (FAILED(hr))\
{\
    hr = hrReturnValue;\
    goto Exit;\
}
#define EXIT_IF_WITH(f, hrReturnValue) if (f)\
{\
    hr = hrReturnValue;\
    goto Exit;\
}

// Workaround for old versions of the Microsoft C++ compiler that lack nullptr
#if (_MSC_VER < 1600)
    #define nullptr 0
#endif

#endif // GLOBALS_H
