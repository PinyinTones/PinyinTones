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


#include "globals.h"

HINSTANCE g_hInst;

LONG g_cRefDll = -1; // -1 /w no refs, for win95 InterlockedIncrement/Decrement compat

CRITICAL_SECTION g_cs;

// {76D39ACA-FAEB-4325-A4CA-C7708E860FFC}
static const GUID c_clsidTextService = 
{ 0x76d39aca, 0xfaeb, 0x4325, { 0xa4, 0xca, 0xc7, 0x70, 0x8e, 0x86, 0xf, 0xfc } };

// {ECFF7A8F-0981-4019-9A37-BF21FA02AAB0}
static const GUID c_guidProfile = 
{ 0xecff7a8f, 0x981, 0x4019, { 0x9a, 0x37, 0xbf, 0x21, 0xfa, 0x2, 0xaa, 0xb0 } };


// {66370D8A-4025-48B4-A846-37071BE622E9}
static const GUID c_guidLangBarIcon = 
{ 0x66370d8a, 0x4025, 0x48b4, { 0xa8, 0x46, 0x37, 0x7, 0x1b, 0xe6, 0x22, 0xe9 } };

// {5C195FAD-B0AA-42B0-8450-72B326CF0362}
static const GUID c_guidLangBarItemButton = 
{ 0x5c195fad, 0xb0aa, 0x42b0, { 0x84, 0x50, 0x72, 0xb3, 0x26, 0xcf, 0x3, 0x62 } };



//
//  GUIDs for display attribute info.
//

// {694C946D-AD19-4E34-9562-8A82F7A2371D}
static const GUID c_guidDisplayAttributeInput = 
{ 0x694c946d, 0xad19, 0x4e34, { 0x95, 0x62, 0x8a, 0x82, 0xf7, 0xa2, 0x37, 0x1d } };
