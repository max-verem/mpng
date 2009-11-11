#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>

#include <png.h>

#ifdef _DEBUG
	#pragma comment(lib, "libpngd.lib")
	#pragma comment(lib, "zlibd.lib")
#else
	#pragma comment(lib, "libpng.lib")
	#pragma comment(lib, "zlib.lib")
#endif

#pragma comment(lib, "WinMM.Lib")

#define ALIGN4(V) (((V + 3) >> 2) << 2)

static void* load_file(char* filename)
{
    FILE* f;
    int s;
    void* buf;

    f = fopen(filename, "rb");
    if(!f) return NULL;

    fseek(f, 0, SEEK_END);
    s = ftell(f);
    fseek(f, 0, SEEK_SET);

    buf = malloc(s);
    if(s)
        fread(buf, 1, s, f);

    fclose(f);

    return buf;
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

int decode_png_frame(void* src_buf)
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 w, h;
    int bd, ct;
    void* buf;
    int line_size;
    unsigned int i;
    unsigned char** rows;

    /* Allocate the png read struct */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        return -1;

    /* Allocate the png info struct */
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return -2;
    };

    /* for proper error handling */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return -3;
    };

    /* setup output buffer ptr */
    buf = src_buf;

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

    /* calc line size */
    line_size = ALIGN4(w) * 4;

    /* calc rows ptrs */
    rows = (unsigned char**)malloc(sizeof(unsigned char*) * h);
    for(i = 0; i < h; i++)
        rows[i] = (unsigned char*)malloc(line_size);

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    /* Read data */
    png_read_image(png_ptr, rows);

    /* Clean up memory */
    png_read_end(png_ptr, NULL);
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);

    for(i = 0; i < h; i++)
        free(rows[i]);
    free(rows);

    return 0;
};

int main(int argc, char** argv)
{
    void* buf;
    int cnt, i, r;
    unsigned int t1, t2;

    timeBeginPeriod(1);

    if(argc != 3)
    {
        fprintf(stderr, "Usage:\n    mpng-benchmark.exe <count> <png file>\n");
        return 1;
    };

    cnt = atol(argv[1]);

    buf = load_file(argv[2]);

    if(!buf)
    {
        fprintf(stderr, "ERROR: failed to load file [%s]\n", argv[2]);
        return 1;
    };

    fprintf(stderr, "Estimating decoding (%d times)....", cnt);

    t1 = timeGetTime();
    for(i = 0, r = 0; i < cnt; i++)
        r = decode_png_frame(buf);
    t2 = timeGetTime();

    if(r)
        fprintf(stderr, "ERROR\n");
    else
    {
        fprintf(stderr, "DONE\n");
        if(t2 != t1)
            fprintf(stderr, "Rate is: %f [fps]\n", 1000.0 * cnt / (float)(t2 - t1));
        else
            fprintf(stderr, "Increase count of probes!\n");
    };

    return 0;
};
