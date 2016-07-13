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


#ifndef PINYIN_H
#define PINYIN_H

namespace PinyinVowels
{
    static const WCHAR uu0 = 0x00FC;
    static const WCHAR UU0 = 0x00DC;

    static const int NUM_VOWELS = 60;
    static const WCHAR VOWELS[NUM_VOWELS] =
    {
        'a', 0x0101, 0x00E1, 0x01CE, 0x00E0,
        'e', 0x0113, 0x00E9, 0x011B, 0x00E8,
        'i', 0x012B, 0x00ED, 0x01D0, 0x00EC,
        'o', 0x014D, 0x00F3, 0x01D2, 0x00F2,
        'u', 0x016B, 0x00FA, 0x01D4, 0x00F9,
        uu0, 0x01D6, 0x01D8, 0x01DA, 0x01DC,
        'A', 0x0100, 0x00C1, 0x01CD, 0x00C0,
        'E', 0x0112, 0x00C9, 0x011A, 0x00C8,
        'I', 0x012A, 0x00CD, 0x01CF, 0x00CC,
        'O', 0x014C, 0x00D3, 0x01D1, 0x00D2,
        'U', 0x016A, 0x00DA, 0x01D3, 0x00D9,
        UU0, 0x01D5, 0x01D7, 0x01D9, 0x01DB
    };
}

#endif