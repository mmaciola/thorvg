/*
 * Copyright (c) 2020-2021 Samsung Electronics Co., Ltd. All rights reserved.

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
#include "tvgSceneImpl.h"

/************************************************************************/
/* External Class Implementation                                        */
/************************************************************************/

Scene::Scene() : pImpl(new Impl(this))
{
    Paint::pImpl->type = PaintType::Scene;
    Paint::pImpl->method(new PaintMethod<Scene::Impl>(pImpl));
}


Scene::~Scene()
{
    delete(pImpl);
}


unique_ptr<Scene> Scene::gen() noexcept
{
    return unique_ptr<Scene>(new Scene);
}


Result Scene::push(unique_ptr<Paint> paint) noexcept
{
    auto p = paint.release();
    if (!p) return Result::MemoryCorruption;
    pImpl->paints.push(p);

    return Result::Success;
}


Result Scene::reserve(uint32_t size) noexcept
{
    pImpl->paints.reserve(size);

    return Result::Success;
}


Result Scene::clear() noexcept
{
    pImpl->paints.clear();

    return Result::Success;
}


Result Scene::load(const string& path)
{
   if (path.empty()) return Result::InvalidArguments;
   return pImpl->load(path);
}

Result Scene::load(const char* data, uint32_t size)
{
   return pImpl->load(data, size);
}


Result Scene::save(const std::string& path) noexcept
{
    if (path.empty()) return Result::InvalidArguments;
    return pImpl->save(path, this);
}

//MGS - temp solution ?
void Scene::serialize()
{
    auto tvgSaver = static_cast<TvgSaver*>(pImpl->saver.get());
    auto tmp = Paint::pImpl->serialize(tvgSaver);

}
