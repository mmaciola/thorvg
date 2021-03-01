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
#include "tvgSceneImpl.h"

/************************************************************************/
/* External Class Implementation                                        */
/************************************************************************/

Scene::Scene() : pImpl(new Impl())
{
cout << __FILE__ << " " << __func__ << endl;
    Paint::pImpl->method(new PaintMethod<Scene::Impl>(pImpl));
}


Scene::~Scene()
{
cout << __FILE__ << " " << __func__ << endl;
    delete(pImpl);
}


unique_ptr<Scene> Scene::gen() noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    return unique_ptr<Scene>(new Scene);
}


Result Scene::push(unique_ptr<Paint> paint) noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    auto p = paint.release();
    if (!p) return Result::MemoryCorruption;
    pImpl->paints.push(p);

    return Result::Success;
}


Result Scene::reserve(uint32_t size) noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    pImpl->paints.reserve(size);

    return Result::Success;
}


Result Scene::clear() noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    pImpl->paints.clear();

    return Result::Success;
}


Result Scene::save(const std::string& path) noexcept
{
cout << __FILE__ << " " << __func__ << endl;
    if (path.empty()) return Result::InvalidArguments;

    //return pImpl->save(path);
    return pImpl->save(path, this); //MGS2
}

//MGS2 - temp solution, can't fine a better one for now
void Scene::serialize()
{
cout << __FILE__ << " " << __func__ << endl;
        //MGS5 auto tvgSaver = static_cast<TvgSaver*>(saver.get());
        //MGS5 serialize(&tvgSaver->pointer);
    pImpl->serialize();

//MGS5    pImpl->serializationStart();
}

// tvgTvgLoader / tvgTvgStorer
Result Scene::load(const string& path)
{
cout << __FILE__ << " " << __func__ << endl;
   return pImpl->load(path);
}


Result Scene::load(const char* data, uint32_t size)
{
cout << __FILE__ << " " << __func__ << endl;
   return pImpl->load(data, size);
}


