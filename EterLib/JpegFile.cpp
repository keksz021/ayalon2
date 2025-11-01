#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <libjpeg-turbo/turbojpeg.h>

int jpeg_save(unsigned char* data_rgb, int width, int height, int quality, const char* filename)
{
    tjhandle handle = tjInitCompress();
    if (!handle)
    {
        fprintf(stderr, "Failed to init TurboJPEG compressor\n");
        return 0;
    }

    unsigned char* jpeg_buf = NULL;
    unsigned long jpeg_size = 0;

    int ret = tjCompress2(handle,
        data_rgb,
        width,
        0,
        height,
        TJPF_RGB,
        &jpeg_buf,
        &jpeg_size,
        TJSAMP_420,
        quality,
        TJFLAG_FASTDCT);
    if (ret != 0)
    {
        fprintf(stderr, "TurboJPEG compression failed: %s\n", tjGetErrorStr());
        tjDestroy(handle);
        return 0;
    }

    FILE* out = fopen(filename, "wb");
    if (!out)
    {
        fprintf(stderr, "Failed to open output file %s\n", filename);
        tjFree(jpeg_buf);
        tjDestroy(handle);
        return 0;
    }

    fwrite(jpeg_buf, 1, jpeg_size, out);
    fclose(out);

    tjFree(jpeg_buf);
    tjDestroy(handle);
    return 1;
}

int jpeg_save_to_mem(unsigned char* data_d, int width, int height, int quality, unsigned char* _dest, int _destlen)
{
    tjhandle handle = tjInitCompress();
    if (!handle)
    {
        fprintf(stderr, "Failed to init turbojpeg compressor\n");
        return 0;
    }

    unsigned char* jpeg_buf = NULL;
    unsigned long jpeg_size = 0;

    int ret = tjCompress2(
        handle,
        data_d,
        width,
        0,
        height,
        TJPF_RGB,
        &jpeg_buf,
        &jpeg_size,
        TJSAMP_420,
        quality,
        TJFLAG_FASTDCT
    );

    if (ret != 0)
    {
        fprintf(stderr, "TurboJPEG compression failed: %s\n", tjGetErrorStr());
        tjDestroy(handle);
        return 0;
    }

    if ((int)jpeg_size > _destlen)
    {
        fprintf(stderr, "Buffer prea mic: jpeg_size=%lu > _destlen=%d\n", jpeg_size, _destlen);
        tjFree(jpeg_buf);
        tjDestroy(handle);
        return 0;
    }

    memcpy(_dest, jpeg_buf, jpeg_size);
    tjFree(jpeg_buf);
    tjDestroy(handle);

    return (int)jpeg_size;
}

int jpeg_load(const char* filename, unsigned char** out_rgb_buf, int* out_width, int* out_height)
{
    FILE* fi = fopen(filename, "rb");
    if (!fi)
    {
        fprintf(stderr, "Failed to open %s\n", filename);
        return 0;
    }

    fseek(fi, 0, SEEK_END);
    long filesize = ftell(fi);
    fseek(fi, 0, SEEK_SET);

    unsigned char* jpeg_buf = (unsigned char*)malloc(filesize);
    if (!jpeg_buf)
    {
        fprintf(stderr, "Out of memory\n");
        fclose(fi);
        return 0;
    }

    fread(jpeg_buf, 1, filesize, fi);
    fclose(fi);

    tjhandle handle = tjInitDecompress();
    if (!handle)
    {
        fprintf(stderr, "Failed to init TurboJPEG decompressor\n");
        free(jpeg_buf);
        return 0;
    }

    int w, h, jpegSubsamp, jpegColorspace;
    if (tjDecompressHeader3(handle, jpeg_buf, filesize, &w, &h, &jpegSubsamp, &jpegColorspace) != 0)
    {
        fprintf(stderr, "Failed to read JPEG header: %s\n", tjGetErrorStr());
        tjDestroy(handle);
        free(jpeg_buf);
        return 0;
    }

    unsigned char* rgb_buf = (unsigned char*)malloc(w * h * 3);
    if (!rgb_buf)
    {
        fprintf(stderr, "Out of memory\n");
        tjDestroy(handle);
        free(jpeg_buf);
        return 0;
    }

    if (tjDecompress2(handle, jpeg_buf, filesize, rgb_buf, w, 0, h, TJPF_RGB, 0) != 0)
    {
        fprintf(stderr, "Decompression failed: %s\n", tjGetErrorStr());
        free(rgb_buf);
        tjDestroy(handle);
        free(jpeg_buf);
        return 0;
    }

    *out_rgb_buf = rgb_buf;
    *out_width = w;
    *out_height = h;

    tjDestroy(handle);
    free(jpeg_buf);

    return 1;
}

int jpeg_load_rgba(const char* filename, unsigned char** out_rgba_buf, int* out_width, int* out_height)
{
    unsigned char* rgb_buf = NULL;
    int w, h;

    if (!jpeg_load(filename, &rgb_buf, &w, &h))
        return 0;

    unsigned char* rgba_buf = (unsigned char*)malloc(w * h * 4);
    if (!rgba_buf)
    {
        fprintf(stderr, "Out of memory\n");
        free(rgb_buf);
        return 0;
    }

    for (int i = 0; i < w * h; i++)
    {
        rgba_buf[4 * i + 0] = rgb_buf[3 * i + 0];
        rgba_buf[4 * i + 1] = rgb_buf[3 * i + 1];
        rgba_buf[4 * i + 2] = rgb_buf[3 * i + 2];
        rgba_buf[4 * i + 3] = 255;
    }

    free(rgb_buf);

    *out_rgba_buf = rgba_buf;
    *out_width = w;
    *out_height = h;

    return 1;
}
