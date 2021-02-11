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

#include <fstream>
#include <string.h>
#include "tvgLoaderMgr.h"
#include "tvgTvgHelper.h"
#include "tvgTvgLoader.h"

#define TVG_LOADER_LOG_ENABLED 1

TvgLoader::TvgLoader()
{

}
TvgLoader::~TvgLoader()
{
   close();
}

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
bool TvgLoader::tvg_validate_header()
{
   this->pointer = this->buffer;
   tvg_header * header = (tvg_header *) this->pointer;
   if (memcmp(header->tvg_sign, TVG_HEADER_TVG_SIGN_CODE, 3)) return false;
   if (memcmp(header->version, TVG_HEADER_TVG_VERSION_CODE, 3)) return false;

#ifdef TVG_LOADER_LOG_ENABLED
   char metadata[header->meta_lenght + 1];
   memcpy(metadata, this->pointer + 8, header->meta_lenght);
   metadata[header->meta_lenght] = '\0';
   printf("TVG_LOADER: Header is valid, metadata[%d]: %s.\n", header->meta_lenght, metadata);
#endif

   this->pointer += 8 + header->meta_lenght;
   return true;
}

/*
 * Read gradient of the .tvg binary file
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
bool TvgLoader::tvg_read_gradient()
{
   if (*(this->pointer) != TVG_GRADIENT_BEGIN_INDICATOR) return false;
   this->pointer += 1;

   tvg_flags_and_id * flags_and_id = (tvg_flags_and_id *) this->pointer;
   this->pointer += sizeof(tvg_flags_and_id);

   unique_ptr<tvg::Fill> fillGrad;
   if (flags_and_id->flags & TVG_GRADIENT_FLAG_TYPE_RADIAL)
      {
         // radial gradient
         tvg_gradient_radial * gradient_radial = (tvg_gradient_radial *) this->pointer;
         this->pointer += sizeof(tvg_gradient_radial);

         fillGrad = tvg::RadialGradient::gen();
         ((unique_ptr<tvg::RadialGradient>) fillGrad)->radial(gradient_radial->x, gradient_radial->y, gradient_radial->radius);
      }
   else
      {
         // linear gradient
         tvg_gradient_linear * gradient_linear = (tvg_gradient_linear *) this->pointer;
         this->pointer += sizeof(tvg_gradient_linear);

         fillGrad = tvg::LinearGradient::gen();
         ((unique_ptr<tvg::LinearGradient>) fillGrad)->linear(gradient_linear->x1, gradient_linear->y1, gradient_linear->x2, gradient_linear->y2);
      }

   uint16_t cnt = (uint16_t) *this->pointer;
   this->pointer += sizeof(uint16_t);

   if (cnt > 0)
      {
         fillGrad->colorStops((Fill::ColorStop *) this->pointer, cnt);
         this->pointer += cnt * sizeof(Fill::ColorStop);
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

bool TvgLoader::tvg_parse()
{

   while (pointer < end)
      {
         switch (*(this->pointer))
         {
            case TVG_GRADIENT_BEGIN_INDICATOR:
               if (!tvg_read_gradient()) return false;
               break;
            default:
               return false;
         }
      }
   return true;
}


void TvgLoader::tvg_clean_buffer()
{
   this->size = 0;
   if (this->buffer)
      {
         free(this->buffer);
         this->buffer = NULL;
      }
   this->pointer = NULL;
}

bool TvgLoader::open(const string& path)
{
   ifstream f;
   f.open(path, ifstream::in | ifstream::binary | ifstream::ate);

   if (!f.is_open())
       {
           // LOG: Failed to open file
           return false;
       }

   this->size = f.tellg();
   f.seekg(0, ifstream::beg);

   this->buffer = (char*) malloc(this->size);
   if (this->buffer == NULL)
      {
         // LOG: Failed to alloc buffer
         this->size = 0;
         f.close();
         return false;
      }

   if (!f.read(this->buffer, size))
      {
         tvg_clean_buffer();
         f.close();
         return false;
      }

   f.close();

   bool success = tvg_validate_header();
   if (!success)
      {
         // LOG: Header is improper
         tvg_clean_buffer();
      }

   return success;
}

bool TvgLoader::open(const char* data, uint32_t size)
{
    this->pointer = data;
    this->size = size;

    bool success = tvg_validate_header();
    if (!success)
       {
          // LOG: Header is improper
          tvg_clean_buffer();
       }

    return success;
}

bool TvgLoader::read()
{
   if (!this->pointer || this->size == 0) return false;
   TaskScheduler::request(this);
   return true;
}

bool TvgLoader::close()
{
   tvg_clean_buffer();
   return true;
}

void TvgLoader::run(unsigned tid)
{
   printf("TvgLoader::run \n");


}



