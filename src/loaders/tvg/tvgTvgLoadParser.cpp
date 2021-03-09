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
   // Sign phase, always "TVG" declared in TVG_HEADER_TVG_SIGN_CODE
   if (memcmp(*pointer, TVG_HEADER_TVG_SIGN_CODE, 3)) return false;
   *pointer += 3; // move after sing code

   // Standard version number, declared in TVG_HEADER_TVG_VERSION_CODE
   if (memcmp(*pointer, TVG_HEADER_TVG_VERSION_CODE, 3)) return false;
   *pointer += 3; // move after version code

   // Matadata phase
   uint16_t meta_lenght = _read_tvg_16(*pointer); // Matadata phase lenght
   *pointer += 2; // move after lenght

#ifdef TVG_LOADER_LOG_ENABLED
   char metadata[meta_lenght + 1];
   memcpy(metadata, *pointer, meta_lenght);
   metadata[meta_lenght] = '\0';
   printf("TVG_LOADER: Header is valid, metadata[%d]: %s.\n", meta_lenght, metadata);
#endif

   *pointer += meta_lenght;
   return true;
}

/*
 * Read shape section of the .tvg binary file
 * Returns true on success and moves pointer to next position or false if corrupted.
 * Details: see tvgLoad() in tvgShapeImpl.h
 */
static LoaderResult tvg_read_shape(const char* pointer, const char* end, Scene * scene)
{
   // create shape
   auto s = Shape::gen();
   printf("tvg_read_shape\n");

   while (pointer < end)
      {
         tvg_block block = read_tvg_block(pointer);
         printf("block.block_end %d end %d\n", block.block_end - pointer, end - pointer);
         //printf("tvg_read_shape block type %d\n", block.type);
         if (block.block_end > end) return LoaderResult::SizeCorruption;

         LoaderResult result = s->tvgLoad(block);
         printf("tvg_read_shape block result %s\n", (result == LoaderResult::Success) ? "Success" : ((result == LoaderResult::InvalidType) ? "InvalidType" : ((result == LoaderResult::SizeCorruption) ? "SizeCorruption" : "MemoryCorruption")));
         if (result > LoaderResult::Success) return result;

         pointer = block.block_end;
      }

   scene->push(move(s));
   return LoaderResult::Success;
}

/*
 * Read scene section of the .tvg binary file
 * Returns true on success and moves pointer to next position or false if corrupted.
 * Details:
 */
static LoaderResult tvg_read_scene(const char* pointer, const char* end, Scene * scene)
{
   // create scene
   auto s = Scene::gen();

   while (pointer < end)
      {
         tvg_block block = read_tvg_block(pointer);
         if (block.block_end > end) return LoaderResult::SizeCorruption;

         LoaderResult result = s->tvgLoad(block);

         if (result == LoaderResult::InvalidType)
            {
               switch (block.type) {
                  case TVG_SHAPE_BEGIN_INDICATOR:
                  result = tvg_read_shape(block.data, block.block_end, s.get());
                  break;
                  case TVG_SCENE_BEGIN_INDICATOR:
                  result = tvg_read_scene(block.data, block.block_end, s.get());
                  break;
               }
            }

         printf("tvg_read_scene result %d \n", result);
         if (result > LoaderResult::Success) return result;

         pointer = block.block_end;
      }

   scene->push(move(s));
   return LoaderResult::Success;
}

// Load .tvg file to pointed scene
bool tvg_file_parse(const char * pointer, uint32_t size, Scene * scene)
{
   const char* end = pointer + size;
   if (!tvg_read_header(&pointer) || pointer >= end)
      {
         // LOG: Header is improper
         return false;
      }

   while (pointer < end)
      {
         tvg_block block = read_tvg_block(pointer);
         if (block.block_end > end) return false;

         switch (block.type)
            {
            case TVG_SCENE_BEGIN_INDICATOR:
               if (tvg_read_scene(block.data, block.block_end, scene) > LoaderResult::Success) return false;
               break;
            case TVG_SHAPE_BEGIN_INDICATOR:
               if (tvg_read_shape(block.data, block.block_end, scene) > LoaderResult::Success) return false;
               break;
            case 0: // TODO mmaciola temporery fix for buffer const lenght in saver ***********
               return true;
            default:
               // LOG: Invalid type
               return false;
            }

         pointer = block.block_end;
      }

   // LOG: File parsed correctly
   printf("TVG_LOADER: File parsed correctly.\n");
   return true;
}

// Read tvg_block from pointer location.
tvg_block read_tvg_block(const char * pointer) {
   tvg_block block;
   block.type = *pointer;
   block.lenght = _read_tvg_32(pointer + sizeof(FlagType));
   //printf("TVG_LOADER: lenght %d: %02x %02x %02x %02x.\n", block.lenght, pointer[1], pointer[2], pointer[3], pointer[4]);
   block.data = pointer + sizeof(FlagType) + sizeof(ByteCounter);
   block.block_end = block.data + block.lenght;
   //printf("TVG_LOADER: type %d, data[%d]: %02x %02x %02x.\n", block.type, block.lenght, block.data[0], block.data[1], block.data[2]);
   return block;
}



