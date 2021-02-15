/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <map>
#include "tvgTvgHelper.h"
#include "tvgTvgLoadParser.h"

#define TVG_LOADER_LOG_ENABLED 1

/*
 * Read header of the .tvg binary file
 * Returns true on success, false otherwise.
 * Details:
 * Each file starts with TVG phrase and three chars of version number.
 * Two next bytes are the amount of bytes to skip. Skipping bytes allow
 * backwards compatibility if future version of standard store some extra informations.
 * These informations are called meta (metadata) and skipped lenght meta_lenght.
 * ["TVG"][version eg. "01a"][uint16 amount of bytes to skip eg. 0x0000]
 *
 * Starting phase together with skipped bytes are called header.
 * After header, binary section starts.
 * NOTE: In whole file little endian is used.
 */
static bool tvg_read_header(const char** pointer)
{
   tvg_header * header = (tvg_header *) (*pointer);
   if (memcmp(header->tvg_sign, TVG_HEADER_TVG_SIGN_CODE, 3)) return false;
   if (memcmp(header->version, TVG_HEADER_TVG_VERSION_CODE, 3)) return false;

#ifdef TVG_LOADER_LOG_ENABLED
   char metadata[header->meta_lenght + 1];
   memcpy(metadata, (*pointer) + 8, header->meta_lenght);
   metadata[header->meta_lenght] = '\0';
   printf("TVG_LOADER: Header is valid, metadata[%d]: %s.\n", header->meta_lenght, metadata);
#endif

   *pointer += 8 + header->meta_lenght;
   return true;
}

/*
 * Read canvas section of the .tvg binary file
 * Returns true on success and moves pointer to next position or false if corrupted.
 * Details:
 * Canvas informations starts the binary section.
 * Canvas section starts with with 0xff (TVG_CANVAS_BEGIN_INDICATOR) byte. Next is a single byte flag.
 * If first bit of flag is 1 (flag & 0x1), next eight bytes defines the default size of the canvas.
 * Flag is needed for future purposes. Skipping canvas size allows multiple canvas stored in a single file.
 * For first canvas section, size information is obligatory. If missing, error is returned.
 * Eg. for 32x32 canvas:
 * [0xff][uint8 flag 0x01][uint32 WIDTH 0x20000000][uint32 HEIGHT 0x20000000]
 */
static bool tvg_read_canvas(const char** pointer)
{
   if (**pointer != TVG_CANVAS_BEGIN_INDICATOR) return false;
   *pointer += 1;

   const tvg_canvas * canvas = (tvg_canvas *) *pointer;
   *pointer += sizeof(tvg_canvas);

   // TODO: free buffer on finish
   static uint32_t * buffer = (uint32_t*) malloc(canvas->width * canvas->height);
   if (!buffer)
      {
         return false;
      }

   auto swCanvas = tvg::SwCanvas::gen();
   swCanvas->target(buffer, canvas->width, canvas->width, canvas->height, tvg::SwCanvas::ARGB8888);
   // swCanvas.get()

   return true;
}

/*
 * Read gradient section of the .tvg binary file
 * Returns true on success and moves pointer to next position or false if corrupted.
 * Details:
 * Gradients starts with 0xfe (TVG_GRADIENT_BEGIN_INDICATOR) byte.
 * Next are: uint8 flags and uint32 unique identifier.
 * We do not define identifier to be incremential or start with zero, just unique.
 * If gradient is linear next are 4xfloat x1, y1, x2, y2. If radial, 3xfloat x, y, r.
 * Next are uint16 cnt and cnt*Fill::ColorStop.
 * Flags:
 * xxxxxxx0 - linear gradient
 * xxxxxxx1 - radial gradient
 * xxxxx01x - FillSpread::Pad
 * xxxxx10x - FillSpread::Reflect
 * xxxxx11x - FillSpread::Repeat
 * xxxxx00x - ERROR
 * [0xff][uint8 flags][uint32 identifier][3xfloat x, y, r OR 4xfloat x1, y1, x2, y2][cnt][cnt*Fill::ColorStop]
 */
static bool tvg_read_gradient(const char** pointer)
{
   if (**pointer != TVG_GRADIENT_BEGIN_INDICATOR) return false;
   *pointer += 1;

   const tvg_flags_and_id * flags_and_id = (tvg_flags_and_id *) *pointer;
   *pointer += sizeof(tvg_flags_and_id);

   unique_ptr<tvg::Fill> fillGrad;
   if (flags_and_id->flags & TVG_GRADIENT_FLAG_TYPE_RADIAL)
      {
         // radial gradient
         const tvg_gradient_radial * gradient_radial = (tvg_gradient_radial *) *pointer;
         *pointer += sizeof(tvg_gradient_radial);

         // TODO: check Gradient/Fill cast
         auto fillGradRadial = tvg::RadialGradient::gen();
         fillGradRadial->radial(gradient_radial->x, gradient_radial->y, gradient_radial->radius);
         fillGrad = move(fillGradRadial);
      }
   else
      {
         // linear gradient
         const tvg_gradient_linear * gradient_linear = (tvg_gradient_linear *) *pointer;
         *pointer += sizeof(tvg_gradient_linear);

         auto fillGradLinear = tvg::LinearGradient::gen();
         fillGradLinear->linear(gradient_linear->x1, gradient_linear->y1, gradient_linear->x2, gradient_linear->y2);
         fillGrad = move(fillGradLinear);
      }

   const uint16_t cnt = (uint16_t) **pointer;
   *pointer += sizeof(uint16_t);

   if (cnt > 0)
      {
         fillGrad->colorStops((Fill::ColorStop *) *pointer, cnt);
         *pointer += cnt * sizeof(Fill::ColorStop);
      }

   switch (flags_and_id->flags & TVG_GRADIENT_FLAG_MASK_FILL_SPREAD) {
      case TVG_GRADIENT_FLAG_FILL_SPREAD_PAD:
         fillGrad->spread(FillSpread::Pad);
         break;
      case TVG_GRADIENT_FLAG_FILL_SPREAD_REFLECT:
         fillGrad->spread(FillSpread::Reflect);
         break;
      case TVG_GRADIENT_FLAG_FILL_SPREAD_REPEAT:
         fillGrad->spread(FillSpread::Repeat);
         break;
      default:
         return false;
   }

   return true;
}

/*
 * Read raw image section of the .tvg binary file
 * Returns true on success and moves pointer to next position or false if corrupted.
 * Details:
 * [0xfe][uint8 flags][uint32 identifier][uint32 width][uint32 height][data]
 */
static bool tvg_read_raw_image(const char** pointer)
{
   if (**pointer != TVG_RAW_IMAGE_BEGIN_INDICATOR) return false;
   *pointer += 1;

   const tvg_flags_and_id * flags_and_id = (tvg_flags_and_id *) *pointer;
   *pointer += sizeof(tvg_flags_and_id);
   const tvg_width_height * width_height = (tvg_width_height *) *pointer;
   *pointer += sizeof(tvg_width_height);
   const char * data = *pointer;
   *pointer += width_height->width * width_height->height;

   return true;
}

/*
 * Read scene section of the .tvg binary file
 * Returns true on success and moves pointer to next position or false if corrupted.
 * Details:
 */
static bool tvg_read_scene(const char** pointer)
{
   if (**pointer != TVG_SCENE_BEGIN_INDICATOR) return false;
   *pointer += 1;

   auto scene = Scene::gen();

   return true;
}

/*
 * Read shape section of the .tvg binary file
 * Returns true on success and moves pointer to next position or false if corrupted.
 * Details:
 * Flags:
 * xxxxxxx0 - FillRule.Winding
 * xxxxxxx1 - FillRule.EvenOdd
 * [0xfe][uint8 flags][color][path][stroke][fill]
 *
 * enum class TVG_EXPORT FillRule { Winding = 0, EvenOdd };
 */
static bool tvg_read_shape(const char** pointer)
{
   if (**pointer != TVG_SHAPE_BEGIN_INDICATOR) return false;
   *pointer += 1;

   // flags
   const uint8_t flags = (uint8_t) **pointer;
   *pointer += sizeof(uint8_t);

   // colors
   const uint8_t * colors = (uint8_t*) *pointer;
   *pointer += sizeof(uint8_t) * 4;

   // ShapePath
   const uint32_t cmdCnt = (uint32_t) **pointer;
   *pointer += sizeof(uint32_t);
   const uint32_t ptsCnt = (uint32_t) **pointer;
   *pointer += sizeof(uint32_t);
   const PathCommand * cmds = (PathCommand *) *pointer;
   *pointer += sizeof(PathCommand) * cmdCnt;
   const Point * pts = (Point *) *pointer;
   *pointer += sizeof(Point) * ptsCnt;

   auto shape = Shape::gen();
   shape->appendPath(cmds, cmdCnt, pts, ptsCnt);

   // ShapeStroke
   const tvg_shape_stroke * shape_stroke = (tvg_shape_stroke *) *pointer;
   *pointer += sizeof(tvg_shape_stroke);

   shape->stroke(shape_stroke->width);
   //shape->stroke(shape_stroke->width);

   return true;
}

//TODO: Color-Fill
//Path
//Stroke

bool tvg_file_parse(const char * pointer, uint32_t size)
{
   const char* end = pointer + size;
   if (!tvg_read_header(&pointer) || pointer >= end)
      {
         // LOG: Header is improper
         return false;
      }

   map<uint32_t, int> m;

   while (pointer < end)
      {
         switch (*(pointer))
         {
            case TVG_CANVAS_BEGIN_INDICATOR:
               if (!tvg_read_canvas(&pointer)) return false;
               break;
            case TVG_GRADIENT_BEGIN_INDICATOR:
               if (!tvg_read_gradient(&pointer)) return false;
               break;
            case TVG_RAW_IMAGE_BEGIN_INDICATOR:
               if (!tvg_read_raw_image(&pointer)) return false;
               break;
            case TVG_SCENE_BEGIN_INDICATOR:
               if (!tvg_read_scene(&pointer)) return false;
               break;
            case TVG_SHAPE_BEGIN_INDICATOR:
               if (!tvg_read_shape(&pointer)) return false;
               break;
            default:
               return false;
         }
      }

   return true;
}




