/*
 *  ============================================================
 *   MasonBinder v2.0 - Professional File Binding Utility
 *   Copyright (c) 2026 MasonGroup. All Rights Reserved.
 *
 *   Developed by MasonGroup Engineering Division
 *   Licensed under MasonGroup Proprietary License (MGPL)
 *  ============================================================
 */

#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")
#endif

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MasonStubData.h"

#define MASON_MAGIC              0x424E534D
#define MASON_VERSION            0x00020000
#define MASON_MAX_FILES          512
#define MASON_MAX_FILEPATH       520
#define MASON_CRC_POLY           0xEDB88320UL
#define MASON_WINDOW_W           760
#define MASON_WINDOW_H           520
#define MASON_TITLEBAR_H         34
#define MASON_STUB_MAGIC         0x53424E4D

#define MASON_FL_COMPRESS        0x0001

#define MASON_STARTUP_NONE       0
#define MASON_STARTUP_REGISTRY   1
#define MASON_STARTUP_SCHEDULER  2

#define MASON_DROP_APPDATA       0
#define MASON_DROP_TEMP          1
#define MASON_DROP_CURRENT       2
#define MASON_DROP_CUSTOM        3

#define MASON_EXEC_RUNNING       0
#define MASON_EXEC_RUNONCE       1
#define MASON_EXEC_NORUN         2

#define MASON_ID_LV              2001
#define MASON_ID_BTN_BUILDEXE    2003
#define MASON_ID_CHK_COMP        2004
#define MASON_ID_CHK_HIDDEN      2008
#define MASON_ID_CMB_DROP        2009
#define MASON_ID_STATUSBAR       2010
#define MASON_ID_PROGRESS        2011
#define MASON_ID_LBL_TOTALSIZE   2012
#define MASON_ID_CHK_UAC         2015
#define MASON_ID_CHK_PUMP        2016
#define MASON_ID_PUMP_EDIT       2017
#define MASON_ID_BTN_ICON        2018
#define MASON_ID_LBL_ICON        2019
#define MASON_ID_CHK_SELFDEL     2020

#define MASON_CTX_REMOVE         3001
#define MASON_CTX_MOVEUP         3002
#define MASON_CTX_MOVEDOWN       3003
#define MASON_CTX_CLEAR          3004

#define MASON_CTX_DROP_APPDATA   3010
#define MASON_CTX_DROP_TEMP      3011
#define MASON_CTX_DROP_CURRENT   3012
#define MASON_CTX_DROP_CUSTOM    3013
#define MASON_CTX_START_REG      3020
#define MASON_CTX_START_SCH      3021
#define MASON_CTX_START_NONE     3022
#define MASON_CTX_EXEC_RUNNING   3030
#define MASON_CTX_EXEC_RUNONCE   3031
#define MASON_CTX_EXEC_NORUN     3032
#define MASON_CTX_HIDDEN_ON      3040
#define MASON_CTX_HIDDEN_OFF     3041
/* delete context removed - self-delete is now on the output EXE */
#define MASON_CTX_SLEEP          3060
#define MASON_CTX_RESET          3070
#define MASON_CTX_ABOUT          3080

#define MASON_CTX_ADD_FILES      3100

#define MASON_CLR_BG             RGB(248, 246, 242)
#define MASON_CLR_TEXT           RGB(40, 38, 35)
#define MASON_CLR_BORDER         RGB(210, 205, 198)
#define MASON_CLR_BTN            RGB(235, 232, 228)
#define MASON_CLR_LISTBG         RGB(255, 254, 252)
#define MASON_CLR_TITLEBAR       RGB(235, 232, 226)
#define MASON_CLR_TITLETXT       RGB(70, 60, 48)
#define MASON_CLR_TITLEBTN       RGB(100, 90, 75)
#define MASON_CLR_TITLEBTNHOV    RGB(220, 100, 100)

#pragma pack(push, 1)
typedef struct {
    DWORD MasonMagic;
    DWORD MasonVersion;
    DWORD MasonFileCount;
    DWORD MasonFlags;
    DWORD MasonReserved1;
    DWORD MasonTotalOrigSize;
    DWORD MasonTotalCompSize;
    DWORD MasonCreationTimeLow;
    DWORD MasonCreationTimeHigh;
    BYTE  MasonReserved[28];
} MASON_ARCHIVE_HEADER;

typedef struct {
    char  MasonFileName[MASON_MAX_FILEPATH];
    DWORD MasonOriginalSize;
    DWORD MasonCompressedSize;
    DWORD MasonDataOffset;
    DWORD MasonChecksum;
    DWORD MasonPriority;
    DWORD MasonFileAttributes;
    DWORD MasonEntryFlags;
    DWORD MasonStartupType;
    DWORD MasonExecMode;
    DWORD MasonHiddenFlag;
    DWORD MasonDeleteFlag;
    DWORD MasonSleepMs;
    char  MasonCustomDrop[MASON_MAX_FILEPATH];
    BYTE  MasonEntryReserved[8];
} MASON_FILE_ENTRY;

typedef struct {
    DWORD MasonArchiveOffset;
    char  MasonDropPath[MASON_MAX_FILEPATH];
    DWORD MasonAutoExec;
    DWORD MasonStubFlags;
    DWORD MasonSelfDelete;
    DWORD MasonStubMagic;
} MASON_STUB_FOOTER;
#pragma pack(pop)

typedef struct {
    char  MasonPath[MASON_MAX_FILEPATH];
    char  MasonName[260];
    char  MasonDropDir[MASON_MAX_FILEPATH];
    DWORD MasonSize;
    DWORD MasonPriority;
    DWORD MasonAttributes;
    DWORD MasonCRC;
    DWORD MasonStartup;
    DWORD MasonExecMode;
    BOOL  MasonHidden;
    DWORD MasonSleepMs;
} MASON_LIST_ITEM;

static HINSTANCE       MasonAppInstance;
static HWND            MasonMainWindow;
static HWND            MasonListView;
static HWND            MasonStatusBar;
static HWND            MasonProgressBar;
static HWND            MasonChkCompress;
static HWND            MasonChkHidden;
static HWND            MasonChkUAC;
static HWND            MasonChkPump;
static HWND            MasonChkSelfDel;
static HWND            MasonPumpEdit;
static HWND            MasonCmbDrop;
static HWND            MasonLblTotal;
static char            MasonCustomDropPath[MASON_MAX_FILEPATH];
static char            MasonIconSourcePath[MASON_MAX_FILEPATH];

static MASON_LIST_ITEM MasonFileList[MASON_MAX_FILES];
static int             MasonFileCount = 0;
static HFONT           MasonFontNormal;
static HFONT           MasonFontBold;
static HFONT           MasonFontMono;
static HBRUSH          MasonBrushBg;
static HBRUSH          MasonBrushListBg;
static HBRUSH          MasonBrushBtn;
static HBRUSH          MasonBrushTitleBar;
static DWORD           MasonCrcTable[256];
static BOOL            MasonCrcTableReady = FALSE;
static BOOL            MasonOperationActive = FALSE;
static BOOL            MasonDraggingTitle = FALSE;
static POINT           MasonDragStart;
static int             MasonHoverCloseBtn = 0;

static const char MasonCopyright[] =
    "MasonBinder v2.0 - (c) 2026 MasonGroup\r\n"
    "All Rights Reserved.\r\n\r\n"
    "Professional File Binding & Archiving Utility\r\n"
    "Developed by MasonGroup Engineering Division\r\n\r\n"
    "Features:\r\n"
    "  - Self-extracting EXE builder\r\n"
    "  - Per-file drop paths & execution modes\r\n"
    "  - Startup persistence (Registry / Scheduler)\r\n"
    "  - RLE Compression engine\r\n"
    "  - Hidden extraction mode\r\n"
    "  - Icon & description cloning\r\n"
    "  - UAC manifest injection\r\n"
    "  - File size pump\r\n\r\n"
    "Licensed under MasonGroup Proprietary License";

void MasonInitCrcTable(void)
{
    DWORD MasonCrc, MasonIdx, MasonBit;
    if (MasonCrcTableReady) return;
    for (MasonIdx = 0; MasonIdx < 256; MasonIdx++) {
        MasonCrc = MasonIdx;
        for (MasonBit = 0; MasonBit < 8; MasonBit++) {
            if (MasonCrc & 1) MasonCrc = (MasonCrc >> 1) ^ MASON_CRC_POLY;
            else MasonCrc >>= 1;
        }
        MasonCrcTable[MasonIdx] = MasonCrc;
    }
    MasonCrcTableReady = TRUE;
}

DWORD MasonCalcCrc32(const BYTE *MasonData, DWORD MasonLen)
{
    DWORD MasonCrc = 0xFFFFFFFF, MasonPos;
    MasonInitCrcTable();
    for (MasonPos = 0; MasonPos < MasonLen; MasonPos++)
        MasonCrc = MasonCrcTable[(MasonCrc ^ MasonData[MasonPos]) & 0xFF] ^ (MasonCrc >> 8);
    return MasonCrc ^ 0xFFFFFFFF;
}

DWORD MasonCompressRLE(const BYTE *MasonSrc, DWORD MasonSrcLen, BYTE *MasonDst, DWORD MasonDstCap)
{
    DWORD MasonSrcPos = 0, MasonDstPos = 0, MasonRunLen, MasonLitLen;
    if (MasonDstCap < 8) return 0;
    while (MasonSrcPos < MasonSrcLen && MasonDstPos + 4 < MasonDstCap) {
        MasonRunLen = 1;
        while (MasonSrcPos + MasonRunLen < MasonSrcLen &&
               MasonSrc[MasonSrcPos] == MasonSrc[MasonSrcPos + MasonRunLen] &&
               MasonRunLen < 127) MasonRunLen++;
        if (MasonRunLen >= 3) {
            if (MasonDstPos + 2 > MasonDstCap) return 0;
            MasonDst[MasonDstPos++] = (BYTE)(0x80 | MasonRunLen);
            MasonDst[MasonDstPos++] = MasonSrc[MasonSrcPos];
            MasonSrcPos += MasonRunLen;
        } else {
            MasonLitLen = 0;
            while (MasonSrcPos + MasonLitLen < MasonSrcLen && MasonLitLen < 127) {
                if (MasonSrcPos + MasonLitLen + 2 < MasonSrcLen &&
                    MasonSrc[MasonSrcPos + MasonLitLen] == MasonSrc[MasonSrcPos + MasonLitLen + 1] &&
                    MasonSrc[MasonSrcPos + MasonLitLen] == MasonSrc[MasonSrcPos + MasonLitLen + 2]) break;
                MasonLitLen++;
            }
            if (MasonLitLen == 0) MasonLitLen = 1;
            if (MasonDstPos + 1 + MasonLitLen > MasonDstCap) return 0;
            MasonDst[MasonDstPos++] = (BYTE)MasonLitLen;
            memcpy(MasonDst + MasonDstPos, MasonSrc + MasonSrcPos, MasonLitLen);
            MasonDstPos += MasonLitLen;
            MasonSrcPos += MasonLitLen;
        }
    }
    return (MasonSrcPos == MasonSrcLen) ? MasonDstPos : 0;
}

void MasonFormatSize(DWORD MasonBytes, char *MasonBuf, int MasonBufLen)
{
    if (MasonBytes >= 1073741824)
        _snprintf(MasonBuf, MasonBufLen, "%.2f GB", MasonBytes / 1073741824.0);
    else if (MasonBytes >= 1048576)
        _snprintf(MasonBuf, MasonBufLen, "%.2f MB", MasonBytes / 1048576.0);
    else if (MasonBytes >= 1024)
        _snprintf(MasonBuf, MasonBufLen, "%.1f KB", MasonBytes / 1024.0);
    else
        _snprintf(MasonBuf, MasonBufLen, "%lu B", (unsigned long)MasonBytes);
}

const char *MasonExecModeName(DWORD MasonMode)
{
    switch (MasonMode) {
        case MASON_EXEC_RUNNING: return "Running";
        case MASON_EXEC_RUNONCE: return "Runonce";
        case MASON_EXEC_NORUN:   return "No Run";
    }
    return "Running";
}

const char *MasonStartupName(DWORD MasonType)
{
    switch (MasonType) {
        case MASON_STARTUP_REGISTRY:  return "Registry";
        case MASON_STARTUP_SCHEDULER: return "Scheduler";
    }
    return "None";
}

void MasonExtractFileName(const char *MasonFullPath, char *MasonName, int MasonNameLen)
{
    const char *MasonSlash, *MasonBack;
    MasonSlash = strrchr(MasonFullPath, '/');
    MasonBack = strrchr(MasonFullPath, '\\');
    if (MasonBack && (!MasonSlash || MasonBack > MasonSlash)) MasonSlash = MasonBack;
    if (MasonSlash) _snprintf(MasonName, MasonNameLen, "%s", MasonSlash + 1);
    else _snprintf(MasonName, MasonNameLen, "%s", MasonFullPath);
}

void MasonSetStatus(const char *MasonMsg)
{
    if (MasonStatusBar) SendMessageA(MasonStatusBar, SB_SETTEXTA, 0, (LPARAM)MasonMsg);
}

void MasonSetProgress(int MasonPercent)
{
    if (MasonProgressBar) SendMessageA(MasonProgressBar, PBM_SETPOS, MasonPercent, 0);
}

void MasonUpdateTotalSize(void)
{
    DWORD MasonTotal = 0;
    int MasonI;
    char MasonBuf[128], MasonSzBuf[32];
    for (MasonI = 0; MasonI < MasonFileCount; MasonI++)
        MasonTotal += MasonFileList[MasonI].MasonSize;
    MasonFormatSize(MasonTotal, MasonSzBuf, sizeof(MasonSzBuf));
    _snprintf(MasonBuf, sizeof(MasonBuf), "  Files: %d  |  Total: %s", MasonFileCount, MasonSzBuf);
    if (MasonLblTotal) SetWindowTextA(MasonLblTotal, MasonBuf);
}

void MasonRefreshListView(void)
{
    LVITEMA MasonLvi;
    char MasonSzBuf[32], MasonIdxBuf[8], MasonSleepBuf[16], MasonFlagsBuf[32];
    int MasonI;
    SendMessageA(MasonListView, LVM_DELETEALLITEMS, 0, 0);
    for (MasonI = 0; MasonI < MasonFileCount; MasonI++) {
        memset(&MasonLvi, 0, sizeof(MasonLvi));
        MasonLvi.mask = LVIF_TEXT;
        MasonLvi.iItem = MasonI;

        _snprintf(MasonIdxBuf, sizeof(MasonIdxBuf), "%d", MasonI + 1);
        MasonLvi.iSubItem = 0;
        MasonLvi.pszText = MasonIdxBuf;
        SendMessageA(MasonListView, LVM_INSERTITEMA, 0, (LPARAM)&MasonLvi);

        MasonLvi.iSubItem = 1;
        MasonLvi.pszText = MasonFileList[MasonI].MasonName;
        SendMessageA(MasonListView, LVM_SETITEMA, 0, (LPARAM)&MasonLvi);

        MasonFormatSize(MasonFileList[MasonI].MasonSize, MasonSzBuf, sizeof(MasonSzBuf));
        MasonLvi.iSubItem = 2;
        MasonLvi.pszText = MasonSzBuf;
        SendMessageA(MasonListView, LVM_SETITEMA, 0, (LPARAM)&MasonLvi);

        MasonLvi.iSubItem = 3;
        MasonLvi.pszText = (MasonFileList[MasonI].MasonDropDir[0] != '\0')
            ? MasonFileList[MasonI].MasonDropDir : (char *)"(Default)";
        SendMessageA(MasonListView, LVM_SETITEMA, 0, (LPARAM)&MasonLvi);

        MasonLvi.iSubItem = 4;
        MasonLvi.pszText = (char *)MasonExecModeName(MasonFileList[MasonI].MasonExecMode);
        SendMessageA(MasonListView, LVM_SETITEMA, 0, (LPARAM)&MasonLvi);

        MasonFlagsBuf[0] = '\0';
        if (MasonFileList[MasonI].MasonHidden) strcat(MasonFlagsBuf, "H ");
        if (MasonFileList[MasonI].MasonStartup == MASON_STARTUP_REGISTRY) strcat(MasonFlagsBuf, "Reg ");
        else if (MasonFileList[MasonI].MasonStartup == MASON_STARTUP_SCHEDULER) strcat(MasonFlagsBuf, "Sch ");
        if (MasonFlagsBuf[0] == '\0') strcpy(MasonFlagsBuf, "-");
        MasonLvi.iSubItem = 5;
        MasonLvi.pszText = MasonFlagsBuf;
        SendMessageA(MasonListView, LVM_SETITEMA, 0, (LPARAM)&MasonLvi);

        if (MasonFileList[MasonI].MasonSleepMs > 0)
            _snprintf(MasonSleepBuf, sizeof(MasonSleepBuf), "%lu ms", (unsigned long)MasonFileList[MasonI].MasonSleepMs);
        else
            strcpy(MasonSleepBuf, "-");
        MasonLvi.iSubItem = 6;
        MasonLvi.pszText = MasonSleepBuf;
        SendMessageA(MasonListView, LVM_SETITEMA, 0, (LPARAM)&MasonLvi);
    }
    MasonUpdateTotalSize();
}

BOOL MasonCheckDuplicate(const char *MasonPath)
{
    int MasonI;
    for (MasonI = 0; MasonI < MasonFileCount; MasonI++)
        if (_stricmp(MasonFileList[MasonI].MasonPath, MasonPath) == 0) return TRUE;
    return FALSE;
}

void MasonAddFilesToList(void)
{
    OPENFILENAMEA MasonOfn;
    char MasonBuf[4096], MasonDir[MASON_MAX_FILEPATH], *MasonPtr;
    int MasonAdded = 0;
    memset(MasonBuf, 0, sizeof(MasonBuf));
    memset(&MasonOfn, 0, sizeof(MasonOfn));
    MasonOfn.lStructSize = sizeof(MasonOfn);
    MasonOfn.hwndOwner = MasonMainWindow;
    MasonOfn.lpstrFilter = "All Files (*.*)\0*.*\0Executables (*.exe)\0*.exe\0";
    MasonOfn.lpstrFile = MasonBuf;
    MasonOfn.nMaxFile = sizeof(MasonBuf);
    MasonOfn.lpstrTitle = "MasonBinder - Select Files";
    MasonOfn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetOpenFileNameA(&MasonOfn)) return;
    MasonPtr = MasonBuf + strlen(MasonBuf) + 1;
    if (*MasonPtr == '\0') {
        if (MasonFileCount >= MASON_MAX_FILES || MasonCheckDuplicate(MasonBuf)) return;
        memset(&MasonFileList[MasonFileCount], 0, sizeof(MASON_LIST_ITEM));
        _snprintf(MasonFileList[MasonFileCount].MasonPath, MASON_MAX_FILEPATH - 1, "%s", MasonBuf);
        MasonExtractFileName(MasonBuf, MasonFileList[MasonFileCount].MasonName, 260);
        {
            HANDLE MasonHf = CreateFileA(MasonBuf, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
            if (MasonHf != INVALID_HANDLE_VALUE) {
                MasonFileList[MasonFileCount].MasonSize = GetFileSize(MasonHf, NULL);
                CloseHandle(MasonHf);
            }
        }
        MasonFileList[MasonFileCount].MasonExecMode = MASON_EXEC_RUNNING;
        MasonFileCount++;
        MasonAdded = 1;
    } else {
        _snprintf(MasonDir, MASON_MAX_FILEPATH - 1, "%s", MasonBuf);
        while (*MasonPtr != '\0') {
            char MasonFullPath[MASON_MAX_FILEPATH];
            if (MasonFileCount >= MASON_MAX_FILES) break;
            _snprintf(MasonFullPath, sizeof(MasonFullPath), "%s\\%s", MasonDir, MasonPtr);
            if (!MasonCheckDuplicate(MasonFullPath)) {
                memset(&MasonFileList[MasonFileCount], 0, sizeof(MASON_LIST_ITEM));
                _snprintf(MasonFileList[MasonFileCount].MasonPath, MASON_MAX_FILEPATH - 1, "%s", MasonFullPath);
                MasonExtractFileName(MasonFullPath, MasonFileList[MasonFileCount].MasonName, 260);
                {
                    HANDLE MasonHf = CreateFileA(MasonFullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
                    if (MasonHf != INVALID_HANDLE_VALUE) {
                        MasonFileList[MasonFileCount].MasonSize = GetFileSize(MasonHf, NULL);
                        CloseHandle(MasonHf);
                    }
                }
                MasonFileList[MasonFileCount].MasonExecMode = MASON_EXEC_RUNNING;
                MasonFileCount++;
                MasonAdded++;
            }
            MasonPtr += strlen(MasonPtr) + 1;
        }
    }
    if (MasonAdded > 0) {
        char MasonMsg[64];
        MasonRefreshListView();
        _snprintf(MasonMsg, sizeof(MasonMsg), "Added %d file(s)", MasonAdded);
        MasonSetStatus(MasonMsg);
    }
}

void MasonGetGlobalDropPath(char *MasonOut, int MasonOutLen)
{
    int MasonSel = (int)SendMessageA(MasonCmbDrop, CB_GETCURSEL, 0, 0);
    if (MasonSel == MASON_DROP_APPDATA)
        _snprintf(MasonOut, MasonOutLen, "%%APPDATA%%\\MasonDrop");
    else if (MasonSel == MASON_DROP_TEMP)
        _snprintf(MasonOut, MasonOutLen, "%%TEMP%%\\MasonDrop");
    else if (MasonSel == MASON_DROP_CURRENT)
        _snprintf(MasonOut, MasonOutLen, "%%CURRENT%%");
    else if (MasonSel == MASON_DROP_CUSTOM && MasonCustomDropPath[0] != '\0')
        _snprintf(MasonOut, MasonOutLen, "%s", MasonCustomDropPath);
    else
        _snprintf(MasonOut, MasonOutLen, "%%TEMP%%\\MasonDrop");
}

LRESULT CALLBACK MasonPathDlgProc(HWND MasonHwnd, UINT MasonMsg, WPARAM MasonWp, LPARAM MasonLp)
{
    static HWND MasonPathEdit = NULL;

    switch (MasonMsg) {
    case WM_CREATE:
    {
        HWND MasonLbl, MasonOkBtn;
        MasonLbl = CreateWindowExA(0, "STATIC", "Enter drop path:", WS_CHILD | WS_VISIBLE,
            10, 12, 120, 18, MasonHwnd, NULL, MasonAppInstance, NULL);
        (void)MasonLbl;
        MasonPathEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "%Current%\\Folder",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            10, 32, 380, 22, MasonHwnd, NULL, MasonAppInstance, NULL);
        MasonOkBtn = CreateWindowExA(0, "BUTTON", "OK",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            160, 62, 80, 26, MasonHwnd, (HMENU)1, MasonAppInstance, NULL);
        (void)MasonOkBtn;
        SetFocus(MasonPathEdit);
        SendMessageA(MasonPathEdit, EM_SETSEL, 0, -1);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(MasonWp) == 1) {
            GetWindowTextA(MasonPathEdit, MasonCustomDropPath, sizeof(MasonCustomDropPath));
            DestroyWindow(MasonHwnd);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (MasonWp == VK_RETURN) {
            GetWindowTextA(MasonPathEdit, MasonCustomDropPath, sizeof(MasonCustomDropPath));
            DestroyWindow(MasonHwnd);
            return 0;
        }
        if (MasonWp == VK_ESCAPE) {
            DestroyWindow(MasonHwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(MasonHwnd);
        return 0;
    case WM_DESTROY:
        EnableWindow(MasonMainWindow, TRUE);
        SetForegroundWindow(MasonMainWindow);
        MasonPathEdit = NULL;
        return 0;
    }
    return DefWindowProcA(MasonHwnd, MasonMsg, MasonWp, MasonLp);
}

void MasonAskCustomPath(void)
{
    WNDCLASSEXA MasonWc;
    HWND MasonDlg;
    MSG MasonMsg;

    memset(&MasonWc, 0, sizeof(MasonWc));
    MasonWc.cbSize = sizeof(MasonWc);
    MasonWc.lpfnWndProc = MasonPathDlgProc;
    MasonWc.hInstance = MasonAppInstance;
    MasonWc.lpszClassName = "MasonPathDlgCls";
    MasonWc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    MasonWc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExA(&MasonWc);

    EnableWindow(MasonMainWindow, FALSE);
    MasonDlg = CreateWindowExA(WS_EX_TOPMOST | WS_EX_DLGMODALFRAME,
        "MasonPathDlgCls", "MasonBinder - Custom Path",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 420, 120,
        MasonMainWindow, NULL, MasonAppInstance, NULL);

    while (IsWindow(MasonDlg) && GetMessageA(&MasonMsg, NULL, 0, 0) > 0) {
        TranslateMessage(&MasonMsg);
        DispatchMessageA(&MasonMsg);
    }
}

LRESULT CALLBACK MasonSleepDlgProc(HWND MasonHwnd, UINT MasonMsg, WPARAM MasonWp, LPARAM MasonLp)
{
    static HWND MasonSleepEdit = NULL;
    static int *MasonSleepResult = NULL;

    switch (MasonMsg) {
    case WM_CREATE:
    {
        CREATESTRUCTA *MasonCs = (CREATESTRUCTA *)MasonLp;
        HWND MasonLbl, MasonOkBtn;
        MasonSleepResult = (int *)MasonCs->lpCreateParams;
        MasonLbl = CreateWindowExA(0, "STATIC", "Sleep (ms):", WS_CHILD | WS_VISIBLE,
            10, 12, 100, 18, MasonHwnd, NULL, MasonAppInstance, NULL);
        (void)MasonLbl;
        MasonSleepEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "1000",
            WS_CHILD | WS_VISIBLE | ES_NUMBER,
            10, 32, 160, 22, MasonHwnd, NULL, MasonAppInstance, NULL);
        MasonOkBtn = CreateWindowExA(0, "BUTTON", "OK",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            60, 62, 80, 26, MasonHwnd, (HMENU)1, MasonAppInstance, NULL);
        (void)MasonOkBtn;
        SetFocus(MasonSleepEdit);
        SendMessageA(MasonSleepEdit, EM_SETSEL, 0, -1);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(MasonWp) == 1) {
            char MasonVal[16];
            GetWindowTextA(MasonSleepEdit, MasonVal, sizeof(MasonVal));
            if (MasonSleepResult) *MasonSleepResult = atoi(MasonVal);
            DestroyWindow(MasonHwnd);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (MasonWp == VK_RETURN) {
            char MasonVal[16];
            GetWindowTextA(MasonSleepEdit, MasonVal, sizeof(MasonVal));
            if (MasonSleepResult) *MasonSleepResult = atoi(MasonVal);
            DestroyWindow(MasonHwnd);
            return 0;
        }
        if (MasonWp == VK_ESCAPE) {
            DestroyWindow(MasonHwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(MasonHwnd);
        return 0;
    case WM_DESTROY:
        EnableWindow(MasonMainWindow, TRUE);
        SetForegroundWindow(MasonMainWindow);
        MasonSleepEdit = NULL;
        MasonSleepResult = NULL;
        return 0;
    }
    return DefWindowProcA(MasonHwnd, MasonMsg, MasonWp, MasonLp);
}

int MasonAskSleep(void)
{
    WNDCLASSEXA MasonWc;
    HWND MasonDlg;
    int MasonResult = 0;
    MSG MasonMsg;

    memset(&MasonWc, 0, sizeof(MasonWc));
    MasonWc.cbSize = sizeof(MasonWc);
    MasonWc.lpfnWndProc = MasonSleepDlgProc;
    MasonWc.hInstance = MasonAppInstance;
    MasonWc.lpszClassName = "MasonSleepDlgCls";
    MasonWc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    MasonWc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExA(&MasonWc);

    EnableWindow(MasonMainWindow, FALSE);
    MasonDlg = CreateWindowExA(WS_EX_TOPMOST | WS_EX_DLGMODALFRAME,
        "MasonSleepDlgCls", "MasonBinder - Sleep",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 210, 120,
        MasonMainWindow, NULL, MasonAppInstance, &MasonResult);

    while (IsWindow(MasonDlg) && GetMessageA(&MasonMsg, NULL, 0, 0) > 0) {
        TranslateMessage(&MasonMsg);
        DispatchMessageA(&MasonMsg);
    }

    return MasonResult;
}

void MasonSelectIconSource(void)
{
    OPENFILENAMEA MasonOfn;
    char MasonBuf[MASON_MAX_FILEPATH];

    memset(MasonBuf, 0, sizeof(MasonBuf));
    memset(&MasonOfn, 0, sizeof(MasonOfn));
    MasonOfn.lStructSize = sizeof(MasonOfn);
    MasonOfn.hwndOwner = MasonMainWindow;
    MasonOfn.lpstrFilter = "Icon Files (*.ico)\0*.ico\0All Files (*.*)\0*.*\0";
    MasonOfn.lpstrFile = MasonBuf;
    MasonOfn.nMaxFile = sizeof(MasonBuf);
    MasonOfn.lpstrTitle = "MasonBinder - Select Icon";
    MasonOfn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameA(&MasonOfn)) {
        _snprintf(MasonIconSourcePath, sizeof(MasonIconSourcePath) - 1, "%s", MasonBuf);
        MasonIconSourcePath[sizeof(MasonIconSourcePath) - 1] = '\0';
        {
            char MasonShortName[64];
            char MasonIconLblTxt[128];
            HWND MasonIconLblWnd;
            MasonExtractFileName(MasonBuf, MasonShortName, sizeof(MasonShortName));
            _snprintf(MasonIconLblTxt, sizeof(MasonIconLblTxt), "Icon: %s", MasonShortName);
            MasonIconLblWnd = GetDlgItem(MasonMainWindow, MASON_ID_LBL_ICON);
            if (MasonIconLblWnd) SetWindowTextA(MasonIconLblWnd, MasonIconLblTxt);
            MasonSetStatus(MasonIconLblTxt);
        }
    }
}

void MasonWriteManifest(const char *MasonExePath, BOOL MasonRequireAdmin)
{
    const char *MasonLevel = MasonRequireAdmin ? "requireAdministrator" : "asInvoker";
    char MasonManifest[1024];
    HANDLE MasonResHandle;

    _snprintf(MasonManifest, sizeof(MasonManifest),
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\r\n"
        "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">\r\n"
        "  <trustInfo xmlns=\"urn:schemas-microsoft-com:asm.v3\">\r\n"
        "    <security>\r\n"
        "      <requestedPrivileges>\r\n"
        "        <requestedExecutionLevel level=\"%s\" uiAccess=\"false\"/>\r\n"
        "      </requestedPrivileges>\r\n"
        "    </security>\r\n"
        "  </trustInfo>\r\n"
        "</assembly>\r\n", MasonLevel);

    MasonResHandle = BeginUpdateResourceA(MasonExePath, FALSE);
    if (MasonResHandle) {
        UpdateResourceA(MasonResHandle, (LPCSTR)24, MAKEINTRESOURCEA(1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
            MasonManifest, (DWORD)strlen(MasonManifest));
        EndUpdateResourceA(MasonResHandle, FALSE);
    }
}

#pragma pack(push, 1)
typedef struct {
    BYTE  MasonWidth;
    BYTE  MasonHeight;
    BYTE  MasonColorCount;
    BYTE  MasonReservedIco;
    WORD  MasonPlanes;
    WORD  MasonBitCount;
    DWORD MasonBytesInRes;
    DWORD MasonImageOffset;
} MASON_ICO_DIR_ENTRY;

typedef struct {
    WORD MasonReservedIco;
    WORD MasonType;
    WORD MasonCount;
} MASON_ICO_DIR;

typedef struct {
    BYTE  MasonWidth;
    BYTE  MasonHeight;
    BYTE  MasonColorCount;
    BYTE  MasonReservedIco;
    WORD  MasonPlanes;
    WORD  MasonBitCount;
    DWORD MasonBytesInRes;
    WORD  MasonId;
} MASON_GRPICON_ENTRY;

typedef struct {
    WORD MasonReservedIco;
    WORD MasonType;
    WORD MasonCount;
} MASON_GRPICON_DIR;
#pragma pack(pop)

void MasonEmbedIcoFile(const char *MasonIcoPath, const char *MasonDstExe)
{
    HANDLE MasonIcoFile;
    DWORD MasonIcoSize, MasonRead;
    BYTE *MasonIcoData;
    MASON_ICO_DIR MasonDir;
    MASON_ICO_DIR_ENTRY *MasonDirEntries;
    BYTE *MasonGrpBuf;
    MASON_GRPICON_DIR *MasonGrpDir;
    MASON_GRPICON_ENTRY *MasonGrpEntries;
    DWORD MasonGrpSize;
    HANDLE MasonUpd;
    int MasonIdx;

    MasonIcoFile = CreateFileA(MasonIcoPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (MasonIcoFile == INVALID_HANDLE_VALUE) return;
    MasonIcoSize = GetFileSize(MasonIcoFile, NULL);
    MasonIcoData = (BYTE *)malloc(MasonIcoSize);
    if (!MasonIcoData) { CloseHandle(MasonIcoFile); return; }
    ReadFile(MasonIcoFile, MasonIcoData, MasonIcoSize, &MasonRead, NULL);
    CloseHandle(MasonIcoFile);

    if (MasonIcoSize == 0 || MasonIcoSize == 0xFFFFFFFF) { free(MasonIcoData); return; }
    if (MasonIcoSize < sizeof(MASON_ICO_DIR)) { free(MasonIcoData); return; }
    memcpy(&MasonDir, MasonIcoData, sizeof(MASON_ICO_DIR));
    if (MasonDir.MasonType != 1 || MasonDir.MasonCount == 0 || MasonDir.MasonCount > 50) { free(MasonIcoData); return; }
    if (sizeof(MASON_ICO_DIR) + MasonDir.MasonCount * sizeof(MASON_ICO_DIR_ENTRY) > MasonIcoSize) { free(MasonIcoData); return; }

    MasonDirEntries = (MASON_ICO_DIR_ENTRY *)(MasonIcoData + sizeof(MASON_ICO_DIR));

    MasonGrpSize = sizeof(MASON_GRPICON_DIR) + MasonDir.MasonCount * sizeof(MASON_GRPICON_ENTRY);
    MasonGrpBuf = (BYTE *)calloc(1, MasonGrpSize);
    if (!MasonGrpBuf) { free(MasonIcoData); return; }

    MasonGrpDir = (MASON_GRPICON_DIR *)MasonGrpBuf;
    MasonGrpDir->MasonReservedIco = 0;
    MasonGrpDir->MasonType = 1;
    MasonGrpDir->MasonCount = MasonDir.MasonCount;
    MasonGrpEntries = (MASON_GRPICON_ENTRY *)(MasonGrpBuf + sizeof(MASON_GRPICON_DIR));

    for (MasonIdx = 0; MasonIdx < MasonDir.MasonCount; MasonIdx++) {
        MasonGrpEntries[MasonIdx].MasonWidth = MasonDirEntries[MasonIdx].MasonWidth;
        MasonGrpEntries[MasonIdx].MasonHeight = MasonDirEntries[MasonIdx].MasonHeight;
        MasonGrpEntries[MasonIdx].MasonColorCount = MasonDirEntries[MasonIdx].MasonColorCount;
        MasonGrpEntries[MasonIdx].MasonReservedIco = 0;
        MasonGrpEntries[MasonIdx].MasonPlanes = MasonDirEntries[MasonIdx].MasonPlanes;
        MasonGrpEntries[MasonIdx].MasonBitCount = MasonDirEntries[MasonIdx].MasonBitCount;
        MasonGrpEntries[MasonIdx].MasonBytesInRes = MasonDirEntries[MasonIdx].MasonBytesInRes;
        MasonGrpEntries[MasonIdx].MasonId = (WORD)(MasonIdx + 1);
    }

    MasonUpd = BeginUpdateResourceA(MasonDstExe, FALSE);
    if (MasonUpd) {
        for (MasonIdx = 0; MasonIdx < MasonDir.MasonCount; MasonIdx++) {
            BYTE *MasonImgData = MasonIcoData + MasonDirEntries[MasonIdx].MasonImageOffset;
            DWORD MasonImgSize = MasonDirEntries[MasonIdx].MasonBytesInRes;
            if (MasonDirEntries[MasonIdx].MasonImageOffset + MasonImgSize <= MasonIcoSize) {
                UpdateResourceA(MasonUpd, RT_ICON, MAKEINTRESOURCEA(MasonIdx + 1),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), MasonImgData, MasonImgSize);
            }
        }
        UpdateResourceA(MasonUpd, RT_GROUP_ICON, MAKEINTRESOURCEA(1),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), MasonGrpBuf, MasonGrpSize);
        EndUpdateResourceA(MasonUpd, FALSE);
    }

    free(MasonGrpBuf);
    free(MasonIcoData);
}


BOOL MasonBuildExe(void)
{
    char MasonOutPath[MASON_MAX_FILEPATH], MasonDropBuf[MASON_MAX_FILEPATH];
    HANDLE MasonOutFile;
    DWORD MasonWritten;
    DWORD MasonActualStubSize;
    MASON_ARCHIVE_HEADER MasonHdr;
    MASON_FILE_ENTRY *MasonEntries;
    MASON_STUB_FOOTER MasonFooter;
    DWORD MasonFlags = 0, MasonCurrentOffset, MasonTotalOrig = 0, MasonTotalComp = 0;
    int MasonI;
    BOOL MasonDoCompress;
    SYSTEMTIME MasonSysTime;
    FILETIME MasonFileTime;
    OPENFILENAMEA MasonOfn;

    if (MasonFileCount == 0) {
        MessageBoxA(MasonMainWindow, "No files to bind.", "MasonBinder", MB_ICONWARNING);
        return FALSE;
    }

    memset(MasonOutPath, 0, sizeof(MasonOutPath));
    {
        char MasonBase[260];
        _snprintf(MasonBase, sizeof(MasonBase) - 1, "%s", MasonFileList[0].MasonName);
        MasonBase[sizeof(MasonBase) - 1] = '\0';
        for (MasonI = (int)strlen(MasonBase) - 1; MasonI >= 0; MasonI--)
            if (MasonBase[MasonI] == '.') { MasonBase[MasonI] = '\0'; break; }
        _snprintf(MasonOutPath, sizeof(MasonOutPath) - 1, "%s_bound.exe", MasonBase);
    }

    memset(&MasonOfn, 0, sizeof(MasonOfn));
    MasonOfn.lStructSize = sizeof(MasonOfn);
    MasonOfn.hwndOwner = MasonMainWindow;
    MasonOfn.lpstrFilter = "Executable (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
    MasonOfn.lpstrFile = MasonOutPath;
    MasonOfn.nMaxFile = sizeof(MasonOutPath);
    MasonOfn.lpstrTitle = "Save Executable As";
    MasonOfn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    MasonOfn.lpstrDefExt = "exe";
    if (!GetSaveFileNameA(&MasonOfn)) { return FALSE; }

    MasonDoCompress = (SendMessageA(MasonChkCompress, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (MasonDoCompress) MasonFlags |= MASON_FL_COMPRESS;

    MasonEntries = (MASON_FILE_ENTRY *)calloc(MasonFileCount, sizeof(MASON_FILE_ENTRY));
    if (!MasonEntries) return FALSE;

    MasonOperationActive = TRUE;
    MasonSetStatus("Preparing stub...");
    MasonSetProgress(0);

    {
        HANDLE MasonTmpFile = CreateFileA(MasonOutPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (MasonTmpFile == INVALID_HANDLE_VALUE) { free(MasonEntries); MasonOperationActive = FALSE; return FALSE; }
        WriteFile(MasonTmpFile, MasonStubBytes, MasonStubSize, &MasonWritten, NULL);
        CloseHandle(MasonTmpFile);
    }

    if (MasonIconSourcePath[0] != '\0') {
        MasonSetStatus("Embedding icon...");
        MasonEmbedIcoFile(MasonIconSourcePath, MasonOutPath);
    }

    {
        BOOL MasonUAC = (SendMessageA(MasonChkUAC, BM_GETCHECK, 0, 0) == BST_CHECKED);
        MasonWriteManifest(MasonOutPath, MasonUAC);
    }

    {
        HANDLE MasonCheckFile = CreateFileA(MasonOutPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (MasonCheckFile == INVALID_HANDLE_VALUE) { free(MasonEntries); MasonOperationActive = FALSE; return FALSE; }
        MasonActualStubSize = GetFileSize(MasonCheckFile, NULL);
        CloseHandle(MasonCheckFile);
    }

    MasonOutFile = CreateFileA(MasonOutPath, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (MasonOutFile == INVALID_HANDLE_VALUE) { free(MasonEntries); MasonOperationActive = FALSE; return FALSE; }

    MasonSetStatus("Building executable...");

    memset(&MasonHdr, 0, sizeof(MasonHdr));
    MasonHdr.MasonMagic = MASON_MAGIC;
    MasonHdr.MasonVersion = MASON_VERSION;
    MasonHdr.MasonFileCount = (DWORD)MasonFileCount;
    MasonHdr.MasonFlags = MasonFlags;
    GetSystemTime(&MasonSysTime);
    SystemTimeToFileTime(&MasonSysTime, &MasonFileTime);
    MasonHdr.MasonCreationTimeLow = MasonFileTime.dwLowDateTime;
    MasonHdr.MasonCreationTimeHigh = MasonFileTime.dwHighDateTime;

    SetFilePointer(MasonOutFile, MasonActualStubSize + sizeof(MASON_ARCHIVE_HEADER) + ((DWORD)MasonFileCount * sizeof(MASON_FILE_ENTRY)), NULL, FILE_BEGIN);
    MasonCurrentOffset = sizeof(MASON_ARCHIVE_HEADER) + ((DWORD)MasonFileCount * sizeof(MASON_FILE_ENTRY));

    for (MasonI = 0; MasonI < MasonFileCount; MasonI++) {
        HANDLE MasonInFile;
        BYTE *MasonRawData, *MasonProcData;
        DWORD MasonFileSize, MasonRead, MasonProcSize;
        char MasonStatusMsg[128];

        _snprintf(MasonStatusMsg, sizeof(MasonStatusMsg), "Building [%d/%d]: %s",
            MasonI + 1, MasonFileCount, MasonFileList[MasonI].MasonName);
        MasonSetStatus(MasonStatusMsg);

        MasonInFile = CreateFileA(MasonFileList[MasonI].MasonPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (MasonInFile == INVALID_HANDLE_VALUE) {
            _snprintf(MasonEntries[MasonI].MasonFileName, MASON_MAX_FILEPATH - 1, "%s", MasonFileList[MasonI].MasonName);
            MasonEntries[MasonI].MasonOriginalSize = 0;
            MasonEntries[MasonI].MasonCompressedSize = 0;
            MasonEntries[MasonI].MasonDataOffset = MasonCurrentOffset;
            MasonEntries[MasonI].MasonExecMode = MASON_EXEC_NORUN;
            continue;
        }
        MasonFileSize = GetFileSize(MasonInFile, NULL);
        if (MasonFileSize == 0 || MasonFileSize == 0xFFFFFFFF) { CloseHandle(MasonInFile); continue; }
        MasonRawData = (BYTE *)malloc(MasonFileSize + 1);
        if (!MasonRawData) { CloseHandle(MasonInFile); continue; }
        ReadFile(MasonInFile, MasonRawData, MasonFileSize, &MasonRead, NULL);
        CloseHandle(MasonInFile);

        _snprintf(MasonEntries[MasonI].MasonFileName, MASON_MAX_FILEPATH - 1, "%s", MasonFileList[MasonI].MasonName);
        MasonEntries[MasonI].MasonOriginalSize = MasonFileSize;
        MasonEntries[MasonI].MasonPriority = MasonFileList[MasonI].MasonPriority;
        MasonEntries[MasonI].MasonDataOffset = MasonCurrentOffset;
        MasonEntries[MasonI].MasonStartupType = MasonFileList[MasonI].MasonStartup;
        MasonEntries[MasonI].MasonExecMode = MasonFileList[MasonI].MasonExecMode;
        MasonEntries[MasonI].MasonHiddenFlag = MasonFileList[MasonI].MasonHidden ? 1 : 0;
        MasonEntries[MasonI].MasonDeleteFlag = 0;
        MasonEntries[MasonI].MasonSleepMs = MasonFileList[MasonI].MasonSleepMs;
        MasonEntries[MasonI].MasonChecksum = MasonCalcCrc32(MasonRawData, MasonFileSize);

        if (MasonFileList[MasonI].MasonDropDir[0] != '\0')
            _snprintf(MasonEntries[MasonI].MasonCustomDrop, MASON_MAX_FILEPATH - 1, "%s", MasonFileList[MasonI].MasonDropDir);

        MasonProcData = MasonRawData;
        MasonProcSize = MasonFileSize;

        if (MasonDoCompress) {
            BYTE *MasonCompBuf = (BYTE *)malloc(MasonFileSize + MasonFileSize / 2 + 256);
            DWORD MasonCompLen;
            if (MasonCompBuf) {
                MasonCompLen = MasonCompressRLE(MasonRawData, MasonFileSize,
                    MasonCompBuf, MasonFileSize + MasonFileSize / 2 + 256);
                if (MasonCompLen > 0 && MasonCompLen < MasonFileSize) {
                    MasonProcData = MasonCompBuf;
                    MasonProcSize = MasonCompLen;
                    MasonEntries[MasonI].MasonEntryFlags |= MASON_FL_COMPRESS;
                } else {
                    free(MasonCompBuf);
                    MasonCompBuf = NULL;
                }
            }
            MasonEntries[MasonI].MasonCompressedSize = MasonProcSize;
            WriteFile(MasonOutFile, MasonProcData, MasonProcSize, &MasonWritten, NULL);
            MasonCurrentOffset += MasonProcSize;
            MasonTotalOrig += MasonFileSize;
            MasonTotalComp += MasonProcSize;
            if (MasonCompBuf && MasonProcData == MasonCompBuf) free(MasonCompBuf);
        } else {
            MasonEntries[MasonI].MasonCompressedSize = MasonProcSize;
            WriteFile(MasonOutFile, MasonProcData, MasonProcSize, &MasonWritten, NULL);
            MasonCurrentOffset += MasonProcSize;
            MasonTotalOrig += MasonFileSize;
            MasonTotalComp += MasonProcSize;
        }
        free(MasonRawData);
        MasonSetProgress((MasonI + 1) * 100 / MasonFileCount);
        {
            MSG MasonPeekMsg;
            while (PeekMessageA(&MasonPeekMsg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&MasonPeekMsg);
                DispatchMessageA(&MasonPeekMsg);
            }
        }
    }

    MasonHdr.MasonTotalOrigSize = MasonTotalOrig;
    MasonHdr.MasonTotalCompSize = MasonTotalComp;
    SetFilePointer(MasonOutFile, MasonActualStubSize, NULL, FILE_BEGIN);
    WriteFile(MasonOutFile, &MasonHdr, sizeof(MasonHdr), &MasonWritten, NULL);
    WriteFile(MasonOutFile, MasonEntries, (DWORD)MasonFileCount * sizeof(MASON_FILE_ENTRY), &MasonWritten, NULL);
    SetFilePointer(MasonOutFile, 0, NULL, FILE_END);

    memset(&MasonFooter, 0, sizeof(MasonFooter));
    MasonFooter.MasonArchiveOffset = MasonActualStubSize;
    MasonGetGlobalDropPath(MasonDropBuf, sizeof(MasonDropBuf));
    _snprintf(MasonFooter.MasonDropPath, sizeof(MasonFooter.MasonDropPath) - 1, "%s", MasonDropBuf);
    MasonFooter.MasonAutoExec = 1;
    MasonFooter.MasonStubFlags = 0;
    if (SendMessageA(MasonChkHidden, BM_GETCHECK, 0, 0) == BST_CHECKED)
        MasonFooter.MasonStubFlags |= 0x0001;
    if (SendMessageA(MasonChkPump, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        char MasonPumpVal[16];
        DWORD MasonPumpKB, MasonPumpBytes;
        GetWindowTextA(MasonPumpEdit, MasonPumpVal, sizeof(MasonPumpVal));
        MasonPumpKB = (DWORD)atoi(MasonPumpVal);
        if (MasonPumpKB > 1048576) MasonPumpKB = 1048576;
        MasonPumpBytes = MasonPumpKB * 1024;
        if (MasonPumpBytes > 0 && MasonPumpKB > 0) {
            BYTE *MasonPadBuf = (BYTE *)calloc(1, MasonPumpBytes);
            if (MasonPadBuf) {
                MasonSetStatus("Pumping file size...");
                WriteFile(MasonOutFile, MasonPadBuf, MasonPumpBytes, &MasonWritten, NULL);
                free(MasonPadBuf);
            }
        }
    }

    MasonFooter.MasonSelfDelete = (SendMessageA(MasonChkSelfDel, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
    MasonFooter.MasonStubMagic = MASON_STUB_MAGIC;
    WriteFile(MasonOutFile, &MasonFooter, sizeof(MasonFooter), &MasonWritten, NULL);

    CloseHandle(MasonOutFile);
    free(MasonEntries);

    MasonOperationActive = FALSE;
    MasonSetProgress(100);

    {
        char MasonDoneBuf[300], MasonOrigSz[32], MasonCompSz[32];
        MasonFormatSize(MasonTotalOrig, MasonOrigSz, sizeof(MasonOrigSz));
        MasonFormatSize(MasonTotalComp, MasonCompSz, sizeof(MasonCompSz));
        _snprintf(MasonDoneBuf, sizeof(MasonDoneBuf),
            "Build complete!\n\nFiles: %d\nOriginal: %s\nPacked: %s\n\nSaved to:\n%s",
            MasonFileCount, MasonOrigSz, MasonCompSz, MasonOutPath);
        MessageBoxA(MasonMainWindow, MasonDoneBuf, "MasonBinder", MB_ICONINFORMATION);
    }
    MasonSetStatus("Build complete");
    return TRUE;
}

HWND MasonCreateButton(HWND MasonParent, const char *MasonText, int MasonId, int MasonX, int MasonY, int MasonW, int MasonH)
{
    return CreateWindowExA(0, "BUTTON", MasonText, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        MasonX, MasonY, MasonW, MasonH, MasonParent, (HMENU)(UINT_PTR)MasonId, MasonAppInstance, NULL);
}

HWND MasonCreateCheck(HWND MasonParent, const char *MasonText, int MasonId, int MasonX, int MasonY, int MasonW, int MasonH)
{
    return CreateWindowExA(0, "BUTTON", MasonText, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        MasonX, MasonY, MasonW, MasonH, MasonParent, (HMENU)(UINT_PTR)MasonId, MasonAppInstance, NULL);
}

void MasonDrawTitleBar(HDC MasonDc, RECT *MasonRect)
{
    RECT MasonTitleRect;
    RECT MasonMinBtn, MasonClsBtn;
    HPEN MasonPen, MasonOldPen;

    MasonTitleRect.left = MasonRect->left;
    MasonTitleRect.top = MasonRect->top;
    MasonTitleRect.right = MasonRect->right;
    MasonTitleRect.bottom = MasonRect->top + MASON_TITLEBAR_H;

    FillRect(MasonDc, &MasonTitleRect, MasonBrushTitleBar);

    MasonPen = CreatePen(PS_SOLID, 1, MASON_CLR_BORDER);
    MasonOldPen = (HPEN)SelectObject(MasonDc, MasonPen);
    MoveToEx(MasonDc, MasonTitleRect.left, MasonTitleRect.bottom - 1, NULL);
    LineTo(MasonDc, MasonTitleRect.right, MasonTitleRect.bottom - 1);
    SelectObject(MasonDc, MasonOldPen);
    DeleteObject(MasonPen);

    SetBkMode(MasonDc, TRANSPARENT);
    SetTextColor(MasonDc, MASON_CLR_TITLETXT);
    SelectObject(MasonDc, MasonFontBold);

    {
        RECT MasonTxtRect;
        MasonTxtRect.left = 14;
        MasonTxtRect.top = MasonTitleRect.top;
        MasonTxtRect.right = MasonTitleRect.right - 80;
        MasonTxtRect.bottom = MasonTitleRect.bottom;
        DrawTextA(MasonDc, "MasonBinder v2.0", -1, &MasonTxtRect,
            DT_SINGLELINE | DT_VCENTER | DT_LEFT);
    }

    MasonMinBtn.left = MasonRect->right - 68;
    MasonMinBtn.top = MasonTitleRect.top;
    MasonMinBtn.right = MasonRect->right - 34;
    MasonMinBtn.bottom = MasonTitleRect.bottom;

    MasonClsBtn.left = MasonRect->right - 34;
    MasonClsBtn.top = MasonTitleRect.top;
    MasonClsBtn.right = MasonRect->right;
    MasonClsBtn.bottom = MasonTitleRect.bottom;

    if (MasonHoverCloseBtn == 2) {
        HBRUSH MasonRedBrush = CreateSolidBrush(RGB(220, 60, 60));
        FillRect(MasonDc, &MasonClsBtn, MasonRedBrush);
        DeleteObject(MasonRedBrush);
        SetTextColor(MasonDc, RGB(255, 255, 255));
    } else {
        SetTextColor(MasonDc, MASON_CLR_TITLEBTN);
    }

    SelectObject(MasonDc, MasonFontNormal);
    DrawTextA(MasonDc, "X", -1, &MasonClsBtn, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

    if (MasonHoverCloseBtn == 1) {
        HBRUSH MasonHovBrush = CreateSolidBrush(RGB(215, 212, 206));
        FillRect(MasonDc, &MasonMinBtn, MasonHovBrush);
        DeleteObject(MasonHovBrush);
    }
    SetTextColor(MasonDc, MASON_CLR_TITLEBTN);
    DrawTextA(MasonDc, "_", -1, &MasonMinBtn, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
}

void MasonCreateControls(HWND MasonWnd)
{
    int MasonRowY = MASON_TITLEBAR_H + 2, MasonI;
    HWND MasonTmp;
    LVCOLUMNA MasonCol;

    MasonListView = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, NULL,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
        10, MasonRowY, MASON_WINDOW_W - 20, 370,
        MasonWnd, (HMENU)MASON_ID_LV, MasonAppInstance, NULL);
    SendMessageA(MasonListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    SendMessageA(MasonListView, WM_SETFONT, (WPARAM)MasonFontMono, TRUE);

    {
        static const struct { const char *MasonName; int MasonWidth; } MasonCols[] = {
            {"#", 26}, {"Filename", 180}, {"Size", 70}, {"Drop Path", 168},
            {"Exec", 55}, {"Flags", 60}, {"Sleep", 55}
        };
        for (MasonI = 0; MasonI < 7; MasonI++) {
            memset(&MasonCol, 0, sizeof(MasonCol));
            MasonCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            MasonCol.cx = MasonCols[MasonI].MasonWidth;
            MasonCol.pszText = (char *)MasonCols[MasonI].MasonName;
            MasonCol.iSubItem = MasonI;
            SendMessageA(MasonListView, LVM_INSERTCOLUMNA, MasonI, (LPARAM)&MasonCol);
        }
    }
    MasonRowY += 373;

    CreateWindowExA(0, "STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        10, MasonRowY, MASON_WINDOW_W - 20, 2, MasonWnd, NULL, MasonAppInstance, NULL);
    MasonRowY += 4;

    {
        int MasonOptX = 12;

        MasonTmp = CreateWindowExA(0, "STATIC", "Drop:", WS_CHILD | WS_VISIBLE | SS_LEFT,
            MasonOptX, MasonRowY + 4, 32, 18, MasonWnd, NULL, MasonAppInstance, NULL);
        SendMessageA(MasonTmp, WM_SETFONT, (WPARAM)MasonFontNormal, TRUE);
        MasonOptX += 34;

        MasonCmbDrop = CreateWindowExA(0, "COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            MasonOptX, MasonRowY, 95, 120, MasonWnd, (HMENU)MASON_ID_CMB_DROP, MasonAppInstance, NULL);
        SendMessageA(MasonCmbDrop, WM_SETFONT, (WPARAM)MasonFontNormal, TRUE);
        SendMessageA(MasonCmbDrop, CB_ADDSTRING, 0, (LPARAM)"AppData");
        SendMessageA(MasonCmbDrop, CB_ADDSTRING, 0, (LPARAM)"Temp");
        SendMessageA(MasonCmbDrop, CB_ADDSTRING, 0, (LPARAM)"Current");
        SendMessageA(MasonCmbDrop, CB_ADDSTRING, 0, (LPARAM)"Custom...");
        SendMessageA(MasonCmbDrop, CB_SETCURSEL, MASON_DROP_TEMP, 0);
        MasonOptX += 100;

        MasonChkCompress = MasonCreateCheck(MasonWnd, "Compress", MASON_ID_CHK_COMP, MasonOptX, MasonRowY + 2, 72, 20);
        SendMessageA(MasonChkCompress, WM_SETFONT, (WPARAM)MasonFontNormal, TRUE);
        MasonOptX += 76;

        MasonChkHidden = MasonCreateCheck(MasonWnd, "Hidden", MASON_ID_CHK_HIDDEN, MasonOptX, MasonRowY + 2, 56, 20);
        SendMessageA(MasonChkHidden, WM_SETFONT, (WPARAM)MasonFontNormal, TRUE);
        MasonOptX += 60;

        MasonChkUAC = MasonCreateCheck(MasonWnd, "UAC", MASON_ID_CHK_UAC, MasonOptX, MasonRowY + 2, 44, 20);
        SendMessageA(MasonChkUAC, WM_SETFONT, (WPARAM)MasonFontNormal, TRUE);
        MasonOptX += 48;

        MasonChkSelfDel = MasonCreateCheck(MasonWnd, "Melt", MASON_ID_CHK_SELFDEL, MasonOptX, MasonRowY + 2, 46, 20);
        SendMessageA(MasonChkSelfDel, WM_SETFONT, (WPARAM)MasonFontNormal, TRUE);
        MasonOptX += 50;

        MasonChkPump = MasonCreateCheck(MasonWnd, "Pump", MASON_ID_CHK_PUMP, MasonOptX, MasonRowY + 2, 48, 20);
        SendMessageA(MasonChkPump, WM_SETFONT, (WPARAM)MasonFontNormal, TRUE);
        MasonOptX += 52;

        MasonPumpEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "0",
            WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER,
            MasonOptX, MasonRowY + 1, 40, 20, MasonWnd, (HMENU)MASON_ID_PUMP_EDIT, MasonAppInstance, NULL);
        SendMessageA(MasonPumpEdit, WM_SETFONT, (WPARAM)MasonFontMono, TRUE);
        MasonOptX += 44;

        MasonTmp = CreateWindowExA(0, "STATIC", "KB", WS_CHILD | WS_VISIBLE | SS_LEFT,
            MasonOptX, MasonRowY + 4, 20, 16, MasonWnd, NULL, MasonAppInstance, NULL);
        SendMessageA(MasonTmp, WM_SETFONT, (WPARAM)MasonFontNormal, TRUE);
    }
    MasonRowY += 25;

    CreateWindowExA(0, "STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        10, MasonRowY, MASON_WINDOW_W - 20, 2, MasonWnd, NULL, MasonAppInstance, NULL);
    MasonRowY += 4;

    {
        HWND MasonExeBtn, MasonIconBtn, MasonIconLbl;

        MasonExeBtn = MasonCreateButton(MasonWnd, "BUILD EXE", MASON_ID_BTN_BUILDEXE,
            10, MasonRowY, 100, 30);
        SendMessageA(MasonExeBtn, WM_SETFONT, (WPARAM)MasonFontBold, TRUE);

        MasonIconBtn = MasonCreateButton(MasonWnd, "Icon...", MASON_ID_BTN_ICON,
            116, MasonRowY, 60, 30);
        SendMessageA(MasonIconBtn, WM_SETFONT, (WPARAM)MasonFontNormal, TRUE);

        MasonLblTotal = CreateWindowExA(0, "STATIC", "  Files: 0  |  Total: 0 B",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            182, MasonRowY + 1, 280, 14, MasonWnd, (HMENU)MASON_ID_LBL_TOTALSIZE, MasonAppInstance, NULL);
        SendMessageA(MasonLblTotal, WM_SETFONT, (WPARAM)MasonFontMono, TRUE);

        MasonIconLbl = CreateWindowExA(0, "STATIC", "Icon: (none)",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            182, MasonRowY + 16, 280, 14, MasonWnd, (HMENU)MASON_ID_LBL_ICON, MasonAppInstance, NULL);
        SendMessageA(MasonIconLbl, WM_SETFONT, (WPARAM)MasonFontNormal, TRUE);
        (void)MasonIconLbl;
    }
    MasonRowY += 33;

    MasonProgressBar = CreateWindowExA(0, PROGRESS_CLASSA, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        10, MasonRowY, MASON_WINDOW_W - 20, 10, MasonWnd, (HMENU)MASON_ID_PROGRESS, MasonAppInstance, NULL);
    SendMessageA(MasonProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    MasonRowY += 12;

    MasonStatusBar = CreateWindowExA(0, STATUSCLASSNAMEA, "(c) 2026 MasonGroup",
        WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, MasonWnd, (HMENU)MASON_ID_STATUSBAR, MasonAppInstance, NULL);
    SendMessageA(MasonStatusBar, WM_SETFONT, (WPARAM)MasonFontNormal, TRUE);
}

void MasonInitFonts(void)
{
    MasonFontNormal = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    MasonFontBold = CreateFontA(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    MasonFontMono = CreateFontA(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        FIXED_PITCH | FF_MODERN, "Consolas");
}

void MasonInitBrushes(void)
{
    MasonBrushBg = CreateSolidBrush(MASON_CLR_BG);
    MasonBrushListBg = CreateSolidBrush(MASON_CLR_LISTBG);
    MasonBrushBtn = CreateSolidBrush(MASON_CLR_BTN);
    MasonBrushTitleBar = CreateSolidBrush(MASON_CLR_TITLEBAR);
}

void MasonCleanup(void)
{
    if (MasonFontNormal) DeleteObject(MasonFontNormal);
    if (MasonFontBold) DeleteObject(MasonFontBold);
    if (MasonFontMono) DeleteObject(MasonFontMono);
    if (MasonBrushBg) DeleteObject(MasonBrushBg);
    if (MasonBrushListBg) DeleteObject(MasonBrushListBg);
    if (MasonBrushBtn) DeleteObject(MasonBrushBtn);
    if (MasonBrushTitleBar) DeleteObject(MasonBrushTitleBar);
}

void MasonHandleContextMenu(HWND MasonHwnd)
{
    int MasonSel = (int)SendMessageA(MasonListView, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
    HMENU MasonPopup;
    POINT MasonPt;
    int MasonCmd;
    MENUINFO MasonMi;

    GetCursorPos(&MasonPt);
    MasonPopup = CreatePopupMenu();

    memset(&MasonMi, 0, sizeof(MasonMi));
    MasonMi.cbSize = sizeof(MasonMi);
    MasonMi.fMask = MIM_STYLE;
    MasonMi.dwStyle = MNS_NOCHECK;
    SetMenuInfo(MasonPopup, &MasonMi);

    AppendMenuA(MasonPopup, MF_STRING, MASON_CTX_ADD_FILES, "  Add Files...");
    AppendMenuA(MasonPopup, MF_SEPARATOR, 0, NULL);

    if (MasonSel >= 0 && MasonSel < MasonFileCount) {
        AppendMenuA(MasonPopup, MF_STRING, MASON_CTX_REMOVE, "  Remove");
        AppendMenuA(MasonPopup, MF_STRING, MASON_CTX_MOVEUP, "  Move Up");
        AppendMenuA(MasonPopup, MF_STRING, MASON_CTX_MOVEDOWN, "  Move Down");
        AppendMenuA(MasonPopup, MF_SEPARATOR, 0, NULL);

        AppendMenuA(MasonPopup, MF_STRING | MF_GRAYED, 0, "  -- Drop Path --");
        AppendMenuA(MasonPopup, MF_STRING, MASON_CTX_DROP_APPDATA, "     AppData");
        AppendMenuA(MasonPopup, MF_STRING, MASON_CTX_DROP_TEMP, "     Temp");
        AppendMenuA(MasonPopup, MF_STRING, MASON_CTX_DROP_CURRENT, "     Current");
        AppendMenuA(MasonPopup, MF_STRING, MASON_CTX_DROP_CUSTOM, "     Custom...");
        AppendMenuA(MasonPopup, MF_SEPARATOR, 0, NULL);

        AppendMenuA(MasonPopup, MF_STRING | MF_GRAYED, 0, "  -- Execution --");
        AppendMenuA(MasonPopup, MF_STRING | ((MasonFileList[MasonSel].MasonExecMode == MASON_EXEC_RUNNING) ? MF_CHECKED : 0), MASON_CTX_EXEC_RUNNING, "     Running");
        AppendMenuA(MasonPopup, MF_STRING | ((MasonFileList[MasonSel].MasonExecMode == MASON_EXEC_RUNONCE) ? MF_CHECKED : 0), MASON_CTX_EXEC_RUNONCE, "     Runonce");
        AppendMenuA(MasonPopup, MF_STRING | ((MasonFileList[MasonSel].MasonExecMode == MASON_EXEC_NORUN) ? MF_CHECKED : 0), MASON_CTX_EXEC_NORUN, "     No Run");
        AppendMenuA(MasonPopup, MF_SEPARATOR, 0, NULL);

        AppendMenuA(MasonPopup, MF_STRING | MF_GRAYED, 0, "  -- Startup --");
        AppendMenuA(MasonPopup, MF_STRING | ((MasonFileList[MasonSel].MasonStartup == MASON_STARTUP_REGISTRY) ? MF_CHECKED : 0), MASON_CTX_START_REG, "     Registry");
        AppendMenuA(MasonPopup, MF_STRING | ((MasonFileList[MasonSel].MasonStartup == MASON_STARTUP_SCHEDULER) ? MF_CHECKED : 0), MASON_CTX_START_SCH, "     Scheduler");
        AppendMenuA(MasonPopup, MF_STRING | ((MasonFileList[MasonSel].MasonStartup == MASON_STARTUP_NONE) ? MF_CHECKED : 0), MASON_CTX_START_NONE, "     None");
        AppendMenuA(MasonPopup, MF_SEPARATOR, 0, NULL);

        AppendMenuA(MasonPopup, MF_STRING | (MasonFileList[MasonSel].MasonHidden ? MF_CHECKED : 0), MASON_CTX_HIDDEN_ON, "  Hidden");
        AppendMenuA(MasonPopup, MF_STRING, MASON_CTX_SLEEP, "  Sleep...");
        AppendMenuA(MasonPopup, MF_SEPARATOR, 0, NULL);
        AppendMenuA(MasonPopup, MF_STRING, MASON_CTX_RESET, "  Reset to Default");
    }

    AppendMenuA(MasonPopup, MF_SEPARATOR, 0, NULL);
    AppendMenuA(MasonPopup, MF_STRING, MASON_CTX_CLEAR, "  Clear All");
    AppendMenuA(MasonPopup, MF_SEPARATOR, 0, NULL);
    AppendMenuA(MasonPopup, MF_STRING, MASON_CTX_ABOUT, "  About MasonBinder");

    MasonCmd = TrackPopupMenu(MasonPopup, TPM_RETURNCMD | TPM_RIGHTBUTTON,
        MasonPt.x, MasonPt.y, 0, MasonHwnd, NULL);
    DestroyMenu(MasonPopup);

    switch (MasonCmd) {
    case MASON_CTX_ADD_FILES:
        MasonAddFilesToList();
        break;
    case MASON_CTX_REMOVE:
        if (MasonSel >= 0 && MasonSel < MasonFileCount) {
            int MasonJ;
            for (MasonJ = MasonSel; MasonJ < MasonFileCount - 1; MasonJ++)
                MasonFileList[MasonJ] = MasonFileList[MasonJ + 1];
            MasonFileCount--;
            MasonRefreshListView();
        }
        break;
    case MASON_CTX_MOVEUP:
        if (MasonSel > 0 && MasonSel < MasonFileCount) {
            MASON_LIST_ITEM MasonTemp = MasonFileList[MasonSel];
            MasonFileList[MasonSel] = MasonFileList[MasonSel - 1];
            MasonFileList[MasonSel - 1] = MasonTemp;
            MasonRefreshListView();
            {
                LVITEMA MasonLvi;
                memset(&MasonLvi, 0, sizeof(MasonLvi));
                MasonLvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                MasonLvi.state = LVIS_SELECTED | LVIS_FOCUSED;
                SendMessageA(MasonListView, LVM_SETITEMSTATE, MasonSel - 1, (LPARAM)&MasonLvi);
            }
        }
        break;
    case MASON_CTX_MOVEDOWN:
        if (MasonSel >= 0 && MasonSel < MasonFileCount - 1) {
            MASON_LIST_ITEM MasonTemp = MasonFileList[MasonSel];
            MasonFileList[MasonSel] = MasonFileList[MasonSel + 1];
            MasonFileList[MasonSel + 1] = MasonTemp;
            MasonRefreshListView();
            {
                LVITEMA MasonLvi;
                memset(&MasonLvi, 0, sizeof(MasonLvi));
                MasonLvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                MasonLvi.state = LVIS_SELECTED | LVIS_FOCUSED;
                SendMessageA(MasonListView, LVM_SETITEMSTATE, MasonSel + 1, (LPARAM)&MasonLvi);
            }
        }
        break;
    case MASON_CTX_CLEAR:
        if (MasonFileCount > 0 &&
            MessageBoxA(MasonMainWindow, "Clear all files?", "MasonBinder", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            MasonFileCount = 0;
            memset(MasonFileList, 0, sizeof(MasonFileList));
            MasonRefreshListView();
        }
        break;
    case MASON_CTX_DROP_APPDATA:
        if (MasonSel >= 0) { _snprintf(MasonFileList[MasonSel].MasonDropDir, MASON_MAX_FILEPATH - 1, "%%APPDATA%%"); MasonRefreshListView(); }
        break;
    case MASON_CTX_DROP_TEMP:
        if (MasonSel >= 0) { _snprintf(MasonFileList[MasonSel].MasonDropDir, MASON_MAX_FILEPATH - 1, "%%TEMP%%"); MasonRefreshListView(); }
        break;
    case MASON_CTX_DROP_CURRENT:
        if (MasonSel >= 0) { _snprintf(MasonFileList[MasonSel].MasonDropDir, MASON_MAX_FILEPATH - 1, "%%CURRENT%%"); MasonRefreshListView(); }
        break;
    case MASON_CTX_DROP_CUSTOM:
        if (MasonSel >= 0) {
            MasonAskCustomPath();
            if (MasonCustomDropPath[0] != '\0') {
                _snprintf(MasonFileList[MasonSel].MasonDropDir, MASON_MAX_FILEPATH - 1, "%s", MasonCustomDropPath);
                MasonFileList[MasonSel].MasonDropDir[MASON_MAX_FILEPATH - 1] = '\0';
                MasonRefreshListView();
            }
        }
        break;
    case MASON_CTX_START_REG:
        if (MasonSel >= 0) { MasonFileList[MasonSel].MasonStartup = MASON_STARTUP_REGISTRY; MasonRefreshListView(); }
        break;
    case MASON_CTX_START_SCH:
        if (MasonSel >= 0) { MasonFileList[MasonSel].MasonStartup = MASON_STARTUP_SCHEDULER; MasonRefreshListView(); }
        break;
    case MASON_CTX_START_NONE:
        if (MasonSel >= 0) { MasonFileList[MasonSel].MasonStartup = MASON_STARTUP_NONE; MasonRefreshListView(); }
        break;
    case MASON_CTX_EXEC_RUNNING:
        if (MasonSel >= 0) { MasonFileList[MasonSel].MasonExecMode = MASON_EXEC_RUNNING; MasonRefreshListView(); }
        break;
    case MASON_CTX_EXEC_RUNONCE:
        if (MasonSel >= 0) { MasonFileList[MasonSel].MasonExecMode = MASON_EXEC_RUNONCE; MasonRefreshListView(); }
        break;
    case MASON_CTX_EXEC_NORUN:
        if (MasonSel >= 0) { MasonFileList[MasonSel].MasonExecMode = MASON_EXEC_NORUN; MasonRefreshListView(); }
        break;
    case MASON_CTX_HIDDEN_ON:
        if (MasonSel >= 0) { MasonFileList[MasonSel].MasonHidden = !MasonFileList[MasonSel].MasonHidden; MasonRefreshListView(); }
        break;
    case MASON_CTX_SLEEP:
        if (MasonSel >= 0) {
            int MasonSleepVal = MasonAskSleep();
            if (MasonSleepVal > 0) {
                MasonFileList[MasonSel].MasonSleepMs = (DWORD)MasonSleepVal;
                MasonRefreshListView();
            }
        }
        break;
    case MASON_CTX_RESET:
        if (MasonSel >= 0) {
            MasonFileList[MasonSel].MasonDropDir[0] = '\0';
            MasonFileList[MasonSel].MasonStartup = MASON_STARTUP_NONE;
            MasonFileList[MasonSel].MasonExecMode = MASON_EXEC_RUNNING;
            MasonFileList[MasonSel].MasonHidden = FALSE;
            MasonFileList[MasonSel].MasonSleepMs = 0;
            MasonRefreshListView();
        }
        break;
    case MASON_CTX_ABOUT:
        MessageBoxA(MasonHwnd, MasonCopyright, "MasonBinder - About", MB_ICONINFORMATION);
        break;
    }
}

LRESULT CALLBACK MasonWndProc(HWND MasonHwnd, UINT MasonMsg, WPARAM MasonWp, LPARAM MasonLp)
{
    switch (MasonMsg) {
    case WM_CREATE:
        MasonCreateControls(MasonHwnd);
        DragAcceptFiles(MasonHwnd, TRUE);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT MasonPs;
        HDC MasonDc = BeginPaint(MasonHwnd, &MasonPs);
        RECT MasonRect;
        GetClientRect(MasonHwnd, &MasonRect);
        MasonDrawTitleBar(MasonDc, &MasonRect);
        EndPaint(MasonHwnd, &MasonPs);
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        int MasonX = (short)LOWORD(MasonLp);
        int MasonY = (short)HIWORD(MasonLp);
        RECT MasonRect;
        GetClientRect(MasonHwnd, &MasonRect);

        if (MasonY < MASON_TITLEBAR_H) {
            if (MasonX >= MasonRect.right - 34) {
                if (MasonOperationActive) {
                    MessageBoxA(MasonHwnd, "Operation in progress.", "MasonBinder", MB_ICONWARNING);
                    return 0;
                }
                DestroyWindow(MasonHwnd);
                return 0;
            }
            if (MasonX >= MasonRect.right - 68) {
                ShowWindow(MasonHwnd, SW_MINIMIZE);
                return 0;
            }
            MasonDraggingTitle = TRUE;
            MasonDragStart.x = MasonX;
            MasonDragStart.y = MasonY;
            SetCapture(MasonHwnd);
        }
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        int MasonNewHover = 0;
        int MasonMx = (short)LOWORD(MasonLp);
        int MasonMy = (short)HIWORD(MasonLp);
        RECT MasonRect;
        GetClientRect(MasonHwnd, &MasonRect);

        if (MasonDraggingTitle) {
            RECT MasonWinRect;
            GetWindowRect(MasonHwnd, &MasonWinRect);
            MoveWindow(MasonHwnd,
                MasonWinRect.left + MasonMx - MasonDragStart.x,
                MasonWinRect.top + MasonMy - MasonDragStart.y,
                MasonWinRect.right - MasonWinRect.left,
                MasonWinRect.bottom - MasonWinRect.top, TRUE);
            return 0;
        }

        if (MasonMy >= 0 && MasonMy < MASON_TITLEBAR_H) {
            if (MasonMx >= MasonRect.right - 34) MasonNewHover = 2;
            else if (MasonMx >= MasonRect.right - 68) MasonNewHover = 1;
        }

        if (MasonNewHover != MasonHoverCloseBtn) {
            MasonHoverCloseBtn = MasonNewHover;
            {
                RECT MasonInvRect;
                MasonInvRect.left = MasonRect.right - 68;
                MasonInvRect.top = 0;
                MasonInvRect.right = MasonRect.right;
                MasonInvRect.bottom = MASON_TITLEBAR_H;
                InvalidateRect(MasonHwnd, &MasonInvRect, FALSE);
            }
        }

        {
            TRACKMOUSEEVENT MasonTme;
            MasonTme.cbSize = sizeof(MasonTme);
            MasonTme.dwFlags = TME_LEAVE;
            MasonTme.hwndTrack = MasonHwnd;
            MasonTme.dwHoverTime = 0;
            TrackMouseEvent(&MasonTme);
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        if (MasonHoverCloseBtn != 0) {
            MasonHoverCloseBtn = 0;
            {
                RECT MasonRect, MasonInvRect;
                GetClientRect(MasonHwnd, &MasonRect);
                MasonInvRect.left = MasonRect.right - 68;
                MasonInvRect.top = 0;
                MasonInvRect.right = MasonRect.right;
                MasonInvRect.bottom = MASON_TITLEBAR_H;
                InvalidateRect(MasonHwnd, &MasonInvRect, FALSE);
            }
        }
        return 0;

    case WM_LBUTTONUP:
        if (MasonDraggingTitle) {
            MasonDraggingTitle = FALSE;
            ReleaseCapture();
        }
        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        HDC MasonDc = (HDC)MasonWp;
        SetTextColor(MasonDc, MASON_CLR_TEXT);
        SetBkMode(MasonDc, TRANSPARENT);
        return (LRESULT)MasonBrushBg;
    }

    case WM_CTLCOLOREDIT:
    {
        HDC MasonDc = (HDC)MasonWp;
        SetTextColor(MasonDc, MASON_CLR_TEXT);
        SetBkColor(MasonDc, MASON_CLR_LISTBG);
        return (LRESULT)MasonBrushListBg;
    }

    case WM_CTLCOLORLISTBOX:
    {
        HDC MasonDc = (HDC)MasonWp;
        SetTextColor(MasonDc, MASON_CLR_TEXT);
        SetBkColor(MasonDc, MASON_CLR_LISTBG);
        return (LRESULT)MasonBrushListBg;
    }

    case WM_ERASEBKGND:
    {
        RECT MasonRect;
        GetClientRect(MasonHwnd, &MasonRect);
        MasonRect.top += MASON_TITLEBAR_H;
        FillRect((HDC)MasonWp, &MasonRect, MasonBrushBg);
        return 1;
    }

    case WM_NOTIFY:
    {
        NMHDR *MasonNm = (NMHDR *)MasonLp;
        if (MasonNm->idFrom == MASON_ID_LV && MasonNm->code == NM_RCLICK) {
            MasonHandleContextMenu(MasonHwnd);
            return 0;
        }
        break;
    }

    case WM_DROPFILES:
    {
        HDROP MasonDrop = (HDROP)MasonWp;
        UINT MasonCount = DragQueryFileA(MasonDrop, 0xFFFFFFFF, NULL, 0);
        UINT MasonDi;
        int MasonAdded = 0;
        for (MasonDi = 0; MasonDi < MasonCount && MasonFileCount < MASON_MAX_FILES; MasonDi++) {
            char MasonDropPath[MASON_MAX_FILEPATH];
            DragQueryFileA(MasonDrop, MasonDi, MasonDropPath, sizeof(MasonDropPath));
            if (MasonCheckDuplicate(MasonDropPath)) continue;
            memset(&MasonFileList[MasonFileCount], 0, sizeof(MASON_LIST_ITEM));
            _snprintf(MasonFileList[MasonFileCount].MasonPath, MASON_MAX_FILEPATH - 1, "%s", MasonDropPath);
            MasonExtractFileName(MasonDropPath, MasonFileList[MasonFileCount].MasonName, 260);
            {
                HANDLE MasonHf = CreateFileA(MasonDropPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
                if (MasonHf != INVALID_HANDLE_VALUE) {
                    MasonFileList[MasonFileCount].MasonSize = GetFileSize(MasonHf, NULL);
                    CloseHandle(MasonHf);
                }
            }
            MasonFileList[MasonFileCount].MasonExecMode = MASON_EXEC_RUNNING;
            MasonFileCount++;
            MasonAdded++;
        }
        DragFinish(MasonDrop);
        if (MasonAdded > 0) {
            char MasonDropMsg[64];
            MasonRefreshListView();
            _snprintf(MasonDropMsg, sizeof(MasonDropMsg), "Dropped %d file(s)", MasonAdded);
            MasonSetStatus(MasonDropMsg);
        }
        return 0;
    }

    case WM_COMMAND:
    {
        int MasonCmdId = LOWORD(MasonWp);
        if (MasonCmdId == MASON_ID_BTN_BUILDEXE) MasonBuildExe();
        else if (MasonCmdId == MASON_ID_BTN_ICON) MasonSelectIconSource();
        else if (MasonCmdId == MASON_ID_CMB_DROP && HIWORD(MasonWp) == CBN_SELCHANGE) {
            int MasonDropSel = (int)SendMessageA(MasonCmbDrop, CB_GETCURSEL, 0, 0);
            if (MasonDropSel == MASON_DROP_CUSTOM) MasonAskCustomPath();
        }
        return 0;
    }

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO *MasonMmi = (MINMAXINFO *)MasonLp;
        MasonMmi->ptMinTrackSize.x = MASON_WINDOW_W;
        MasonMmi->ptMinTrackSize.y = MASON_WINDOW_H;
        MasonMmi->ptMaxTrackSize.x = MASON_WINDOW_W;
        MasonMmi->ptMaxTrackSize.y = MASON_WINDOW_H;
        return 0;
    }

    case WM_CLOSE:
        if (MasonOperationActive) {
            MessageBoxA(MasonHwnd, "Operation in progress.", "MasonBinder", MB_ICONWARNING);
            return 0;
        }
        DestroyWindow(MasonHwnd);
        return 0;

    case WM_DESTROY:
        MasonCleanup();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(MasonHwnd, MasonMsg, MasonWp, MasonLp);
}

int WINAPI WinMain(HINSTANCE MasonInst, HINSTANCE MasonPrev, LPSTR MasonCmdLine, int MasonShow)
{
    WNDCLASSEXA MasonWc;
    MSG MasonMsg;
    INITCOMMONCONTROLSEX MasonIcc;

    (void)MasonPrev;
    (void)MasonCmdLine;
    MasonAppInstance = MasonInst;

    MasonIcc.dwSize = sizeof(MasonIcc);
    MasonIcc.dwICC = ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS | ICC_BAR_CLASSES;
    InitCommonControlsEx(&MasonIcc);
    OleInitialize(NULL);
    MasonInitFonts();
    MasonInitBrushes();
    memset(MasonCustomDropPath, 0, sizeof(MasonCustomDropPath));
    memset(MasonIconSourcePath, 0, sizeof(MasonIconSourcePath));

    memset(&MasonWc, 0, sizeof(MasonWc));
    MasonWc.cbSize = sizeof(MasonWc);
    MasonWc.style = CS_HREDRAW | CS_VREDRAW;
    MasonWc.lpfnWndProc = MasonWndProc;
    MasonWc.hInstance = MasonInst;
    MasonWc.hCursor = LoadCursor(NULL, IDC_ARROW);
    MasonWc.hbrBackground = MasonBrushBg;
    MasonWc.lpszClassName = "MasonBinderClass";
    MasonWc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    MasonWc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExA(&MasonWc)) return 1;

    {
        int MasonScreenW = GetSystemMetrics(SM_CXSCREEN);
        int MasonScreenH = GetSystemMetrics(SM_CYSCREEN);
        int MasonPosX = (MasonScreenW - MASON_WINDOW_W) / 2;
        int MasonPosY = (MasonScreenH - MASON_WINDOW_H) / 2;

        MasonMainWindow = CreateWindowExA(
            WS_EX_ACCEPTFILES,
            "MasonBinderClass", "",
            WS_POPUP | WS_VISIBLE | WS_MINIMIZEBOX,
            MasonPosX, MasonPosY,
            MASON_WINDOW_W, MASON_WINDOW_H,
            NULL, NULL, MasonInst, NULL);
    }

    if (!MasonMainWindow) return 1;

    ShowWindow(MasonMainWindow, MasonShow);
    UpdateWindow(MasonMainWindow);

    while (GetMessageA(&MasonMsg, NULL, 0, 0) > 0) {
        TranslateMessage(&MasonMsg);
        DispatchMessageA(&MasonMsg);
    }

    OleUninitialize();
    return (int)MasonMsg.wParam;
}

/*
 *  ============================================================
 *   End of MasonBinder.c
 *   (c) 2026 MasonGroup - All Rights Reserved
 *   MasonGroup Engineering Division
 *  ============================================================
 */
