#include <windows.h>
#include <vfw.h>
//#pragma hdrstop

static const DWORD FOURCC_MPNG = mmioFOURCC('M','P','N','G');   // our compressed format

extern HMODULE hmoduleMPNG;

#define ALIGN4(V) (((V + 3) >> 2) << 2)

struct CodecInst
{
    unsigned char **rows;

    // methods
    CodecInst() : rows(0) {}

    BOOL QueryAbout();
    DWORD About(HWND hwnd);

    BOOL QueryConfigure();
    DWORD Configure(HWND hwnd);

    DWORD GetState(LPVOID pv, DWORD dwSize);
    DWORD SetState(LPVOID pv, DWORD dwSize);

    DWORD GetInfo(ICINFO* icinfo, DWORD dwSize);

    DWORD CompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
    DWORD CompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
    DWORD CompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
    DWORD CompressGetSize(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
    DWORD Compress(ICCOMPRESS* icinfo, DWORD dwSize);
    DWORD CompressEnd();

    DWORD DecompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
    DWORD DecompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
    DWORD DecompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
    DWORD Decompress(ICDECOMPRESS* icinfo, DWORD dwSize);
    DWORD DecompressGetPalette(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
    DWORD DecompressEnd();

    void log_message(const char fmt[], ...);
};

CodecInst* Open(ICOPEN* icinfo);
DWORD Close(CodecInst* pinst);
