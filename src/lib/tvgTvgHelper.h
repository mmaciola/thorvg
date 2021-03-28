#ifndef _TVG_STANDARD_HELPER_H_
#define _TVG_STANDARD_HELPER_H_

#include "tvgCommon.h"

using TvgFlag = uint8_t;
#define TVG_FLAG_SIZE 1
using TvgIndicator = uint8_t;
#define TVG_INDICATOR_SIZE 1
using ByteCounter = uint32_t;
#define BYTE_COUNTER_SIZE 4

struct tvg_block {
   TvgIndicator type;
   ByteCounter lenght;
   const char * data;
   const char * block_end;
};

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || defined(__BIG_ENDIAN__)
// big endian
#define _read_tvg_ui16(dst, src) *dst = ((((src)[0] & 0xff) << 8) | ((src)[1] & 0xff))
#define _read_tvg_ui32(dst, src) *dst = ((((src)[0] & 0xff) << 24) | (((src)[1] & 0xff) << 16) | (((src)[2] & 0xff) << 8) | ((src)[3] & 0xff))
#define _read_tvg_float(dst, src) {char *r = (char*)dst;r[0]=src[3];r[1]=src[2];r[2]=src[1];r[3]=src[0];} // TODO
#else
// little endian
#define _read_tvg_ui16(dst, src) memcpy(dst, (src), sizeof(uint16_t))
#define _read_tvg_ui32(dst, src) memcpy(dst, (src), sizeof(uint32_t))
#define _read_tvg_float(dst, src) memcpy(dst, (src), sizeof(float))
#endif

#define TVG_HEADER_TVG_SIGN_CODE "TVG"
#define TVG_HEADER_TVG_VERSION_CODE "000"

#define TVG_SCENE_BEGIN_INDICATOR    (TvgIndicator)0xfe
#define TVG_SHAPE_BEGIN_INDICATOR    (TvgIndicator)0xfd
#define TVG_PICTURE_BEGIN_INDICATOR  (TvgIndicator)0xfc
#define TVG_RAW_IMAGE_BEGIN_INDICATOR (TvgIndicator)0x50

#define TVG_PAINT_FLAG_HAS_OPACITY          (TvgIndicator)0x01
#define TVG_PAINT_FLAG_HAS_TRANSFORM_MATRIX (TvgIndicator)0x02
#define TVG_SCENE_FLAG_RESERVEDCNT          (TvgIndicator)0x03
#define TVG_PAINT_FLAG_HAS_CMP_TARGET       (TvgIndicator)0x04

#define TVG_PAINT_FLAG_CMP_METHOD          (TvgIndicator)0x10
#define TVG_PAINT_FLAG_CMP_METHOD_CLIPPATH      (TvgFlag)0x01
#define TVG_PAINT_FLAG_CMP_METHOD_ALPHAMASK     (TvgFlag)0x02
#define TVG_PAINT_FLAG_CMP_METHOD_INV_ALPHAMASK (TvgFlag)0x03

#define TVG_SHAPE_FLAG_HAS_PATH    (TvgIndicator)0x20 // If set, shape has path section
#define TVG_SHAPE_FLAG_HAS_STROKE  (TvgIndicator)0x21 // If set, shape has stroke section
#define TVG_SHAPE_FLAG_HAS_FILL    (TvgIndicator)0x22 // If set, shape has fill section
#define TVG_SHAPE_FLAG_COLOR       (TvgIndicator)0x23 // If set, shape has color.

#define TVG_SHAPE_FLAG_FILLRULE        (TvgIndicator)0x24 // for shape FillRule
#define TVG_SHAPE_FLAG_FILLRULE_WINDING     (TvgFlag)0x01 // FillRule::Winding
#define TVG_SHAPE_FLAG_FILLRULE_EVENODD     (TvgFlag)0x02 // FillRule::EvenOdd

#define TVG_SHAPE_STROKE_FLAG_CAP   (TvgIndicator)0x30 // for stroke StrokeCap
#define TVG_SHAPE_STROKE_FLAG_CAP_SQUARE (TvgFlag)0x01 // StrokeCap::Square
#define TVG_SHAPE_STROKE_FLAG_CAP_ROUND  (TvgFlag)0x02 // StrokeCap::Round
#define TVG_SHAPE_STROKE_FLAG_CAP_BUTT   (TvgFlag)0x03 // StrokeCap::Butt

#define TVG_SHAPE_STROKE_FLAG_JOIN  (TvgIndicator)0x31 // mask for stroke StrokeJoin
#define TVG_SHAPE_STROKE_FLAG_JOIN_BEVEL (TvgFlag)0x01 // StrokeJoin::Bevel
#define TVG_SHAPE_STROKE_FLAG_JOIN_ROUND (TvgFlag)0x02 // StrokeJoin::Round
#define TVG_SHAPE_STROKE_FLAG_JOIN_MITER (TvgFlag)0x03 // StrokeJoin::Miter

#define TVG_SHAPE_STROKE_FLAG_WIDTH        (TvgIndicator)0x32 // stroke width flag
#define TVG_SHAPE_STROKE_FLAG_COLOR        (TvgIndicator)0x33 // stroke color flag
#define TVG_SHAPE_STROKE_FLAG_HAS_FILL     (TvgIndicator)0x34 // stroke fill flag
#define TVG_SHAPE_STROKE_FLAG_HAS_DASHPTRN (TvgIndicator)0x35 // dashed stroke flag

#define TVG_GRADIENT_FLAG_TYPE      (TvgIndicator)0x40 // mask for gradient type
#define TVG_GRADIENT_FLAG_TYPE_LINEAR    (TvgFlag)0x01 // linear gradient flag
#define TVG_GRADIENT_FLAG_TYPE_RADIAL    (TvgFlag)0x02 // radial gradient flag
#define TVG_FILL_FLAG_COLORSTOPS    (TvgIndicator)0x41 // fill's colorStops flag
#define TVG_FILL_FLAG_FILLSPREAD    (TvgIndicator)0x42 // mask for fill spread
#define TVG_FILL_FLAG_FILLSPREAD_PAD     (TvgFlag)0x01 // FillSpread::Pad
#define TVG_FILL_FLAG_FILLSPREAD_REFLECT (TvgFlag)0x02 // FillSpread::Reflect
#define TVG_FILL_FLAG_FILLSPREAD_REPEAT  (TvgFlag)0x03 // FillSpread::Repeat


#endif //_TVG_STANDARD_HELPER_H_
