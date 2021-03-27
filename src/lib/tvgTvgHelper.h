#ifndef _TVG_STANDARD_HELPER_H_
#define _TVG_STANDARD_HELPER_H_

#include "tvgCommon.h"

// now only little endian
#define _read_tvg_ui16(dst, src) memcpy(dst, (src), sizeof(uint16_t))
#define _read_tvg_ui32(dst, src) memcpy(dst, (src), sizeof(uint32_t))
#define _read_tvg_float(dst, src) memcpy(dst, (src), sizeof(float))

#define TVG_HEADER_TVG_SIGN_CODE "TVG"
#define TVG_HEADER_TVG_VERSION_CODE "000"

#define TVG_SCENE_BEGIN_INDICATOR    (IndicatorType)0xfe
#define TVG_SHAPE_BEGIN_INDICATOR    (IndicatorType)0xfd
#define TVG_PICTURE_BEGIN_INDICATOR  (IndicatorType)0xfc
#define TVG_RAW_IMAGE_BEGIN_INDICATOR (IndicatorType)0x50

#define TVG_PAINT_FLAG_HAS_OPACITY          (IndicatorType)0x01
#define TVG_PAINT_FLAG_HAS_TRANSFORM_MATRIX (IndicatorType)0x02
#define TVG_SCENE_FLAG_RESERVEDCNT          (IndicatorType)0x03
#define TVG_PAINT_FLAG_HAS_CMP_TARGET       (IndicatorType)0x04

#define TVG_PAINT_FLAG_CMP_METHOD          (IndicatorType)0x10
#define TVG_PAINT_FLAG_CMP_METHOD_CLIPPATH      (FlagType)0x01
#define TVG_PAINT_FLAG_CMP_METHOD_ALPHAMASK     (FlagType)0x02
#define TVG_PAINT_FLAG_CMP_METHOD_INV_ALPHAMASK (FlagType)0x03

#define TVG_SHAPE_FLAG_HAS_PATH    (IndicatorType)0x20 // If set, shape has path section
#define TVG_SHAPE_FLAG_HAS_STROKE  (IndicatorType)0x21 // If set, shape has stroke section
#define TVG_SHAPE_FLAG_HAS_FILL    (IndicatorType)0x22 // If set, shape has fill section
#define TVG_SHAPE_FLAG_COLOR       (IndicatorType)0x23 // If set, shape has color.

#define TVG_SHAPE_FLAG_FILLRULE        (IndicatorType)0x24 // for shape FillRule
#define TVG_SHAPE_FLAG_FILLRULE_WINDING     (FlagType)0x01 // FillRule::Winding
#define TVG_SHAPE_FLAG_FILLRULE_EVENODD     (FlagType)0x02 // FillRule::EvenOdd

#define TVG_SHAPE_STROKE_FLAG_CAP   (IndicatorType)0x30 // for stroke StrokeCap
#define TVG_SHAPE_STROKE_FLAG_CAP_SQUARE (FlagType)0x01 // StrokeCap::Square
#define TVG_SHAPE_STROKE_FLAG_CAP_ROUND  (FlagType)0x02 // StrokeCap::Round
#define TVG_SHAPE_STROKE_FLAG_CAP_BUTT   (FlagType)0x03 // StrokeCap::Butt

#define TVG_SHAPE_STROKE_FLAG_JOIN  (IndicatorType)0x31 // mask for stroke StrokeJoin
#define TVG_SHAPE_STROKE_FLAG_JOIN_BEVEL (FlagType)0x01 // StrokeJoin::Bevel
#define TVG_SHAPE_STROKE_FLAG_JOIN_ROUND (FlagType)0x02 // StrokeJoin::Round
#define TVG_SHAPE_STROKE_FLAG_JOIN_MITER (FlagType)0x03 // StrokeJoin::Miter

#define TVG_SHAPE_STROKE_FLAG_WIDTH        (IndicatorType)0x32 // stroke width flag
#define TVG_SHAPE_STROKE_FLAG_COLOR        (IndicatorType)0x33 // stroke color flag
#define TVG_SHAPE_STROKE_FLAG_HAS_FILL     (IndicatorType)0x34 // stroke fill flag
#define TVG_SHAPE_STROKE_FLAG_HAS_DASHPTRN (IndicatorType)0x35 // dashed stroke flag

#define TVG_GRADIENT_FLAG_TYPE      (IndicatorType)0x40 // mask for gradient type
#define TVG_GRADIENT_FLAG_TYPE_LINEAR    (FlagType)0x01 // linear gradient flag
#define TVG_GRADIENT_FLAG_TYPE_RADIAL    (FlagType)0x02 // radial gradient flag
#define TVG_FILL_FLAG_COLORSTOPS    (IndicatorType)0x41 // fill's colorStops flag
#define TVG_FILL_FLAG_FILLSPREAD    (IndicatorType)0x42 // mask for fill spread
#define TVG_FILL_FLAG_FILLSPREAD_PAD     (FlagType)0x01 // FillSpread::Pad
#define TVG_FILL_FLAG_FILLSPREAD_REFLECT (FlagType)0x02 // FillSpread::Reflect
#define TVG_FILL_FLAG_FILLSPREAD_REPEAT  (FlagType)0x03 // FillSpread::Repeat


#endif //_TVG_STANDARD_HELPER_H_
