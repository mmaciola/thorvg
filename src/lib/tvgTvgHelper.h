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

};

#define TVG_GRADIENT_BEGIN_INDICATOR 0xfe
#define TVG_GRADIENT_FLAG_TYPE_RADIAL 0b00000001 // if set- radial gradient, if clear- linear.
struct tvg_gradient {
   uint8_t flags;
};

#endif //_TVG_STANDARD_HELPER_H_
