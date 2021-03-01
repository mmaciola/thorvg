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
#ifndef _TVG_SAVER_H_
#define _TVG_SAVER_H_

#include "tvgCommon.h"
//#include "tvgSceneImpl.h"

namespace tvg
{

class Saver
{
public:
    virtual ~Saver() {}

    virtual bool open(const string& path) { /* Not supported */ return false; };
    virtual bool write() = 0;
    virtual bool close() = 0;

//MGS5
    //void nic(Scene* scene) {cout << "UDALO SIE " << (int)scene->pImpl->opacity << endl;};
    //void nic(Scene* scene) {cout << "UDALO SIE " << (int)scene->val << endl;};
    void nic() {
       cout << "UDALO SIE " << (int)sc->val << endl;
       auto scI = sc->Scene::pImpl;
//       scI->wypisz();
       //sc->pImpl->wypisz();
       //cout << "UDALO SIE " << (int)(sc->pImpl->opacity) << endl;
    };
    Scene* sc = nullptr;
};

}

#endif //_TVG_SAVER_H_
