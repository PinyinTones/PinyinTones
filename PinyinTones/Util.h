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

#pragma once

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

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}