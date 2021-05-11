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

/*
void saveMemberIndicator(TvgIndicator ind)
{
    if (size + TVG_INDICATOR_SIZE > reserved) resizeBuffer(size + TVG_INDICATOR_SIZE);

    memcpy(bufferPosition, &ind, TVG_INDICATOR_SIZE);
    bufferPosition += TVG_INDICATOR_SIZE;
    size += TVG_INDICATOR_SIZE;
}


void saveMemberDataSize(ByteCounter byteCnt)
{
    if (size + BYTE_COUNTER_SIZE > reserved) resizeBuffer(size + BYTE_COUNTER_SIZE);

    memcpy(bufferPosition, &byteCnt, BYTE_COUNTER_SIZE);
    bufferPosition += BYTE_COUNTER_SIZE;
    size += BYTE_COUNTER_SIZE;
}


void saveMemberDataSizeAt(ByteCounter byteCnt)
{
    memcpy(bufferPosition - byteCnt - BYTE_COUNTER_SIZE, &byteCnt, BYTE_COUNTER_SIZE);
}


void skipMemberDataSize()
{
    if (size + BYTE_COUNTER_SIZE > reserved) resizeBuffer(size + BYTE_COUNTER_SIZE);
    bufferPosition += BYTE_COUNTER_SIZE;
    size += BYTE_COUNTER_SIZE;
}


ByteCounter saveMemberData(const void* data, ByteCounter byteCnt)
{
    if (size + byteCnt > reserved) resizeBuffer(size + byteCnt);

    memcpy(bufferPosition, data, byteCnt);
    bufferPosition += byteCnt;
    size += byteCnt;

    return byteCnt;
}


ByteCounter saveMember(TvgIndicator ind, ByteCounter byteCnt, const void* data)
{
    ByteCounter blockByteCnt = TVG_INDICATOR_SIZE + BYTE_COUNTER_SIZE + byteCnt;

    if (size + blockByteCnt > reserved) resizeBuffer(size + blockByteCnt);

    memcpy(bufferPosition, &ind, TVG_INDICATOR_SIZE);
    bufferPosition += TVG_INDICATOR_SIZE;
    memcpy(bufferPosition, &byteCnt, BYTE_COUNTER_SIZE);
    bufferPosition += BYTE_COUNTER_SIZE;
    memcpy(bufferPosition, data, byteCnt);
    bufferPosition += byteCnt;

    size += blockByteCnt;

    return blockByteCnt;
}


void resizeBuffer(uint32_t newSize)
{
    // MGS - find more optimal alg ?
    reserved += 100;
    if (newSize > reserved) reserved += newSize - reserved + 100;

    auto bufferOld = buffer;

    buffer = static_cast<char*>(realloc(buffer, reserved));

    if (buffer != bufferOld)
        bufferPosition = buffer + (bufferPosition - bufferOld);
}

void rewindBuffer(ByteCounter bytesNum)
{
    if (bufferPosition - bytesNum < buffer) return;

    bufferPosition -= bytesNum;
    size -= bytesNum;
}
*/

};

#endif //_TVG_SAVER_IMPL_H_
