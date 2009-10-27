#include "CodecInst.h"

static TCHAR codec_description[]    = TEXT("Motion PNG v1.0");
static TCHAR codec_name[]           = TEXT("Motion PNG");
#define CODEC_VERSION               0x00010002              // 1.0

CodecInst* Open(ICOPEN* icinfo)
{
    if (icinfo && ICTYPE_VIDEO != icinfo->fccType)
        return NULL;

    CodecInst* pinst = new CodecInst();

    if (icinfo)
        icinfo->dwError = pinst ? ICERR_OK : ICERR_MEMORY;

    return pinst;
};

DWORD Close(CodecInst* pinst)
{
//    delete pinst;       // this caused problems when deleting at app close time
    return 1;
};

DWORD CodecInst::GetInfo(ICINFO* icinfo, DWORD dwSize)
{
    if (icinfo == NULL)
        return sizeof(ICINFO);

    if (dwSize < sizeof(ICINFO))
        return 0;

    icinfo->dwSize              = sizeof(ICINFO);
    icinfo->fccType             = ICTYPE_VIDEO;
    icinfo->fccHandler          = FOURCC_MPNG;
    icinfo->dwFlags             = 0;

    icinfo->dwVersion           = CODEC_VERSION;
    icinfo->dwVersionICM        = ICVERSION;

    MultiByteToWideChar(CP_ACP, 0, codec_description, -1,
        icinfo->szDescription, sizeof(icinfo->szDescription)/sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, codec_name, -1,
        icinfo->szName, sizeof(icinfo->szName)/sizeof(WCHAR));

  return sizeof(ICINFO);
};

/* we have no state information which needs to be stored */
DWORD CodecInst::GetState(LPVOID pv, DWORD dwSize) { return 0; };
DWORD CodecInst::SetState(LPVOID pv, DWORD dwSize) { return 0; };

void CodecInst::log_message(const char fmt[], ...)
{
#ifndef _DEBUG
    static int debug = GetPrivateProfileInt("debug", "log", 0, "mpng.ini");
    if (!debug) return;
#endif /* _DEBUG */

    DWORD written;
    char buf[2000];
    va_list val;
  
    va_start(val, fmt);
    wvsprintf(buf, fmt, val);

    const COORD _80x50 = {80,50};
    static BOOL startup = (AllocConsole(), SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), _80x50));
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buf, lstrlen(buf), &written, 0);
};
