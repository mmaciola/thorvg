#ifndef _TVG_STANDARD_HELPER_H_
#define _TVG_STANDARD_HELPER_H_

#include "thorvg.h"

#define TVG_HEADER_TVG_SIGN_CODE "TVG"
#define TVG_HEADER_TVG_VERSION_CODE "000"
struct tvg_header {
   uint8_t tvg_sign[3]; // Sign phase, always "TVG" declared in TVG_HEADER_TVG_SIGN_CODE
   uint8_t version[3]; // Standard version number, declared in TVG_HEADER_TVG_VERSION_CODE
   uint16_t meta_lenght; // Matadata phase lenght
} __attribute__((packed));

#define TVG_CANVAS_BEGIN_INDICATOR (char)0xff
#define TVG_CANVAS_FLAG_HAVE_SIZE 0b00000001
struct tvg_canvas {
   uint8_t flags;
   uint32_t width;
   uint32_t height;
} __attribute__((packed));

struct tvg_flags_and_id {
   uint8_t flags;
   uint32_t id;
} __attribute__((packed));

#define TVG_GRADIENT_BEGIN_INDICATOR (char)0xfe
#define TVG_GRADIENT_FLAG_TYPE_RADIAL 0b00000001 // if set- radial gradient, if clear- linear.
#define TVG_GRADIENT_FLAG_MASK_FILL_SPREAD 0b00000110 // mask for fill spread
#define TVG_GRADIENT_FLAG_FILL_SPREAD_PAD 0b00000010 // FillSpread::Pad
#define TVG_GRADIENT_FLAG_FILL_SPREAD_REFLECT 0b00000100 // FillSpread::Reflect
#define TVG_GRADIENT_FLAG_FILL_SPREAD_REPEAT 0b00000110 // FillSpread::Repeat
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

#define TVG_RAW_IMAGE_BEGIN_INDICATOR (char)0xfd
struct tvg_width_height {
   uint32_t width;
   uint32_t height;
};

#define TVG_SCENE_BEGIN_INDICATOR (char)0xfc

#define TVG_PAINT_FLAG_HAS_OPACITY (char)0b00000001
#define TVG_PAINT_FLAG_HAS_TRANSFORM_MATRIX (char)0b00000010

#define TVG_SHAPE_BEGIN_INDICATOR (char)0xfb
#define TVG_SHAPE_FLAG_MASK_FILLRULE (char)0b00000001 // FillRule, if set FillRule::EvenOdd, else FillRule::Winding
#define TVG_SHAPE_FLAG_HAS_STROKE (char)0b00000010 // If set, has stroke section
#define TVG_SHAPE_FLAG_HAS_FILL (char)0b00000100 // If set, has fillid. If clear, has color.

#define TVG_STROKE_FLAG_MASK_CAP 0b00000011 // mask for stroke StrokeCap
#define TVG_STROKE_FLAG_CAP_SQUARE 0b00000001 // StrokeCap::Square
#define TVG_STROKE_FLAG_CAP_ROUND 0b00000010 // StrokeCap::Round
#define TVG_STROKE_FLAG_CAP_BUTT 0b00000011 // StrokeCap::Butt
#define TVG_STROKE_FLAG_MASK_JOIN 0b00001100 // mask for stroke StrokeJoin
#define TVG_STROKE_FLAG_JOIN_BEVEL 0b00000100 // StrokeJoin::Bevel
#define TVG_STROKE_FLAG_JOIN_ROUND 0b00001000 // StrokeJoin::Round
#define TVG_STROKE_FLAG_JOIN_MITER 0b00001100 // StrokeJoin::Miter
#define TVG_STROKE_FLAG_HAS_FILL 0b00010000 // flag for using fill. If set- fillid (gradient), if clear- color.
#define TVG_STROKE_FLAG_HAS_DASH_PATTERN 0b00100000 // If set, stroke has dashPattern
struct tvg_shape_stroke {
   uint8_t flags;
   float width;
   union {
      uint8_t color[4];
      uint32_t fillid;
   };
} __attribute__((packed));








#endif //_TVG_STANDARD_HELPER_H_