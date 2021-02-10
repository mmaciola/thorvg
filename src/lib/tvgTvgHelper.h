#ifndef _TVG_STANDARD_HELPER_H_
#define _TVG_STANDARD_HELPER_H_

#define TVG_HEADER_TVG_SIGN_CODE "TVG"
#define TVG_HEADER_TVG_VERSION_CODE "000"
struct tvg_header {
   uint8_t tvg_sign[3]; // Sign phase, always "TVG" declared in TVG_HEADER_TVG_SIGN_CODE
   uint8_t version[3]; // Standard version number, declared in TVG_HEADER_TVG_VERSION_CODE
   uint16_t meta_lenght; // Matadata phase lenght
};

#define TVG_CANVAS_BEGIN_INDICATOR 0xff
#define TVG_CANVAS_FLAG_HAVE_SIZE 0b00000001
struct tvg_canvas {
   uint8_t flags;
   uint32_t width;
   uint32_t height;
};

struct tvg_flags_and_id {
   uint8_t flags;
   uint32_t id;
};
#define TVG_GRADIENT_BEGIN_INDICATOR 0xfe
#define TVG_GRADIENT_FLAG_TYPE_RADIAL 0b00000001 // if set- radial gradient, if clear- linear.
struct tvg_gradient_linear {
   float x1;
   float y1;
   float x2;
   float y2;
};
struct tvg_gradient_radial {
   float x;
   float y;
   float radius;
};

#endif //_TVG_STANDARD_HELPER_H_
