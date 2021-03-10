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
#ifndef _TVG_TVG_SAVER_H_
#define _TVG_TVG_SAVER_H_

#include "tvgTaskScheduler.h"
#include "tvgSaverMgr.h"
#include <fstream>

class TvgSaver : public Saver, public Task
{
public:
    ofstream outFile;

    char* buffer;
    uint32_t size = 0;   
    uint32_t reserved = 0; //MGS
    char* pointer = nullptr;

    Scene* root;

    TvgSaver(Scene* scene);
    ~TvgSaver();

    using Saver::open;  //MGS
    bool open(const string& path) override;
    bool write() override;
    bool close() override;
    void run(unsigned tid);

    void resizeBuffer(); //MGS
};

#endif //_TVG_TVG_SAVER_H_
