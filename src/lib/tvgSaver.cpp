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
//#include <string.h>
//#include <fstream>
#include "tvgSaverImpl.h"
#include <iostream>

/************************************************************************/
/* Internal Class Implementation                                        */
/************************************************************************/

/************************************************************************/
/* External Class Implementation                                        */
/************************************************************************/

//Saver::Saver(Paint* paint)
Saver::Saver() : pImpl(new Impl(this))
{
}


Saver::~Saver()
{
    delete(pImpl);
}


Result Saver::save(std::unique_ptr<Paint> paint, const std::string& path) noexcept
{
   // if empty path save to buffer
    if (!paint) return Result::InvalidArguments;

    auto saver = unique_ptr<Saver>(new Saver());
    if (!saver) return Result::FailedAllocation;

// MGS  zwolnic
    if (saver->pImpl->save(paint.release(), path)) return Result::Success;

    return Result::Unknown;
}


Result Saver::save(std::unique_ptr<Paint> paint, char** buffer_out, uint32_t* size_out) noexcept
{
   // if empty path save to buffer
    if (!paint) return Result::InvalidArguments;

    auto saver = unique_ptr<Saver>(new Saver());
    if (!saver) return Result::FailedAllocation;

// MGS  zwolnic
    if (saver->pImpl->save(paint.release(), buffer_out, size_out)) return Result::Success;

    return Result::Unknown;
}


/*
bool Saver::open(const string& path)
{
    filePath = path;

    reserved = 8;
    buffer = static_cast<char*>(malloc(reserved));
    if (!buffer) {
        reserved = 0;
        return false;
    }
    bufferPosition = buffer;

    return saveHeader();
}


bool Saver::write()
{
    if (!buffer) return false;

    TaskScheduler::request(this);

    return true;
}


void Saver::run(unsigned tid)
{
    root->pImpl->serializeStart();  //MGS temp

    ofstream outFile;
    outFile.open(filePath, ios::out | ios::trunc | ios::binary);
    if (!outFile.is_open()) return;
    outFile.write(buffer,size);
    outFile.close();

    if (buffer) free(buffer);
    buffer = nullptr;
    bufferPosition = nullptr;
    size = 0;
    reserved = 0;
};


bool Saver::close()
{
    this->done();

    if (buffer) free(buffer);
    buffer = nullptr;
    bufferPosition = nullptr;
    size = 0;
    reserved = 0;

    return true;
}
*/

void Saver::saveMemberIndicator(TvgIndicator ind)
{
    if (pImpl->size + TVG_INDICATOR_SIZE > pImpl->reserved) resizeBuffer(pImpl->size + TVG_INDICATOR_SIZE);

    memcpy(pImpl->bufferPosition, &ind, TVG_INDICATOR_SIZE);
    pImpl->bufferPosition += TVG_INDICATOR_SIZE;
    pImpl->size += TVG_INDICATOR_SIZE;
}


void Saver::saveMemberDataSize(ByteCounter byteCnt)
{
    if (pImpl->size + BYTE_COUNTER_SIZE > pImpl->reserved) resizeBuffer(pImpl->size + BYTE_COUNTER_SIZE);

    memcpy(pImpl->bufferPosition, &byteCnt, BYTE_COUNTER_SIZE);
    pImpl->bufferPosition += BYTE_COUNTER_SIZE;
    pImpl->size += BYTE_COUNTER_SIZE;
}


void Saver::saveMemberDataSizeAt(ByteCounter byteCnt)
{
    memcpy(pImpl->bufferPosition - byteCnt - BYTE_COUNTER_SIZE, &byteCnt, BYTE_COUNTER_SIZE);
}


void Saver::skipMemberDataSize()
{
    if (pImpl->size + BYTE_COUNTER_SIZE > pImpl->reserved) resizeBuffer(pImpl->size + BYTE_COUNTER_SIZE);
    pImpl->bufferPosition += BYTE_COUNTER_SIZE;
    pImpl->size += BYTE_COUNTER_SIZE;
}


ByteCounter Saver::saveMemberData(const void* data, ByteCounter byteCnt)
{
    if (pImpl->size + byteCnt > pImpl->reserved) resizeBuffer(pImpl->size + byteCnt);

    memcpy(pImpl->bufferPosition, data, byteCnt);
    pImpl->bufferPosition += byteCnt;
    pImpl->size += byteCnt;

    return byteCnt;
}


ByteCounter Saver::saveMember(TvgIndicator ind, ByteCounter byteCnt, const void* data)
{
    ByteCounter blockByteCnt = TVG_INDICATOR_SIZE + BYTE_COUNTER_SIZE + byteCnt;

    if (pImpl->size + blockByteCnt > pImpl->reserved) resizeBuffer(pImpl->size + blockByteCnt);

    memcpy(pImpl->bufferPosition, &ind, TVG_INDICATOR_SIZE);
    pImpl->bufferPosition += TVG_INDICATOR_SIZE;
    memcpy(pImpl->bufferPosition, &byteCnt, BYTE_COUNTER_SIZE);
    pImpl->bufferPosition += BYTE_COUNTER_SIZE;
    memcpy(pImpl->bufferPosition, data, byteCnt);
    pImpl->bufferPosition += byteCnt;

    pImpl->size += blockByteCnt;

    return blockByteCnt;
}


void Saver::resizeBuffer(uint32_t newSize)
{
    // MGS - find more optimal alg ?
    pImpl->reserved += 100;
    if (newSize > pImpl->reserved) pImpl->reserved += newSize - pImpl->reserved + 100;

    auto bufferOld = pImpl->buffer;

    pImpl->buffer = static_cast<char*>(realloc(pImpl->buffer, pImpl->reserved));

    if (pImpl->buffer != bufferOld)
        pImpl->bufferPosition = pImpl->buffer + (pImpl->bufferPosition - bufferOld);
}


void Saver::rewindBuffer(ByteCounter bytesNum)
{
    if (pImpl->bufferPosition - bytesNum < pImpl->buffer) return;

    pImpl->bufferPosition -= bytesNum;
    pImpl->size -= bytesNum;
}

/*
bool Saver::saveHeader()
{
    const char *tvg = TVG_HEADER_TVG_SIGN_CODE;
    const char *version = TVG_HEADER_TVG_VERSION_CODE;
    // MGS - metadata not raelly used for now - type hardcoded (to be changed)
    uint16_t metadataByteCnt = 0;
    ByteCounter headerByteCnt = strlen(tvg) + strlen(version) + sizeof(metadataByteCnt);
    if (size + headerByteCnt > reserved) resizeBuffer(headerByteCnt);

    memcpy(bufferPosition, tvg, strlen(tvg));
    bufferPosition += strlen(tvg);
    memcpy(bufferPosition, version, strlen(version));
    bufferPosition += strlen(version);
    memcpy(bufferPosition, &metadataByteCnt, sizeof(metadataByteCnt));
    bufferPosition += sizeof(metadataByteCnt);

    size += headerByteCnt;
    return true;
}
*/
