#ifndef EXR_H
#define EXR_H

#include <memory>

/// Simple RGBA struct
struct RGBA {
   float r;
   float g;
   float b;
   float a;
};


std::unique_ptr<RGBA[]> read_rgba_exr(const char* filename, int& width, int& height);
void write_rgba_exr(const char* filename, int width, int height, RGBA* pixels);

#endif
