#include "CodecInst.h"

#include <png.h>

#ifdef _DEBUG
	#ifdef _M_X64
		#pragma comment(lib, "Win64_LIB_ASM_Debug-libpngd.lib")
		#pragma comment(lib, "Win64_LIB_ASM_Debug-zlibd.lib")
	#else
		#pragma comment(lib, "Win32_LIB_ASM_Debug-libpngd.lib")
		#pragma comment(lib, "Win32_LIB_ASM_Debug-zlibd.lib")
	#endif /* _M_X64 */
#else
	#ifdef _M_X64
		#pragma comment(lib, "Win64_LIB_ASM_Release-libpng.lib")
		#pragma comment(lib, "Win64_LIB_ASM_Release-zlib.lib")
	#else
		#pragma comment(lib, "Win32_LIB_ASM_Release-libpng.lib")
		#pragma comment(lib, "Win32_LIB_ASM_Release-zlib.lib")
	#endif /* _M_X64 */
#endif

/* we accept RGB or RGBA input */
static bool CanCompress(LPBITMAPINFOHEADER lpbiIn)
{
    if(0 == lpbiIn->biCompression || mmioFOURCC('D','I','B',' ') == lpbiIn->biCompression)
    {
        if(24 == lpbiIn->biBitCount || 32 == lpbiIn->biBitCount)
            return true;
    };
    return false;
};

static bool CanCompress(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    if(!lpbiOut)
        return CanCompress(lpbiIn);

    if(FOURCC_MPNG == lpbiOut->biCompression)
        return (lpbiIn->biBitCount == lpbiOut->biBitCount);

    return false;
};

DWORD CodecInst::CompressGetSize(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    return ALIGN4(lpbiIn->biWidth) * lpbiIn->biHeight * (lpbiIn->biBitCount / 8);
};

DWORD CodecInst::CompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    bool r = CanCompress(lpbiIn, lpbiOut);

    log_message
    (
       "CodecInst::CompressQuery()=%s  lpbiIn->biCompression=[%.8X], lpbiIn->biBitCount=%d\n\n",
       r ? "ICERR_OK":"ICERR_BADFORMAT",
       lpbiIn->biCompression,
       lpbiIn->biBitCount
    );

    return r ? ICERR_OK : ICERR_BADFORMAT;
};

DWORD CodecInst::CompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    if (lpbiOut == 0)
        return sizeof(BITMAPINFOHEADER);

    bool r = CanCompress(lpbiIn);

    log_message
    (
       "CodecInst::CompressGetFormat()=%s lpbiIn->biCompression=[%.8X], lpbiIn->biBitCount=%d lpbiOut->biCompression=[%.8X], lpbiOut->biBitCount=%d\n\n",
       r ? "ICERR_OK":"ICERR_BADFORMAT",
       lpbiIn->biCompression,
       lpbiIn->biBitCount,
       (lpbiOut)?lpbiOut->biCompression:0xFFFFFFFF,
       (lpbiOut)?lpbiOut->biBitCount:0
    );

    *lpbiOut = *lpbiIn;
    lpbiOut->biCompression = FOURCC_MPNG;
    return ICERR_OK;
};

DWORD CodecInst::CompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    log_message("CodecInst::CompressBegin()\n\n");

    CompressEnd();  // free resources if necessary

    // allocate buffers
    rows = (unsigned char**)malloc(sizeof(unsigned char*) * lpbiIn->biHeight);

    return CanCompress(lpbiIn, lpbiOut) ? ICERR_OK : ICERR_BADFORMAT;
};

DWORD CodecInst::CompressEnd()
{
    // release buffers
    if(rows)
    {
        log_message("CodecInst::CompressEnd()\n\n");
        free(rows);
        rows = NULL;
    };

    return ICERR_OK;
};

static png_size_t compressed_data_writer(png_structp png_ptr, png_bytep data, png_size_t len)
{
    void **buf_ptr = (void **)png_get_progressive_ptr(png_ptr);
    unsigned char *buf = (unsigned char *)(*buf_ptr);

    if(data)
    {
        memcpy(buf, data, len);
        buf += len;
        *buf_ptr = buf;
    };

    return len;
};

DWORD CodecInst::Compress(ICCOMPRESS* icinfo, DWORD dwSize)
{
    int cs;
    png_structp png_ptr;
    png_infop info_ptr;

    log_message("CodecInst::Compress(%d) INPUT\n\n", icinfo->lFrameNum);

    if(ICERR_OK != CompressQuery(icinfo->lpbiInput, icinfo->lpbiOutput))
        return ICERR_BADFORMAT;

    if(32 == icinfo->lpbiInput->biBitCount)
        cs = PNG_COLOR_TYPE_RGB_ALPHA;
    else if(24 == icinfo->lpbiInput->biBitCount)
        cs = PNG_COLOR_TYPE_RGB;
    else
    {
        log_message("CodecInst::Compress error value of icinfo->lpbiInput->biBitCount=%d\n",
            icinfo->lpbiInput->biBitCount);
        return ICERR_BADFORMAT;
    };

    /* initialize stuff */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
    {
        log_message("png_create_write_struct() failed\n");
        return ICERR_MEMORY;
    };

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        log_message("png_create_info_struct() failed\n");
        return ICERR_MEMORY;
    };

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        log_message("setjmp(png_jmpbuf(png_ptr)) failed\n");
        return ICERR_ERROR;
    };

    /* calc line size */
    int line_size = ALIGN4(icinfo->lpbiInput->biWidth) * (icinfo->lpbiInput->biBitCount / 8);

    /* calc rows ptrs */
    for(int i = 0; i < icinfo->lpbiInput->biHeight; i++)
        rows[i] = (unsigned char*)icinfo->lpInput + line_size * i;

    /* setup output buffer ptr */
    void* buf = icinfo->lpOutput;

    png_set_write_fn(png_ptr, &buf, (png_rw_ptr)compressed_data_writer, (png_flush_ptr)NULL);

    /* write header */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        log_message("setjmp(png_jmpbuf(png_ptr)) failed\n");
        return ICERR_ERROR;
    };

    png_set_IHDR
    (
        png_ptr, info_ptr,
        icinfo->lpbiInput->biWidth, icinfo->lpbiInput->biHeight,
        8, cs,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE
    );

    png_write_info(png_ptr, info_ptr);

    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        log_message("setjmp(png_jmpbuf(png_ptr)) failed\n");
        return ICERR_ERROR;
    };

//    png_write_rows(png_ptr, (png_bytepp)rows, icinfo->lpbiInput->biHeight);
    png_write_image(png_ptr, (png_bytepp)rows);

    /* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        log_message("setjmp(png_jmpbuf(png_ptr)) failed\n");
        return ICERR_ERROR;
    };

    png_write_end(png_ptr, NULL);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    /* setup compressed inmage size */
    icinfo->lpbiOutput->biSizeImage = (unsigned char*)buf - (unsigned char*)icinfo->lpOutput;
    if (icinfo->lpckid)
        *icinfo->lpckid = FOURCC_MPNG;
    *icinfo->lpdwFlags = AVIIF_KEYFRAME;

    log_message("CodecInst::Compress(%d) FINISHED (%d bytes)\n\n", icinfo->lFrameNum, icinfo->lpbiOutput->biSizeImage);

    return ICERR_OK;
};
