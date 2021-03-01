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
#include "tvgPaint.h"

/************************************************************************/
/* Internal Class Implementation                                        */
/************************************************************************/

Paint :: Paint() : pImpl(new Impl())
{
cout << __FILE__ << " " << __func__ << endl;
}


Paint :: ~Paint()
{
cout << __FILE__ << " " << __func__ << endl;
    delete(pImpl);
}


/************************************************************************/
/* External Class Implementation                                        */
/************************************************************************/

Result Paint::rotate(float degree) noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    if (pImpl->rotate(degree)) return Result::Success;
    return Result::FailedAllocation;
}


Result Paint::scale(float factor) noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    if (pImpl->scale(factor)) return Result::Success;
    return Result::FailedAllocation;
}


Result Paint::translate(float x, float y) noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    if (pImpl->translate(x, y)) return Result::Success;
    return Result::FailedAllocation;
}


Result Paint::transform(const Matrix& m) noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    if (pImpl->transform(m)) return Result::Success;
    return Result::FailedAllocation;
}


Result Paint::bounds(float* x, float* y, float* w, float* h) const noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    if (pImpl->bounds(x, y, w, h)) return Result::Success;
    return Result::InsufficientCondition;
}


Paint* Paint::duplicate() const noexcept
{
cout << __FILE__ << " " << __func__ << endl;
   printf("Paint::duplicate() cpp \n");
    return pImpl->duplicate();
}


Result Paint::composite(std::unique_ptr<Paint> target, CompositeMethod method) const noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    if (pImpl->composite(target.release(), method)) return Result::Success;
    return Result::InsufficientCondition;
}


Result Paint::opacity(uint8_t o) noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    if (pImpl->opacity == o) return Result::Success;

    pImpl->opacity = o;
    pImpl->flag |= RenderUpdateFlag::Color;

    return Result::Success;
}


uint8_t Paint::opacity() const noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    return pImpl->opacity;
}

// tvgTvgLoader / tvgTvgStorer
LoaderResult Paint::tvgLoad(const char* pointer, const char* end) noexcept
{
cout << __FILE__ << " " << __func__ << endl;
   return pImpl->tvgLoad(pointer, end);
}

bool Paint::tvgStore() noexcept
{
cout << __FILE__ << " " << __func__ << endl;
   return pImpl->tvgStore();
}
