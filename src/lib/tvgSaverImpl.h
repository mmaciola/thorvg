/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All rights reserved.

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
#ifndef _TVG_SAVER_IMPL_H_
#define _TVG_SAVER_IMPL_H_

#include "tvgCommon.h"
#include "tvgPaint.h"
#include "tvgTvgHelper.h"
#include <fstream>

struct Saver::Impl
{
    Saver* saver;
    char* buffer = nullptr;
    uint32_t size = 0;
    uint32_t reserved = 0;
    char* bufferPosition = nullptr;
//    Scene* root; // moze nie jest potrzebne


    Impl(Saver* s) : saver(s)
    {
    }

    ~Impl()
    {
    }

bool saveHeader()
{
    const char *tvg = TVG_HEADER_TVG_SIGN_CODE;
    const char *version = TVG_HEADER_TVG_VERSION_CODE;
    // MGS - metadata not really used for now - hardcoded (to be changed)
    uint16_t metadataByteCnt = 0;
    ByteCounter headerByteCnt = strlen(tvg) + strlen(version) + sizeof(metadataByteCnt);
    if (size + headerByteCnt > reserved) saver->resizeBuffer(headerByteCnt);

    memcpy(bufferPosition, tvg, strlen(tvg));
    bufferPosition += strlen(tvg);
    memcpy(bufferPosition, version, strlen(version));
    bufferPosition += strlen(version);
    memcpy(bufferPosition, &metadataByteCnt, sizeof(metadataByteCnt));
    bufferPosition += sizeof(metadataByteCnt);

    size += headerByteCnt;
    return true;
}



    bool save(Paint* paint, const std::string& path)
    {
      cout << __FILE__ << " " << __func__ << endl;

      reserved = 8;
      buffer = static_cast<char*>(malloc(reserved));
      if (!buffer) {
      reserved = 0;
      return false;
      }
      bufferPosition = buffer;

      saveHeader();

      auto tmp = paint->pImpl->serialize(saver);

      ofstream outFile;
      outFile.open(path, ios::out | ios::trunc | ios::binary);
      cout << "otworze  plik " << path << endl;
      if (!outFile.is_open()) return false;
      cout << "otworzylam plik " << path << endl;
      outFile.write(buffer,size);
      outFile.close();

      if (buffer) free(buffer);
      buffer = nullptr;
      bufferPosition = nullptr;
      size = 0;
      reserved = 0;

      return true;
    }

    bool save(Paint* paint, char** buffer_out, uint32_t* size_out)
    {
       reserved = 8;
       buffer = static_cast<char*>(malloc(reserved));
       if (!buffer) {
          reserved = 0;
          return false;
       }
       bufferPosition = buffer;

       saveHeader();

       auto tmp = paint->pImpl->serialize(saver);

       *buffer_out = buffer;
       *size_out = size;

       return true;
    }


};

#endif //_TVG_SAVER_IMPL_H_
