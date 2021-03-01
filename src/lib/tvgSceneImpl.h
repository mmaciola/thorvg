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
#ifndef _TVG_SCENE_IMPL_H_
#define _TVG_SCENE_IMPL_H_

#include "tvgPaint.h"
#include "tvgSaverMgr.h"
//MGS5
#include "tvgTvgSaver.h" // MGS - usunac

#include "tvgTvgLoader.h"

/************************************************************************/
/* Internal Class Implementation                                        */
/************************************************************************/

struct Scene::Impl
{
    Array<Paint*> paints;
    uint8_t opacity = 17;            //for composition //MGS5

    unique_ptr<Saver> saver = nullptr;

//MGS5
    void wypisz()
    {
        cout << "HAHAHA " << (int)opacity << endl;
    }

    bool dispose(RenderMethod& renderer)
    {
cout << __FILE__ << " " << __func__ << endl;
        for (auto paint = paints.data; paint < (paints.data + paints.count); ++paint) {
            (*paint)->pImpl->dispose(renderer);
            delete(*paint);
        }
        paints.clear();

        return true;
    }

    void* update(RenderMethod &renderer, const RenderTransform* transform, uint32_t opacity, Array<RenderData>& clips, RenderUpdateFlag flag)
    {
cout << __FILE__ << " " << __func__ << endl;
        /* Overriding opacity value. If this scene is half-translucent,
           It must do intermeidate composition with that opacity value. */
        this->opacity = static_cast<uint8_t>(opacity);
        if (opacity > 0) opacity = 255;

int i = 0;
        for (auto paint = paints.data; paint < (paints.data + paints.count); ++paint) {
cout << "update sceny " << ++i << endl;
            (*paint)->pImpl->update(renderer, transform, opacity, clips, static_cast<uint32_t>(flag));
        }
cout << "koniec update sceny" << endl;

        if (saver && !saver->write()) return nullptr; //MGS3

        /* FXIME: it requires to return list of children engine data
           This is necessary for scene composition */
        return nullptr;
    }

    bool render(RenderMethod& renderer)
    {
cout << __FILE__ << " " << __func__ << endl;
        Compositor* cmp = nullptr;

        //Half translucent. This condition requires intermediate composition.
        if ((opacity < 255 && opacity > 0) && (paints.count > 1)) {
            uint32_t x, y, w, h;
            if (!bounds(renderer, &x, &y, &w, &h)) return false;
            cmp = renderer.target(x, y, w, h);
            renderer.beginComposite(cmp, CompositeMethod::None, opacity);
        }

        for (auto paint = paints.data; paint < (paints.data + paints.count); ++paint) {
            if (!(*paint)->pImpl->render(renderer)) return false;
        }

        if (cmp) renderer.endComposite(cmp);

        return true;
    }

    bool bounds(RenderMethod& renderer, uint32_t* px, uint32_t* py, uint32_t* pw, uint32_t* ph)
    {
cout << __FILE__ << " " << __func__ << endl;
        if (paints.count == 0) return false;

        uint32_t x1 = UINT32_MAX;
        uint32_t y1 = UINT32_MAX;
        uint32_t x2 = 0;
        uint32_t y2 = 0;

        for (auto paint = paints.data; paint < (paints.data + paints.count); ++paint) {
            uint32_t x = UINT32_MAX;
            uint32_t y = UINT32_MAX;
            uint32_t w = 0;
            uint32_t h = 0;

            if (!(*paint)->pImpl->bounds(renderer, &x, &y, &w, &h)) continue;

            //Merge regions
            if (x < x1) x1 = x;
            if (x2 < x + w) x2 = (x + w);
            if (y < y1) y1 = y;
            if (y2 < y + h) y2 = (y + h);
        }

        if (px) *px = x1;
        if (py) *py = y1;
        if (pw) *pw = (x2 - x1);
        if (ph) *ph = (y2 - y1);

        return true;
    }

    bool bounds(float* px, float* py, float* pw, float* ph)
    {
cout << __FILE__ << " " << __func__ << endl;
        if (paints.count == 0) return false;

        auto x1 = FLT_MAX;
        auto y1 = FLT_MAX;
        auto x2 = 0.0f;
        auto y2 = 0.0f;

        for (auto paint = paints.data; paint < (paints.data + paints.count); ++paint) {
            auto x = FLT_MAX;
            auto y = FLT_MAX;
            auto w = 0.0f;
            auto h = 0.0f;

            if (!(*paint)->pImpl->bounds(&x, &y, &w, &h)) continue;

            //Merge regions
            if (x < x1) x1 = x;
            if (x2 < x + w) x2 = (x + w);
            if (y < y1) y1 = y;
            if (y2 < y + h) y2 = (y + h);
        }

        if (px) *px = x1;
        if (py) *py = y1;
        if (pw) *pw = (x2 - x1);
        if (ph) *ph = (y2 - y1);

        return true;
    }

    Paint* duplicate()
    {
cout << __FILE__ << " " << __func__ << endl;
       printf("Paint* duplicate() scene h \n");
        auto ret = Scene::gen();
        if (!ret) return nullptr;
        auto dup = ret.get()->pImpl;

        dup->paints.reserve(paints.count);

        for (auto paint = paints.data; paint < (paints.data + paints.count); ++paint) {
            dup->paints.push((*paint)->duplicate());
        }

        return ret.release();
    }

    void serialize(char** pointer)
    {
cout << __FILE__ << " " << __func__ << endl;
        for (auto paint = paints.data; paint < (paints.data + paints.count); ++paint) {
            (*paint)->pImpl->serialize(pointer);
        }
    }

//MGS2 - temp solution
    void serializationStart()
    {
cout << __FILE__ << " " << __func__ << endl;
//        if (!saver->write()) return;// Result::Unknown; //MGS3 - jesli nei da segf to nei wiem czemu :), to daje requesta w runie
//MGS5 - wykomentowalam na potrzeby testow
//MGS5        auto tvgSaver = static_cast<TvgSaver*>(saver.get());
//MGS5       serialize(&tvgSaver->pointer);
    }

//    Result save(const std::string& path)
    Result save(const std::string& path, Scene *scene) //MGS2
    {
cout << __FILE__ << " " << __func__ << endl;
        if (saver) saver->close();
        //saver = SaverMgr::saver(path);
        saver = SaverMgr::saver(path, scene);  //MGS2
        if (!saver) return Result::NonSupport;
        //if (!saver->write()) return Result::Unknown; //MGS3 - tu za wczesnie jest to staskowac...

        // MGS - temp solution
//        auto tvgSaver = static_cast<TvgSaver*>(saver.get());  //MGS2
//        serialize(&tvgSaver->pointer);

        return Result::Success;
    }

    Result load(const string& path)
    {
cout << __FILE__ << " " << __func__ << endl;
       // TODO: [mmaciola] zadecydowac czy uzywac schematu z LoaderMgr
       TvgLoader * loader = new TvgLoader();
       if (!loader->open(path)) return Result::Unknown;
       if (!loader->read()) return Result::Unknown;
       return Result::Success;
    }

    Result load(const char* data, uint32_t size)
    {
cout << __FILE__ << " " << __func__ << endl;
       TvgLoader * loader = new TvgLoader();
       if (!loader->open(data, size)) return Result::Unknown;
       if (!loader->read()) return Result::Unknown;
       return Result::Success;
    }

    /*
     * Load scene from .tvg binary file
     * Returns LoaderResult:: Success on success and moves pointer to next position,
     * LoaderResult::SizeCorruption if corrupted or LoaderResult::InvalidType if not applicable for paint.
     * Details:
     * TODO
     */
    LoaderResult tvgLoad(const char* pointer, const char* end)
    {
cout << __FILE__ << " " << __func__ << endl;
       const tvg_block * block = (tvg_block*) pointer;
       switch (block->type)
         {
          case TVG_SCENE_FLAG_RESERVEDCNT: {
             if (block->lenght != 1) return LoaderResult::SizeCorruption;
             uint32_t reservedCnt = (uint32_t) block->data;
             paints.reserve(reservedCnt);
             break;
          }
          default:
             return LoaderResult::InvalidType;
         }

       return LoaderResult::Success;
    }


    /*
     * Store scene from .tvg binary file
     * Details: see above function tvgLoad
     */
    bool tvgStore()
    {
cout << __FILE__ << " " << __func__ << endl;
       return true;
    }
};

#endif //_TVG_SCENE_IMPL_H_
