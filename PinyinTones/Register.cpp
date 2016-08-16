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


#include <windows.h>
#include <ole2.h>
#include "msctf.h"
#include "globals.h"

#define CLSID_STRLEN 38  // strlen("{xxxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxx}")

// Since we target multiple Windows versions, we must conditionally define
// the Windows 8 GUIDs.
#ifndef TF_TMF_IMMERSIVEMODE
    const GUID GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT =
        { 0x13a016df, 0x560b, 0x46cd, { 0x94, 0x7a, 0x4c, 0x3a, 0xf1, 0xe0, 0xe3, 0x5d } };
#endif

static const TCHAR c_szInfoKeyPrefix[] = TEXT("CLSID\\");
static const TCHAR c_szInProcSvr32[] = TEXT("InProcServer32");
static const TCHAR c_szModelName[] = TEXT("ThreadingModel");

//+---------------------------------------------------------------------------
//
// GetLangId
//
// Gets the language ID that this text service should be registered under.
//
//----------------------------------------------------------------------------

LANGID GetLangId()
{
    // Default to Japanese, in order to workaround bug in Microsoft Word,
    // in which toned vowels appear in an East Asian font even when the
    // characters exist in the current font.
    LANGID defaultLanguageId = MAKELANGID(LANG_JAPANESE, SUBLANG_JAPANESE_JAPAN);

    HRESULT hr = S_OK;
    DWORD dwLanguageId = defaultLanguageId;

    // The 32-bit and 64-bit DLLs should both read the same regkey
    HKEY hKeyPinyinTones;
    hr = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\PinyinTones",
        0,
        KEY_WOW64_64KEY | KEY_READ,
        &hKeyPinyinTones);
    EXIT_IF_FAILED(hr);

    // Read alternative language ID from registry.
    DWORD cbData = sizeof(DWORD);
    hr = RegGetValueW(
        hKeyPinyinTones,
        NULL,
        L"TSFLanguage",
        RRF_RT_REG_DWORD,
        NULL,
        &dwLanguageId,
        &cbData);
    if (FAILED(hr) || !dwLanguageId)
    {
        dwLanguageId = defaultLanguageId;
    }

Exit:
    return (LANGID)dwLanguageId;
}


BOOL RegisterProfiles()
{
    HRESULT hr;

    ///////////////////////
    // Get TSF interfaces
    ///////////////////////

    ITfInputProcessorProfiles *pProfiles;
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfiles, (void**)&pProfiles);
    if (hr != S_OK)
        goto Exit;

    ITfInputProcessorProfileMgr *pProfileMgr;
    hr = pProfiles->QueryInterface(IID_ITfInputProcessorProfileMgr, (void**)&pProfileMgr);
    if (hr != S_OK)
        goto Exit;

    ////////////////////////////////////
    // Collect registration parameters
    ////////////////////////////////////

    // Get icon to be displayed in the language bar
    WCHAR achIconFile[MAX_PATH];
    char achFileNameA[MAX_PATH];
    DWORD cchA = GetModuleFileNameA(g_hInst, achFileNameA, ARRAYSIZE(achFileNameA));
    int cchIconFile = MultiByteToWideChar(CP_ACP, 0, achFileNameA, cchA, achIconFile,
                                          ARRAYSIZE(achIconFile) - 1);
    achIconFile[cchIconFile] = '\0';

    // Get language in which to register the text service
    LANGID languageId = GetLangId();

    //////////////////////////
    // Register text service
    //////////////////////////

    hr = pProfileMgr->RegisterProfile(
        c_clsidTextService,
        languageId,
        c_guidProfile,
        TEXTSERVICE_DESC,
        (ULONG)wcslen(TEXTSERVICE_DESC),
        achIconFile,
        cchIconFile,
        TEXTSERVICE_ICON_INDEX,
        0,
        0,
        TRUE,
        0);
    if (hr != S_OK)
        goto Exit;

Exit:
    SafeRelease(&pProfileMgr);
    SafeRelease(&pProfiles);
    return (hr == S_OK);
}

//+---------------------------------------------------------------------------
//
//  UnregisterProfiles
//
//----------------------------------------------------------------------------

void UnregisterProfiles()
{
    ITfInputProcessorProfiles *pInputProcessProfiles;
    HRESULT hr;

    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER,
                          IID_ITfInputProcessorProfiles, (void**)&pInputProcessProfiles);
    if (hr != S_OK)
        return;

    pInputProcessProfiles->Unregister(c_clsidTextService);
    SafeRelease(&pInputProcessProfiles);
}

//+---------------------------------------------------------------------------
//
//  RegisterCategories
//
//----------------------------------------------------------------------------

BOOL RegisterCategories()
{
    ITfCategoryMgr *pCategoryMgr;
    HRESULT hr;

    hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, 
                          IID_ITfCategoryMgr, (void**)&pCategoryMgr);
    if (hr != S_OK)
        goto Exit;

    // Register as a keyboard.
    hr = pCategoryMgr->RegisterCategory(c_clsidTextService,
                                        GUID_TFCAT_TIP_KEYBOARD, 
                                        c_clsidTextService);
    if (hr != S_OK)
        goto Exit;

    // Register as a display attribute provider.
    hr = pCategoryMgr->RegisterCategory(c_clsidTextService,
                                        GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER, 
                                        c_clsidTextService);
    if (hr != S_OK)
        goto Exit;
    
    // Indicate support for Windows 8 immersive mode.
    hr = pCategoryMgr->RegisterCategory(c_clsidTextService,
                                        GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
                                        c_clsidTextService);
    if (hr != S_OK)
    {
      // We may be running on an older version of Windows, so a failure should
      // be ignored.  Reset hr so that this function returns success.
      hr = S_OK;
    }

Exit:
    SafeRelease(&pCategoryMgr);
    return (hr == S_OK);
}

//+---------------------------------------------------------------------------
//
//  UnregisterCategories
//
//----------------------------------------------------------------------------

void UnregisterCategories()
{
    ITfCategoryMgr *pCategoryMgr;
    HRESULT hr;

    hr = CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, 
                          IID_ITfCategoryMgr, (void**)&pCategoryMgr);

    if (hr != S_OK)
        return;

    //
    // unregister this text service from GUID_TFCAT_TIP_KEYBOARD category.
    //
    pCategoryMgr->UnregisterCategory(c_clsidTextService,
                                     GUID_TFCAT_TIP_KEYBOARD, 
                                     c_clsidTextService);

    //
    // unregister this text service from GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER category.
    //
    pCategoryMgr->UnregisterCategory(c_clsidTextService,
                                     GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER, 
                                     c_clsidTextService);

    SafeRelease(&pCategoryMgr);
    return;
}

//+---------------------------------------------------------------------------
//
// CLSIDToStringA
//
//----------------------------------------------------------------------------

BOOL CLSIDToStringA(REFGUID refGUID, char *pchA)
{
    static const BYTE GuidMap[] = {3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
                                   8, 9, '-', 10, 11, 12, 13, 14, 15};

    static const char szDigits[] = "0123456789ABCDEF";

    int i;
    char *p = pchA;

    const BYTE * pBytes = (const BYTE *) &refGUID;

    *p++ = '{';
    for (i = 0; i < sizeof(GuidMap); i++)
    {
        if (GuidMap[i] == '-')
        {
            *p++ = '-';
        }
        else
        {
            *p++ = szDigits[ (pBytes[GuidMap[i]] & 0xF0) >> 4 ];
            *p++ = szDigits[ (pBytes[GuidMap[i]] & 0x0F) ];
        }
    }

    *p++ = '}';
    *p   = '\0';

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// RecurseDeleteKey
//
// RecurseDeleteKey is necessary because on NT RegDeleteKey doesn't work if the
// specified key has subkeys
//----------------------------------------------------------------------------
LONG RecurseDeleteKey(HKEY hParentKey, LPCTSTR lpszKey)
{
    HKEY hKey;
    LONG lRes;
    FILETIME time;
    TCHAR szBuffer[256];
    DWORD dwSize = ARRAYSIZE(szBuffer);

    if (RegOpenKey(hParentKey, lpszKey, &hKey) != ERROR_SUCCESS)
        return ERROR_SUCCESS; // Assume it couldn't be opened because it's not there

    lRes = ERROR_SUCCESS;
    while (RegEnumKeyEx(hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL, &time)==ERROR_SUCCESS)
    {
        szBuffer[ARRAYSIZE(szBuffer)-1] = '\0';
        lRes = RecurseDeleteKey(hKey, szBuffer);
        if (lRes != ERROR_SUCCESS)
            break;
        dwSize = ARRAYSIZE(szBuffer);
    }
    RegCloseKey(hKey);

    return lRes == ERROR_SUCCESS ? RegDeleteKey(hParentKey, lpszKey) : lRes;
}

//+---------------------------------------------------------------------------
//
//  RegisterServer
//
//----------------------------------------------------------------------------

BOOL RegisterServer()
{
    DWORD dw;
    HKEY hKey;
    HKEY hSubKey;
    BOOL fRet;
    TCHAR achIMEKey[ARRAYSIZE(c_szInfoKeyPrefix) + CLSID_STRLEN];
    TCHAR achFileName[MAX_PATH];

    if (!CLSIDToStringA(c_clsidTextService, achIMEKey + ARRAYSIZE(c_szInfoKeyPrefix) - 1))
        return FALSE;
    memcpy(achIMEKey, c_szInfoKeyPrefix, sizeof(c_szInfoKeyPrefix)-sizeof(TCHAR));

    if (fRet = RegCreateKeyEx(HKEY_CLASSES_ROOT, achIMEKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dw)
            == ERROR_SUCCESS)
    {
        fRet &= RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *)TEXTSERVICE_DESC_A, (lstrlen(TEXTSERVICE_DESC_A)+1)*sizeof(TCHAR))
            == ERROR_SUCCESS;

        if (fRet &= RegCreateKeyEx(hKey, c_szInProcSvr32, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hSubKey, &dw)
            == ERROR_SUCCESS)
        {
            dw = GetModuleFileNameA(g_hInst, achFileName, ARRAYSIZE(achFileName));

            fRet &= RegSetValueEx(hSubKey, NULL, 0, REG_SZ, (BYTE *)achFileName, (lstrlen(achFileName)+1)*sizeof(TCHAR)) == ERROR_SUCCESS;
            fRet &= RegSetValueEx(hSubKey, c_szModelName, 0, REG_SZ, (BYTE *)TEXTSERVICE_MODEL, (lstrlen(TEXTSERVICE_MODEL)+1)*sizeof(TCHAR)) == ERROR_SUCCESS;
            RegCloseKey(hSubKey);
        }
        RegCloseKey(hKey);
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  UnregisterServer
//
//----------------------------------------------------------------------------

void UnregisterServer()
{
    TCHAR achIMEKey[ARRAYSIZE(c_szInfoKeyPrefix) + CLSID_STRLEN];

    if (!CLSIDToStringA(c_clsidTextService, achIMEKey + ARRAYSIZE(c_szInfoKeyPrefix) - 1))
        return;
    memcpy(achIMEKey, c_szInfoKeyPrefix, sizeof(c_szInfoKeyPrefix)-sizeof(TCHAR));

    RecurseDeleteKey(HKEY_CLASSES_ROOT, achIMEKey);
}
