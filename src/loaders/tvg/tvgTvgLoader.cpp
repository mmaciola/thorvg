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

TvgLoader::TvgLoader()
{

}
TvgLoader::~TvgLoader()
{

}
void TvgLoader::run(unsigned tid)
{

}

/*
 * Read header of the .tvg binary file
 * See HEADER section in TVG Standard specification for details.
 * Returns true on success, false otherwise.
 */
static bool tvg_read_header(ifstream &f)
{
   tvg_header header;
   if (f.read((char*) &header, 8)) {
         //LOG: Failed to read header section
         return false;
   }

   if (memcmp(header.tvg_sign, TVG_HEADER_TVG_SIGN_CODE, 3)) return false;
   if (memcmp(header.version, TVG_HEADER_TVG_VERSION_CODE, 3)) return false;

   header.meta = (char*) malloc(sizeof(char) * header.meta_lenght);
   if (header.meta == NULL) return false;
   if (f.read(header.meta, header.meta_lenght))
     {
         free(header.meta);
         header.meta = NULL;
         return false;
     }
   return true;
}

bool TvgLoader::open(const string& path)
{
   ifstream f;
   f.open(path, ifstream::in | ifstream::binary);

   if (!f.is_open())
    {
        // LOG: Failed to open file
        return false;
    }

   bool success = tvg_read_header(f);

   f.close();
   return success;
}

bool TvgLoader::read()
{
   //if (!content || size == 0) return false;
   TaskScheduler::request(this);
   return true;
}

bool TvgLoader::close()
{
    return true;
}
