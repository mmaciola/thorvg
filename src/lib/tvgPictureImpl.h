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
#ifndef _TVG_PICTURE_IMPL_H_
#define _TVG_PICTURE_IMPL_H_

#include <string>
#include "tvgPaint.h"
#include "tvgLoaderMgr.h"

/************************************************************************/
/* Internal Class Implementation                                        */
/************************************************************************/

struct Picture::Impl
{
    unique_ptr<Loader> loader = nullptr;
    Paint* paint = nullptr;
    uint32_t *pixels = nullptr;
    Picture *picture = nullptr;
    void *rdata = nullptr;              //engine data
    float w = 0, h = 0;
    bool resizing = false;

    Impl(Picture* p) : picture(p)
    {
    }

    bool dispose(RenderMethod& renderer)
    {
        bool ret = true;
        if (paint) {
            ret = paint->pImpl->dispose(renderer);
            delete(paint);
            paint = nullptr;
        }
        else if (pixels) {
            ret =  renderer.dispose(rdata);
            rdata = nullptr;
        }
        return ret;
    }

    void resize()
    {
        auto sx = w / loader->vw;
        auto sy = h / loader->vh;

        if (loader->preserveAspect) {
            //Scale
            auto scale = sx < sy ? sx : sy;
            paint->scale(scale);
            //Align
            auto vx = loader->vx * scale;
            auto vy = loader->vy * scale;
            auto vw = loader->vw * scale;
            auto vh = loader->vh * scale;
            if (vw > vh) vy -= (h - vh) * 0.5f;
            else vx -= (w - vw) * 0.5f;
            paint->translate(-vx, -vy);
        } else {
            //Align
            auto vx = loader->vx * sx;
            auto vy = loader->vy * sy;
            auto vw = loader->vw * sx;
            auto vh = loader->vh * sy;
            if (vw > vh) vy -= (h - vh) * 0.5f;
            else vx -= (w - vw) * 0.5f;

            Matrix m = {sx, 0, -vx, 0, sy, -vy, 0, 0, 1};
            paint->transform(m);
        }
        resizing = false;
    }

    uint32_t reload()
    {
        if (loader) {
            if (!paint) {
                auto scene = loader->scene();
                if (scene) {
                    paint = scene.release();
                    loader->close();
                    if (w != loader->w && h != loader->h) resize();
                    if (paint) return RenderUpdateFlag::None;
                }
            }
            if (!pixels) {
                pixels = const_cast<uint32_t*>(loader->pixels());
                if (pixels) return RenderUpdateFlag::Image;
            }
        }
        return RenderUpdateFlag::None;
    }

    void* update(RenderMethod &renderer, const RenderTransform* transform, uint32_t opacity, Array<RenderData>& clips, RenderUpdateFlag pFlag)
    {
        auto flag = reload();

        if (pixels) rdata = renderer.prepare(*picture, rdata, transform, opacity, clips, static_cast<RenderUpdateFlag>(pFlag | flag));
        else if (paint) {
            if (resizing) resize();
            rdata = paint->pImpl->update(renderer, transform, opacity, clips, static_cast<RenderUpdateFlag>(pFlag | flag));
        }
        return rdata;
    }

    bool render(RenderMethod &renderer)
    {
        if (pixels) return renderer.renderImage(rdata);
        else if (paint) return paint->pImpl->render(renderer);
        return false;
    }

    bool viewbox(float* x, float* y, float* w, float* h)
    {
        if (!loader) return false;
        if (x) *x = loader->vx;
        if (y) *y = loader->vy;
        if (w) *w = loader->vw;
        if (h) *h = loader->vh;
        return true;
    }

    bool size(uint32_t w, uint32_t h)
    {
        this->w = w;
        this->h = h;
        resizing = true;
        return true;
    }

    bool bounds(float* x, float* y, float* w, float* h)
    {
        if (!paint) return false;
        return paint->pImpl->bounds(x, y, w, h);
    }

    bool bounds(RenderMethod& renderer, uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h)
    {
        if (rdata) return renderer.region(rdata, x, y, w, h);
        if (paint) return paint->pImpl->bounds(renderer, x, y, w, h);
        return false;
    }

    Result load(const string& path)
    {
        if (loader) loader->close();
        loader = LoaderMgr::loader(path);
        if (!loader) return Result::NonSupport;
        if (!loader->read()) return Result::Unknown;
        w = loader->w;
        h = loader->h;
        return Result::Success;
    }

    Result load(const char* data, uint32_t size)
    {
        if (loader) loader->close();
        loader = LoaderMgr::loader(data, size);
        if (!loader) return Result::NonSupport;
        if (!loader->read()) return Result::Unknown;
        w = loader->w;
        h = loader->h;
        return Result::Success;
    }

    Result load(uint32_t* data, uint32_t w, uint32_t h, bool copy)
    {
        if (loader) loader->close();
        loader = LoaderMgr::loader(data, w, h, copy);
        if (!loader) return Result::NonSupport;
        return Result::Success;
    }

    Paint* duplicate()
    {
        reload();

        if (!paint) return nullptr;
        auto ret = Picture::gen();
        if (!ret) return nullptr;
        auto dup = ret.get()->pImpl;
        dup->paint = paint->duplicate();

        return ret.release();
    }

    void serialize(char** pointer)
    {
      cout << __FILE__ << " " << __func__ << endl;
        if (!*pointer) return;// false;

        reload();
        float vw = loader->vw, vh = loader->vh; //MGS - temp, waiting for merge
        if (!paint && !pixels && vw > 0 && vh > 0) return; //false

        char* start = *pointer;
        FlagType flag;
        size_t flagSize = sizeof(FlagType);
        ByteCounter byteCnt = flagSize;
        size_t byteCntSize = sizeof(ByteCounter);

        // picture indicator
        flag = TVG_PICTURE_BEGIN_INDICATOR;
        memcpy(*pointer, &flag, sizeof(FlagType));
        *pointer += sizeof(FlagType);
        // number of bytes associated with picture - empty for now
        *pointer += byteCntSize;

        if (paint) {
            paint->Paint::pImpl->serialize(pointer);
        }
        else if (pixels) {
            uint32_t w_ = (uint32_t)vw;
            uint32_t h_ = (uint32_t)vh;
            uint32_t size = w_ * h_ * sizeof(pixels[0]);
          
            // raw image indicator
            flag = TVG_RAW_IMAGE_BEGIN_INDICATOR;
            memcpy(*pointer, &flag, sizeof(FlagType));
            *pointer += sizeof(FlagType);
          
            // number of bytes associated with picture
            byteCnt = 2 * sizeof(uint32_t) + size;
            memcpy(*pointer, &byteCnt, byteCntSize);
            *pointer += byteCntSize;
            // bytes associated with picture: [w][h][pixels]
            memcpy(*pointer, &w_, sizeof(uint32_t));
            *pointer += sizeof(uint32_t);
            memcpy(*pointer, &h_, sizeof(uint32_t));
            *pointer += sizeof(uint32_t);
            memcpy(*pointer, pixels, size);
            *pointer += size;
          
            /*auto sizeofW = sizeof(vw);
            auto sizeofH = sizeof(vh);
            if (fabsf(vw) < FLT_EPSILON || fabsf(vh) < FLT_EPSILON) return; //false 
            // raw image indicator
            flag = TVG_RAW_IMAGE_BEGIN_INDICATOR;
            memcpy(*pointer, &flag, sizeof(FlagType));
            *pointer += sizeof(FlagType);
            // number of bytes associated with raw image
            byteCnt = sizeofW + sizeofH + vw * vh * sizeof(pixels[0]);
            memcpy(*pointer, &byteCnt, byteCntSize);
            *pointer += byteCntSize;
            // bytes associated with raw image: [w][h][pixels]
            memcpy(*pointer, &vw, sizeofW);
            *pointer += sizeofW;
            memcpy(*pointer, &vh, sizeofH);
            *pointer += sizeofH;
            memcpy(*pointer, pixels, vw * vh * sizeof(pixels[0]));
            *pointer += (size_t)(vw * vh) * sizeof(pixels[0]);*/
        }

        picture->Paint::pImpl->serializePaint(pointer);

        // number of bytes associated with picture - filled
        byteCnt = *pointer - start - flagSize - byteCntSize;
        memcpy(*pointer - byteCnt - byteCntSize, &byteCnt, byteCntSize);
        printf("Shape byteCnt : %d \n", byteCnt);
    }

    /*
     * Load picture from .tvg binary file
     * Returns LoaderResult:: Success on success and moves pointer to next position,
     * LoaderResult::SizeCorruption if corrupted or LoaderResult::InvalidType if not applicable for paint.
     * Details:
     * TVG_RAW_IMAGE_BEGIN_INDICATOR : [w][h][pixels]
     */
    LoaderResult tvgLoad(tvg_block block)
    {
       switch (block.type)
         {
            case TVG_RAW_IMAGE_BEGIN_INDICATOR: {
               if (block.lenght < 8) return LoaderResult::SizeCorruption;

               uint32_t w, h;
               _read_tvg_ui32(&w, block.data);
               _read_tvg_ui32(&h, block.data + 4);
               uint32_t size = w * h * sizeof(pixels[0]);
               if (block.lenght != 8 + size) return LoaderResult::SizeCorruption;

               uint32_t* pixels = (uint32_t*) malloc(size);
               if (!pixels) return LoaderResult::MemoryCorruption;
               memcpy(pixels, block.data + 8, size);

               load(pixels, w, h, false);
               return LoaderResult::Success;
            }
         }

       Paint * paint_local;
       LoaderResult result = tvg_read_paint(block, &paint_local);
       if (result == LoaderResult::Success) paint = paint_local;

       if (result != LoaderResult::InvalidType) return result;
       return LoaderResult::InvalidType;
    }
};

#endif //_TVG_PICTURE_IMPL_H_
