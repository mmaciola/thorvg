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

TvgLoader::~TvgLoader()
{
    close();
}

void TvgLoader::clearBuffer()
{
    size = 0;
    free(buffer);
    buffer = nullptr;
    pointer = nullptr;
}

bool TvgLoader::open(const string &path)
{
    ifstream f;
    f.open(path, ifstream::in | ifstream::binary | ifstream::ate);

    if (!f.is_open())
    {
#ifdef THORVG_LOG_ENABLED
        printf("TVG_LOADER: Failed to open file\n");
#endif
        return false;
    }

    size = f.tellg();
    f.seekg(0, ifstream::beg);

    buffer = (char*) malloc(size);
    if (!buffer)
    {
        size = 0;
        f.close();
#ifdef THORVG_LOG_ENABLED
        printf("TVG_LOADER: Failed to alloc buffer\n");
#endif
        return false;
    }

    if (!f.read(buffer, size))
    {
        clearBuffer();
        f.close();
        return false;
    }

    f.close();

    pointer = buffer;

    return tvgValidateTvgHeader(pointer, size);
}

bool TvgLoader::open(const char *data, uint32_t size)
{
    //TODO: verify memory leak if open() is called multiple times.

    pointer = data;
    size = size;

    return tvgValidateTvgHeader(pointer, size);
}

bool TvgLoader::read(bool async)
{
    printf("TvgLoader::read 0 \n");
    if (!pointer || size == 0) return false;
    printf("TvgLoader::read \n");

    this->async = async;
    TaskScheduler::request(this, async);

    return true;
}

bool TvgLoader::close()
{
    if (async) this->done();
    clearBuffer();
    return true;
}

void TvgLoader::run(unsigned tid)
{
    printf("TvgLoader::run \n");
    if (root) root.reset();
    root = tvgLoadTvgData(pointer, size);

    if (!root)
    {
#ifdef THORVG_LOG_ENABLED
        printf("TVG_LOADER: File parsing error\n");
#endif
        clearBuffer();
    }
}

unique_ptr<Scene> TvgLoader::scene()
{
    if (async) this->done();
    if (root) return move(root);
    return nullptr;
}
