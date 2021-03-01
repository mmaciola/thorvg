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
#include "tvgCanvasImpl.h"

/************************************************************************/
/* External Class Implementation                                        */
/************************************************************************/

Canvas::Canvas(RenderMethod *pRenderer):pImpl(new Impl(pRenderer))
{
cout << __FILE__ << " " << __func__ << endl;
}


Canvas::~Canvas()
{
cout << __FILE__ << " " << __func__ << endl;
    delete(pImpl);
}


Result Canvas::reserve(uint32_t n) noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    pImpl->paints.reserve(n);
    return Result::Success;
}


Result Canvas::push(unique_ptr<Paint> paint) noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    return pImpl->push(move(paint));
}


Result Canvas::clear(bool free) noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    return pImpl->clear(free);
}


Result Canvas::draw() noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    return pImpl->draw();
}


Result Canvas::update(Paint* paint) noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    return pImpl->update(paint, false);
}


Result Canvas::sync() noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    if (pImpl->renderer->sync()) return Result::Success;

    return Result::InsufficientCondition;
}


Result Canvas::load(const string& path)
{
cout << __FILE__ << " " << __func__ << endl;
   return pImpl->load(path);
}


Result Canvas::load(const char* data, uint32_t size)
{
cout << __FILE__ << " " << __func__ << endl;
   return pImpl->load(data, size);
}
