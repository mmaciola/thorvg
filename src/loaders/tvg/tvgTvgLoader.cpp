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
#include "tvgTvgLoader.h"
#include "tvgTvgLoadParser.h"

TvgLoader::TvgLoader()
{
   // TvgLoader
}
TvgLoader::~TvgLoader()
{
   close();
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

   this->pointer = this->buffer;

   return true;
}

bool TvgLoader::open(const char* data, uint32_t size)
{
   this->pointer = data;
   this->size = size;
   return true;
}

bool TvgLoader::read()
{
   if (!this->pointer || this->size == 0) return false;
   TaskScheduler::request(this);
   return true;
}

bool TvgLoader::close()
{
   this->done();
   tvg_clean_buffer();
   return true;
}

bool TvgLoader::paints(Scene * scene)
{
   this->root = scene;
   if (!this->root) return false;
   return true;
}

void TvgLoader::run(unsigned tid)
{
   if (!tvg_file_parse(this->pointer, this->size, this->root))
      {
         // TODO: what should I do if parsing error
         printf("[mmaciola] tvg_file_parse ERROR\n");
         tvg_clean_buffer();
      }
   // todo done?
}



