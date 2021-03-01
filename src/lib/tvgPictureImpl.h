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
#include "tvgTvgHelper.h"

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
cout << __FILE__ << " " << __func__ << endl;
    }

    bool dispose(RenderMethod& renderer)
    {
cout << __FILE__ << " " << __func__ << endl;
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
cout << __FILE__ << " " << __func__ << endl;
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
cout << __FILE__ << " " << __func__ << endl;
        if (loader) {
            if (!paint) {
                auto scene = loader->scene();
                if (scene) {
                    paint = scene.release();
                    loader->close();
                    if (w != loader->w && h != loader->h) resize();
                    if (paint) return RenderUpdateFlag::None; //MGS4 - check
                }
            }
            if (!pixels) {
                pixels = const_cast<uint32_t*>(loader->pixels());
                if (pixels) return RenderUpdateFlag::Image;
            }
        }
cout << "KONIEC " << __FILE__ << " " << __func__ << endl;
        return RenderUpdateFlag::None;
    }

    void* update(RenderMethod &renderer, const RenderTransform* transform, uint32_t opacity, Array<RenderData>& clips, RenderUpdateFlag pFlag)
    {
cout << __FILE__ << " " << __func__ << endl;
        auto flag = reload();

        if (pixels) rdata = renderer.prepare(*picture, rdata, transform, opacity, clips, static_cast<RenderUpdateFlag>(pFlag | flag));
        else if (paint) {
            if (resizing) resize();
            rdata = paint->pImpl->update(renderer, transform, opacity, clips, static_cast<RenderUpdateFlag>(pFlag | flag));
        }
cout << "KONIEC " << __FILE__ << " " << __func__ << endl;
        return rdata;
    }

    bool render(RenderMethod &renderer)
    {
cout << __FILE__ << " " << __func__ << endl;
        if (pixels) return renderer.renderImage(rdata);
        else if (paint) return paint->pImpl->render(renderer);
cout << "KONIEC " << __FILE__ << " " << __func__ << endl;
        return false;
    }

    bool viewbox(float* x, float* y, float* w, float* h)
    {
cout << __FILE__ << " " << __func__ << endl;
        if (!loader) return false;
        if (x) *x = loader->vx;
        if (y) *y = loader->vy;
        if (w) *w = loader->vw;
        if (h) *h = loader->vh;
        return true;
    }

    bool size(uint32_t w, uint32_t h)
    {
cout << __FILE__ << " " << __func__ << endl;
        this->w = w;
        this->h = h;
        resizing = true;
        return true;
    }

    bool bounds(float* x, float* y, float* w, float* h)
    {
cout << __FILE__ << " " << __func__ << endl;
        if (!paint) return false;
        return paint->pImpl->bounds(x, y, w, h);
    }

    bool bounds(RenderMethod& renderer, uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h)
    {
cout << __FILE__ << " " << __func__ << endl;
        if (rdata) return renderer.region(rdata, x, y, w, h);
        if (paint) return paint->pImpl->bounds(renderer, x, y, w, h);
        return false;
    }

    Result load(const string& path)
    {
cout << __FILE__ << " " << __func__ << endl;
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
cout << __FILE__ << " " << __func__ << endl;
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
cout << __FILE__ << " " << __func__ << endl;
        if (loader) loader->close();
        loader = LoaderMgr::loader(data, w, h, copy);
        if (!loader) return Result::NonSupport;
        return Result::Success;
    }

    Paint* duplicate()
    {
cout << __FILE__ << " " << __func__ << endl;
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

        //NGS - reconsider
        if (loader) {
            // remove?
            if (!pixels) {
                cout << "pixele nie byly zaladowane - dziwne" << endl;
                pixels = const_cast<uint32_t*>(loader->pixels());
            }
            // should I use local var instead of w/h ?  //MGS3
            w = loader->vw;
            h = loader->vh;
        }
        if (fabsf(w) < FLT_EPSILON || fabsf(h) < FLT_EPSILON) return; //false 
cout << "rozmiar pict juz znany " << w << " " << h << endl;

        FlagType flag;
        ByteCounter byteCnt = 0;
        size_t byteCntSize = sizeof(ByteCounter);
        auto sizeofW = sizeof(w);
        auto sizeofH = sizeof(h);

        // picture indicator
        flag = TVG_RAW_IMAGE_BEGIN_INDICATOR;
        memcpy(*pointer, &flag, sizeof(FlagType));
        *pointer += sizeof(FlagType);
        // number of bytes associated with picture
        byteCnt = sizeofW + sizeofH + w * h * sizeof(pixels[0]);
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;
        // bytes associated with picture: [w][h][pixels]
        memcpy(*pointer, &w, sizeofW);
        *pointer += sizeofW;
        memcpy(*pointer, &h, sizeofH);
        *pointer += sizeofH;
        memcpy(*pointer, pixels, w * h * sizeof(pixels[0]));
        *pointer += (size_t)(w * h) * sizeof(pixels[0]);
    }

    /*
     * Load picture from .tvg binary file
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
          case 0: {
             // TODO
             break;
          }
          default:
             return LoaderResult::InvalidType;
         }

       return LoaderResult::Success;
    }
};

#endif //_TVG_PICTURE_IMPL_H_
