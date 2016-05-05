#include "exr.h"
#include <OpenEXR/ImfRgbaFile.h>

/// Opens the given exr file and returns a pixel array
/// \return An array of pixels
/// \param filename The exr file to open
/// \param width Width of the image will be stored here
/// \param height Height of the image will be stored here
std::unique_ptr<RGBA[]> read_rgba_exr(const char* filename, int& width, int& height) {
   Imf::RgbaInputFile file(filename);
   Imath::Box2i dw = file.dataWindow();
   width = dw.max.x - dw.min.x + 1;
   height = dw.max.y - dw.min.y + 1;
   std::unique_ptr<RGBA[]> pixels(new RGBA[width * height]);
   file.setFrameBuffer((Imf::Rgba*)pixels.get() - dw.min.x - dw.min.y * width, 1, width);
   file.readPixels(dw.min.y, dw.max.y);
   return pixels;
} 

void write_rgba_exr(const char* filename, int width, int height, RGBA* pixels) {
   Imf::RgbaOutputFile file(filename, width, height, Imf::WRITE_RGBA);
   file.setFrameBuffer((Imf::Rgba*)pixels, 1, width);
   file.writePixels(height);
}

