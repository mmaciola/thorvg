#ifndef _TVG_STANDARD_HELPER_H_
#define _TVG_STANDARD_HELPER_H_

#include "thorvg.h"

using FlagType = uint8_t;
using ByteCounter = uint16_t;

#define TVG_HEADER_TVG_SIGN_CODE "TVG"
#define TVG_HEADER_TVG_VERSION_CODE "000"
struct tvg_header {
   uint8_t tvg_sign[3]; // Sign phase, always "TVG" declared in TVG_HEADER_TVG_SIGN_CODE
   uint8_t version[3]; // Standard version number, declared in TVG_HEADER_TVG_VERSION_CODE
   uint16_t meta_lenght; // Matadata phase lenght
} __attribute__((packed));

#define TVG_SCENE_BEGIN_INDICATOR (FlagType)0xfe
#define TVG_SHAPE_BEGIN_INDICATOR (FlagType)0xfd
#define TVG_PICTURE_BEGIN_INDICATOR (FlagType)0xfc

#define TVG_SHAPE_FLAG_HAS_PATH    (FlagType)0x01 // If set, shape has path section
#define TVG_SHAPE_FLAG_HAS_STROKE  (FlagType)0x02 // If set, shape has stroke section
#define TVG_SHAPE_FLAG_HAS_FILL    (FlagType)0x03 // If set, shape has fill section
#define TVG_SHAPE_FLAG_COLOR   (FlagType)0x04 // If set, shape has color.

#define TVG_SHAPE_FLAG_FILLRULE        (FlagType)0x05 // mask for shape FillRule
#define TVG_SHAPE_FLAG_FILLRULE_EVENODD     0x01 // FillRule::EvenOdd
#define TVG_SHAPE_FLAG_FILLRULE_WINDING     0x02 // FillRule::Winding

#define TVG_SHAPE_STROKE_FLAG_CAP   (FlagType)0x06 // mask for stroke StrokeCap
#define TVG_SHAPE_STROKE_FLAG_CAP_SQUARE 0x01 // StrokeCap::Square
#define TVG_SHAPE_STROKE_FLAG_CAP_ROUND  0x02 // StrokeCap::Round
#define TVG_SHAPE_STROKE_FLAG_CAP_BUTT   0x03 // StrokeCap::Butt

#define TVG_SHAPE_STROKE_FLAG_JOIN  (FlagType)0x07 // mask for stroke StrokeJoin
#define TVG_SHAPE_STROKE_FLAG_JOIN_BEVEL 0x01 // StrokeJoin::Bevel
#define TVG_SHAPE_STROKE_FLAG_JOIN_ROUND 0x02 // StrokeJoin::Round
#define TVG_SHAPE_STROKE_FLAG_JOIN_MITER 0x03 // StrokeJoin::Miter

#define TVG_SHAPE_STROKE_FLAG_WIDTH  (FlagType)0x08 // stroke width flag
#define TVG_SHAPE_STROKE_FLAG_COLOR  (FlagType)0x09 // stroke color flag
#define TVG_SHAPE_STROKE_FLAG_HAS_FILL   (FlagType)0x10 // stroke fill flag
#define TVG_SHAPE_STROKE_FLAG_HAS_DASHPTRN   (FlagType)0x11 // dashed stroke flag

#define TVG_FILL_FLAG_COLORSTOPS          (FlagType)0b00000001 // fill's colorStops flag
#define TVG_FILL_FLAG_FILLSPREAD     (FlagType)0b00000110 // mask for fill spread
#define TVG_FILL_FLAG_FILLSPREAD_PAD      0x01 // FillSpread::Pad
#define TVG_FILL_FLAG_FILLSPREAD_REFLECT  0x02 // FillSpread::Reflect
#define TVG_FILL_FLAG_FILLSPREAD_REPEAT   0x03 // FillSpread::Repeat

#define TVG_GRADIENT_FLAG_TYPE   (FlagType)0b00000001 // mask for gradient type
#define TVG_GRADIENT_FLAG_TYPE_LINEAR     (FlagType)0b00000000 // linear gradient flag
#define TVG_GRADIENT_FLAG_TYPE_RADIAL     (FlagType)0b00000001 // radial gradient flag

#define TVG_PATH_FLAG_CMDS (FlagType)0b00000001 // path's commands flag
#define TVG_PATH_FLAG_PTS  (FlagType)0b00000010 // path's points flag

#define TVG_RAW_IMAGE_BEGIN_INDICATOR (FlagType)0xfd

#endif //_TVG_STANDARD_HELPER_H_
