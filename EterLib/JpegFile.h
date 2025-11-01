#pragma once

#include <stdio.h>

int jpeg_save(unsigned char*data_d, int width, int height, int quality, const char*filename);
int jpeg_save_to_mem(unsigned char*data_d, int width, int height, int quality, unsigned char*dest, int destsize);
int jpeg_load(const char*filename, unsigned char**dest, int*width, int*height);
int jpeg_load_rgba(const char* filename, unsigned char** dest, int* width, int* height);