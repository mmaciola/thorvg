#ifndef _TVG_STANDARD_HELPER_H_
#define _TVG_STANDARD_HELPER_H_

#include "tvgCommon.h"

// now only little endian
#define _read_tvg_ui16(dst, src) memcpy(dst, (src), sizeof(uint16_t))
#define _read_tvg_ui32(dst, src) memcpy(dst, (src), sizeof(uint32_t))
#define _read_tvg_float(dst, src) memcpy(dst, (src), sizeof(float))

#define TVG_HEADER_TVG_SIGN_CODE "TVG"
#define TVG_HEADER_TVG_VERSION_CODE "000"

#define TVG_SCENE_BEGIN_INDICATOR     (TvgIndicator)0xfe
#define TVG_SHAPE_BEGIN_INDICATOR     (TvgIndicator)0xfd
#define TVG_PICTURE_BEGIN_INDICATOR   (TvgIndicator)0xfc
#define TVG_RAW_IMAGE_BEGIN_INDICATOR (TvgIndicator)0x50

#define TVG_PAINT_OPACITY_INDICATOR          (TvgIndicator)0x01
#define TVG_PAINT_TRANSFORM_MATRIX_INDICATOR (TvgIndicator)0x02
#define TVG_SCENE_FLAG_RESERVEDCNT           (TvgIndicator)0x03
#define TVG_PAINT_CMP_TARGET_INDICATOR       (TvgIndicator)0x04

#define TVG_PAINT_CMP_METHOD_INDICATOR     (TvgIndicator)0x10
#define TVG_PAINT_CMP_METHOD_CLIPPATH_FLAG      (TvgFlag)0x01
#define TVG_PAINT_CMP_METHOD_ALPHAMASK_FLAG     (TvgFlag)0x02
#define TVG_PAINT_CMP_METHOD_INV_ALPHAMASK_FLAG (TvgFlag)0x03

#define TVG_SHAPE_PATH_INDICATOR    (TvgIndicator)0x20 // If set, shape has path section
#define TVG_SHAPE_STROKE_INDICATOR  (TvgIndicator)0x21 // If set, shape has stroke section
#define TVG_SHAPE_FILL_INDICATOR    (TvgIndicator)0x22 // If set, shape has fill section
#define TVG_SHAPE_COLOR_INDICATOR   (TvgIndicator)0x23 // If set, shape has color.

#define TVG_SHAPE_FILLRULE_INDICATOR    (TvgIndicator)0x24 // for shape FillRule
#define TVG_SHAPE_FILLRULE_WINDING_FLAG      (TvgFlag)0x01 // FillRule::Winding
#define TVG_SHAPE_FILLRULE_EVENODD_FLAG      (TvgFlag)0x02 // FillRule::EvenOdd

#define TVG_SHAPE_STROKE_CAP_INDICATOR  (TvgIndicator)0x30 // for stroke StrokeCap
#define TVG_SHAPE_STROKE_CAP_SQUARE_FLAG     (TvgFlag)0x01 // StrokeCap::Square
#define TVG_SHAPE_STROKE_CAP_ROUND_FLAG      (TvgFlag)0x02 // StrokeCap::Round
#define TVG_SHAPE_STROKE_CAP_BUTT_FLAG       (TvgFlag)0x03 // StrokeCap::Butt

#define TVG_SHAPE_STROKE_JOIN_INDICATOR (TvgIndicator)0x31 // mask for stroke StrokeJoin
#define TVG_SHAPE_STROKE_JOIN_BEVEL_FLAG     (TvgFlag)0x01 // StrokeJoin::Bevel
#define TVG_SHAPE_STROKE_JOIN_ROUND_FLAG     (TvgFlag)0x02 // StrokeJoin::Round
#define TVG_SHAPE_STROKE_JOIN_MITER_FLAG     (TvgFlag)0x03 // StrokeJoin::Miter

#define TVG_SHAPE_STROKE_WIDTH_INDICATOR    (TvgIndicator)0x32 // stroke width flag
#define TVG_SHAPE_STROKE_COLOR_INDICATOR    (TvgIndicator)0x33 // stroke color flag
#define TVG_SHAPE_STROKE_FILL_INDICATOR     (TvgIndicator)0x34 // stroke fill flag
#define TVG_SHAPE_STROKE_DASHPTRN_INDICATOR (TvgIndicator)0x35 // dashed stroke flag

#define TVG_FILL_LINEAR_GRADIENT_INDICATOR  (TvgIndicator)0x01 // linear gradient flag
#define TVG_FILL_RADIAL_GRADIENT_INDICATOR  (TvgIndicator)0x02 // radial gradient flag
#define TVG_FILL_COLORSTOPS_INDICATOR       (TvgIndicator)0x41 // fill's colorStops flag
#define TVG_FILL_FILLSPREAD_INDICATOR       (TvgIndicator)0x42 // mask for fill spread
#define TVG_FILL_FILLSPREAD_PAD_FLAG             (TvgFlag)0x01 // FillSpread::Pad
#define TVG_FILL_FILLSPREAD_REFLECT_FLAG         (TvgFlag)0x02 // FillSpread::Reflect
#define TVG_FILL_FILLSPREAD_REPEAT_FLAG          (TvgFlag)0x03 // FillSpread::Repeat


#endif //_TVG_STANDARD_HELPER_H_
