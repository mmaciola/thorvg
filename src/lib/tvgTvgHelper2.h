#ifndef _TVG_STANDARD_HELPER_H_
#define _TVG_STANDARD_HELPER_H_

#include "thorvg.h"

using ByteCounter = uint16_t;
using FlagType = uint8_t;  // flags' casting not needed during saving - loading?
#define TVG_FLAG_SIZE 1   // size of FlagType - to avoid calling sizeof(flags) many times

#define TVG_HEADER_TVG_SIGN_CODE "TVG"
#define TVG_HEADER_TVG_VERSION_CODE "000"
struct tvg_header {
   uint8_t tvg_sign[3]; // Sign phase, always "TVG" declared in TVG_HEADER_TVG_SIGN_CODE
   uint8_t version[3]; // Standard version number, declared in TVG_HEADER_TVG_VERSION_CODE
   uint16_t meta_lenght; // Matadata phase lenght
} __attribute__((packed));

#define TVG_CANVAS_BEGIN_INDICATOR (FlagType)0xff
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

#define TVG_RAW_IMAGE_BEGIN_INDICATOR (FlagType)0xfd
struct tvg_width_height {
   uint32_t width;
   uint32_t height;
};

#define TVG_SCENE_BEGIN_INDICATOR (FlagType)0xfc
struct tvg_scene {
   uint32_t reservedCnt;
   uint8_t opacity;
   tvg::Matrix matrix;
} __attribute__((packed));


#define TVG_SHAPE_BEGIN_INDICATOR (FlagType)0xfb

#define TVG_SHAPE_FLAG_HAS_STROKE           (FlagType)0b00000001 // If set, shape has stroke section
#define TVG_SHAPE_FLAG_HAS_FILL             (FlagType)0b00000010 // If set, shape has fill
#define TVG_SHAPE_FLAG_HAS_COLOR            (FlagType)0b00000100 // If set, shape has color.
#define TVG_SHAPE_FLAG_HAS_PATH             (FlagType)0b00001000 // If set, shape has stroke section
#define TVG_SHAPE_FLAG_HAS_TRANSFORM_MATRIX (FlagType)0b00010000 // If set, has transform matrix

#define TVG_SHAPE_FLAG_MASK_FILLRULE        (FlagType)0b00010000 // mask for shape FillRule            // MGS - just for clarity
#define TVG_SHAPE_FLAG_FILLRULE_EVENODD     (FlagType)0b00010000 // FillRule::EvenOdd
#define TVG_SHAPE_FLAG_FILLRULE_WINDING     (FlagType)0b00000000 // FillRule::Winding

#define TVG_STROKE_FLAG_MASK_CAP   (FlagType)0b00000011 // mask for stroke StrokeCap
#define TVG_STROKE_FLAG_CAP_SQUARE (FlagType)0b00000001 // StrokeCap::Square
#define TVG_STROKE_FLAG_CAP_ROUND  (FlagType)0b00000010 // StrokeCap::Round
#define TVG_STROKE_FLAG_CAP_BUTT   (FlagType)0b00000011 // StrokeCap::Butt

#define TVG_STROKE_FLAG_MASK_JOIN  (FlagType)0b00001100 // mask for stroke StrokeJoin
#define TVG_STROKE_FLAG_JOIN_BEVEL (FlagType)0b00000100 // StrokeJoin::Bevel
#define TVG_STROKE_FLAG_JOIN_ROUND (FlagType)0b00001000 // StrokeJoin::Round
#define TVG_STROKE_FLAG_JOIN_MITER (FlagType)0b00001100 // StrokeJoin::Miter

#define TVG_STROKE_FLAG_HAS_WIDTH  (FlagType)0b00010000 // stroke width flag
#define TVG_STROKE_FLAG_HAS_FILL   (FlagType)0b00100000 // stroke fill flag
#define TVG_STROKE_FLAG_HAS_COLOR  (FlagType)0b01000000 // stroke color flag
#define TVG_STROKE_FLAG_HAS_DASH   (FlagType)0b10000000 // dashed stroke flag
struct tvg_shape_stroke {
   uint8_t flags;
   float width;
   union {
      uint8_t color[4];
      uint32_t fillid;
   };
} __attribute__((packed));

#define TVG_FILL_FLAG_COLORSTOPS          (FlagType)0b00000001 // fill's colorStops flag
#define TVG_FILL_FLAG_MASK_FILLSPREAD     (FlagType)0b00000110 // mask for fill spread
#define TVG_FILL_FLAG_FILLSPREAD_PAD      (FlagType)0b00000010 // FillSpread::Pad
#define TVG_FILL_FLAG_FILLSPREAD_REFLECT  (FlagType)0b00000100 // FillSpread::Reflect
#define TVG_FILL_FLAG_FILLSPREAD_REPEAT   (FlagType)0b00000110 // FillSpread::Repeat

#define TVG_GRADIENT_FLAG_MASK_GRADTYPE   (FlagType)0b00000001 // mask for gradient type           // MGS - just for clarity
#define TVG_GRADIENT_FLAG_TYPE_LINEAR     (FlagType)0b00000000 // linear gradient flag
#define TVG_GRADIENT_FLAG_TYPE_RADIAL     (FlagType)0b00000001 // radial gradient flag

#define TVG_PATH_FLAG_CMDS (FlagType)0b00000001 // path's commands flag
#define TVG_PATH_FLAG_PTS  (FlagType)0b00000010 // path's points flag

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


// usuniete
#define TVG_GRADIENT_BEGIN_INDICATOR (FlagType)0xfe
#define TVG_STROKE_FLAG_HAS_DASH_PATTERN 0b00100000 // If set, stroke has dashPattern
#define TVG_GRADIENT_FLAG_MASK_FILL_SPREAD 0b00000110 // mask for fill spread
#define TVG_GRADIENT_FLAG_FILL_SPREAD_PAD 0b00000010 // FillSpread::Pad
#define TVG_GRADIENT_FLAG_FILL_SPREAD_REFLECT 0b00000100 // FillSpread::Reflect
#define TVG_GRADIENT_FLAG_FILL_SPREAD_REPEAT 0b00000110 // FillSpread::Repeat


#endif //_TVG_STANDARD_HELPER_H_
