#include "CodecInst.h"

#include <png.h>

#ifdef _DEBUG
#pragma comment(lib, "Win32_LIB_ASM_Debug-libpngd.lib")
#pragma comment(lib, "Win32_LIB_ASM_Debug-zlibd.lib")
#else
#pragma comment(lib, "Win32_LIB_ASM_Release-libpng.lib")
#pragma comment(lib, "Win32_LIB_ASM_Release-zlib.lib")
#endif

static bool CanDecompress(LPBITMAPINFOHEADER lpbiIn)
{
    if(FOURCC_MPNG == lpbiIn->biCompression)
        return true;
    return false;
};

static bool CanDecompress(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    if(!lpbiOut)
        return CanDecompress(lpbiIn);

    if(0 == lpbiOut->biCompression || mmioFOURCC('D','I','B',' ') == lpbiOut->biCompression)
        return (lpbiIn->biBitCount == lpbiOut->biBitCount);

    return false;
};

DWORD CodecInst::DecompressQuery(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    bool r = CanDecompress(lpbiIn, lpbiOut);

    log_message
    (
       "CodecInst::DecompressQuery()=%s lpbiIn->biCompression=[%.8X], lpbiIn->biBitCount=%d lpbiOut->biCompression=[%.8X], lpbiOut->biBitCount=%d\n\n",
       r ? "ICERR_OK":"ICERR_BADFORMAT",
       lpbiIn->biCompression,
       lpbiIn->biBitCount,
       (lpbiOut)?lpbiOut->biCompression:0xFFFFFFFF,
       (lpbiOut)?lpbiOut->biBitCount:0
    );

    return r ? ICERR_OK : ICERR_BADFORMAT;
};

DWORD CodecInst::DecompressGetFormat(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
  // if lpbiOut == NULL, then return the size required to hold an output format struct
    if (lpbiOut == NULL)
        return lpbiIn->biSize;

    if (!CanDecompress(lpbiIn))
        return ICERR_BADFORMAT;

    memcpy(lpbiOut, lpbiIn, lpbiIn->biSize);

    lpbiOut->biCompression = 0;
    lpbiOut->biSizeImage = ALIGN4(lpbiIn->biWidth) * lpbiIn->biHeight * (lpbiIn->biBitCount / 8);

    return ICERR_OK;
};

DWORD CodecInst::DecompressBegin(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    CompressEnd();  // free resources if necessary

    // allocate buffers
    rows = (unsigned char**)malloc(sizeof(unsigned char*) * lpbiOut->biHeight);

    return CanDecompress(lpbiIn, lpbiOut) ? ICERR_OK : ICERR_BADFORMAT;
};

DWORD CodecInst::DecompressEnd()
{
    // release buffers
    if(rows)
    {
        free(rows);
        rows = NULL;
    };

    return ICERR_OK;
};

DWORD CodecInst::DecompressGetPalette(LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    return ICERR_BADFORMAT;
};

static png_size_t compressed_data_reader(png_structp png_ptr, png_bytep data, png_size_t len)
{
    void **buf_ptr = (void **)png_get_progressive_ptr(png_ptr);
    unsigned char *buf = (unsigned char *)(*buf_ptr);

    if(data)
    {
        memcpy(data, buf, len);
        buf += len;
        *buf_ptr = buf;
    };

    return len;
};

DWORD CodecInst::Decompress(ICDECOMPRESS* icinfo, DWORD dwSize)
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 w, h;
    int bd, ct;

    if(ICERR_OK != DecompressQuery(icinfo->lpbiInput, icinfo->lpbiOutput))
        return ICERR_BADFORMAT;

    /* Allocate the png read struct */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        return ICERR_ERROR;

    /* Allocate the png info struct */
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        log_message("png_create_info_struct() failed\n");
        return ICERR_ERROR;
    };

    /* for proper error handling */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return ICERR_ERROR;
    };

    /* setup output buffer ptr */
    void* buf = icinfo->lpInput;

    png_set_read_fn(png_ptr, &buf, (png_rw_ptr)compressed_data_reader);

    /* Read the info section of the png file */
    png_read_info(png_ptr, info_ptr);

    /* Extract info */
    png_get_IHDR
    (
        png_ptr, info_ptr, 
        &w, &h,
        &bd, &ct,
        NULL, NULL, NULL
    );

    /* check if decoded data is the same as in header */
    if(h != icinfo->lpbiOutput->biHeight || w != icinfo->lpbiOutput->biWidth)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        log_message("png_get_IHDR() returns differ data: h=%d, icinfo->lpbiOutput->biHeight=%d, w=%d, icinfo->lpbiOutput->biWidth=%d, bd=%d, icinfo->lpbiOutput->biBitCount=%d\n",
            h, icinfo->lpbiOutput->biHeight, w, icinfo->lpbiOutput->biWidth, bd, icinfo->lpbiOutput->biBitCount);
        return ICERR_BADFORMAT;
    };
    if(PNG_COLOR_TYPE_RGB != ct && PNG_COLOR_TYPE_RGB_ALPHA != ct)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        log_message("png_get_IHDR() returns unsupported channels count, ct=%d (not PNG_COLOR_TYPE_RGB_ALPHA neigher PNG_COLOR_TYPE_RGB_ALPHA)\n", ct);
        return ICERR_BADFORMAT;
    };

    /* calc line size */
    int line_size = ALIGN4(icinfo->lpbiOutput->biWidth) * (icinfo->lpbiOutput->biBitCount / 8);

    /* calc rows ptrs */
    for(int i = 0; i < icinfo->lpbiOutput->biHeight; i++)
        rows[i] = (unsigned char*)icinfo->lpOutput + line_size * i;

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    /* Read data */
    png_read_image(png_ptr, rows);

    /* Clean up memory */
    png_read_end(png_ptr, NULL);
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);

    /* setup compressed inmage size */
    icinfo->lpbiOutput->biSizeImage = icinfo->lpbiOutput->biHeight * line_size;

    return ICERR_OK;
};
