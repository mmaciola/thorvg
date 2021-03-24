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
#ifndef _TVG_PAINT_H_
#define _TVG_PAINT_H_

#include <float.h>
#include <math.h>
#include "tvgRender.h"
#include "tvgTvgLoadParser.h"

namespace tvg
{
    enum class PaintType { Shape = 0, Scene, Picture };

    struct StrategyMethod
    {
        virtual ~StrategyMethod() {}

        virtual bool dispose(RenderMethod& renderer) = 0;
        virtual void* update(RenderMethod& renderer, const RenderTransform* transform, uint32_t opacity, Array<RenderData>& clips, RenderUpdateFlag pFlag) = 0;   //Return engine data if it has.
        virtual bool render(RenderMethod& renderer) = 0;
        virtual bool bounds(float* x, float* y, float* w, float* h) const = 0;
        virtual RenderRegion bounds(RenderMethod& renderer) const = 0;
        virtual Paint* duplicate() = 0;

        virtual LoaderResult tvgLoad(tvg_block block) = 0;
    };

    struct Paint::Impl
    {
        StrategyMethod* smethod = nullptr;
        RenderTransform *rTransform = nullptr;
        uint32_t flag = RenderUpdateFlag::None;

        Paint* cmpTarget = nullptr;
        CompositeMethod cmpMethod = CompositeMethod::None;

        uint8_t opacity = 255;

        PaintType type;

        ~Impl() {
            if (cmpTarget) delete(cmpTarget);
            if (smethod) delete(smethod);
            if (rTransform) delete(rTransform);
        }

        void method(StrategyMethod* method)
        {
            smethod = method;
        }

        bool rotate(float degree)
        {
            if (rTransform) {
                if (fabsf(degree - rTransform->degree) <= FLT_EPSILON) return true;
            } else {
                if (fabsf(degree) <= FLT_EPSILON) return true;
                rTransform = new RenderTransform();
                if (!rTransform) return false;
            }
            rTransform->degree = degree;
            if (!rTransform->overriding) flag |= RenderUpdateFlag::Transform;

            return true;
        }

        bool scale(float factor)
        {
            if (rTransform) {
                if (fabsf(factor - rTransform->scale) <= FLT_EPSILON) return true;
            } else {
                if (fabsf(factor) <= FLT_EPSILON) return true;
                rTransform = new RenderTransform();
                if (!rTransform) return false;
            }
            rTransform->scale = factor;
            if (!rTransform->overriding) flag |= RenderUpdateFlag::Transform;

            return true;
        }

        bool translate(float x, float y)
        {
            if (rTransform) {
                if (fabsf(x - rTransform->x) <= FLT_EPSILON && fabsf(y - rTransform->y) <= FLT_EPSILON) return true;
            } else {
                if (fabsf(x) <= FLT_EPSILON && fabsf(y) <= FLT_EPSILON) return true;
                rTransform = new RenderTransform();
                if (!rTransform) return false;
            }
            rTransform->x = x;
            rTransform->y = y;
            if (!rTransform->overriding) flag |= RenderUpdateFlag::Transform;

            return true;
        }

        bool transform(const Matrix& m)
        {
            if (!rTransform) {
                rTransform = new RenderTransform();
                if (!rTransform) return false;
            }
            rTransform->override(m);
            flag |= RenderUpdateFlag::Transform;

            return true;
        }

        bool bounds(float* x, float* y, float* w, float* h) const
        {
            return smethod->bounds(x, y, w, h);
        }

        RenderRegion bounds(RenderMethod& renderer) const
        {
            return smethod->bounds(renderer);
        }

        bool dispose(RenderMethod& renderer)
        {
            if (cmpTarget) cmpTarget->pImpl->dispose(renderer);
            return smethod->dispose(renderer);
        }

        void* update(RenderMethod& renderer, const RenderTransform* pTransform, uint32_t opacity, Array<RenderData>& clips, uint32_t pFlag)
        {
            if (flag & RenderUpdateFlag::Transform) {
                if (!rTransform) return nullptr;
                if (!rTransform->update()) {
                    delete(rTransform);
                    rTransform = nullptr;
                }
            }

            void *cmpData = nullptr;

            if (cmpTarget) {
                cmpData = cmpTarget->pImpl->update(renderer, pTransform, 255, clips, pFlag);
                if (cmpMethod == CompositeMethod::ClipPath) clips.push(cmpData);
            }

            void *edata = nullptr;
            auto newFlag = static_cast<RenderUpdateFlag>(pFlag | flag);
            flag = RenderUpdateFlag::None;
            opacity = (opacity * this->opacity) / 255;

            if (rTransform && pTransform) {
                RenderTransform outTransform(pTransform, rTransform);
                edata = smethod->update(renderer, &outTransform, opacity, clips, newFlag);
            } else {
                auto outTransform = pTransform ? pTransform : rTransform;
                edata = smethod->update(renderer, outTransform, opacity, clips, newFlag);
            }

            if (cmpData && cmpMethod == CompositeMethod::ClipPath) clips.pop();

            return edata;
        }

        bool render(RenderMethod& renderer)
        {
            Compositor* cmp = nullptr;

            /* Note: only ClipPath is processed in update() step.
               Create a composition image. */
            if (cmpTarget && cmpMethod != CompositeMethod::ClipPath) {
                auto region = cmpTarget->pImpl->bounds(renderer);
                if (region.w == 0 || region.h == 0) return false;
                cmp = renderer.target(region);
                renderer.beginComposite(cmp, CompositeMethod::None, 255);
                cmpTarget->pImpl->render(renderer);
            }

            if (cmp) renderer.beginComposite(cmp, cmpMethod, cmpTarget->pImpl->opacity);

            auto ret = smethod->render(renderer);

            if (cmp) renderer.endComposite(cmp);

            return ret;
        }

        Paint* duplicate()
        {
            auto ret = smethod->duplicate();
            if (!ret) return nullptr;

            //duplicate Transform
            if (rTransform) {
                ret->pImpl->rTransform = new RenderTransform();
                if (ret->pImpl->rTransform) {
                    *ret->pImpl->rTransform = *rTransform;
                    ret->pImpl->flag |= RenderUpdateFlag::Transform;
                }
            }

            ret->pImpl->opacity = opacity;

            if (cmpTarget) ret->pImpl->cmpTarget = cmpTarget->duplicate();

            ret->pImpl->cmpMethod = cmpMethod;

            return ret;
        }

        bool composite(Paint* target, CompositeMethod method)
        {
            if ((!target && method != CompositeMethod::None) || (target && method == CompositeMethod::None)) return false;
            if (cmpTarget) delete(cmpTarget);
            cmpTarget = target;
            cmpMethod = method;
            return true;
        }

        LoaderResult tvgLoadCmpTarget(const char* pointer, const char* end)
        {
           tvg_block block = read_tvg_block(pointer);
           if (block.block_end > end) return LoaderResult::SizeCorruption;

           if (block.type != TVG_PAINT_FLAG_CMP_METHOD) return LoaderResult::LogicalCorruption;
           if (block.lenght != 1) return LoaderResult::SizeCorruption;
           switch (*block.data)
           {
              case TVG_PAINT_FLAG_CMP_METHOD_CLIPPATH:
                 cmpMethod = CompositeMethod::ClipPath;
                 break;
              case TVG_PAINT_FLAG_CMP_METHOD_ALPHAMASK:
                 cmpMethod = CompositeMethod::AlphaMask;
                 break;
              case TVG_PAINT_FLAG_CMP_METHOD_INV_ALPHAMASK:
                 cmpMethod = CompositeMethod::InvAlphaMask;
                 break;
              default:
                 return LoaderResult::LogicalCorruption;
           }

           pointer = block.block_end;
           tvg_block block_paint = read_tvg_block(pointer);
           if (block_paint.block_end > end) return LoaderResult::SizeCorruption;

           LoaderResult result = tvg_read_paint(block_paint, &cmpTarget);
           if (result != LoaderResult::Success)
           {
              if (cmpTarget) delete(cmpTarget);
              cmpMethod = CompositeMethod::None;
           }

           if (result > LoaderResult::Success) return result;
           return LoaderResult::Success;
        }

        LoaderResult tvgLoad(const char* pointer, const char* end)
        {
           while (pointer < end)
           {
              tvg_block block = read_tvg_block(pointer);
              if (block.block_end > end) return LoaderResult::SizeCorruption;

              LoaderResult result = smethod->tvgLoad(block);
              if (result == LoaderResult::InvalidType)
              {
                 switch (block.type) {
                    case TVG_PAINT_FLAG_HAS_OPACITY: {
                       if (block.lenght != 1) return LoaderResult::SizeCorruption;
                       opacity = *block.data;
                       break;
                    }
                    case TVG_PAINT_FLAG_HAS_TRANSFORM_MATRIX: {
                       if (block.lenght != sizeof(Matrix)) return LoaderResult::SizeCorruption;
                       const Matrix * matrix = (Matrix *) block.data;
                       transform(*matrix); // TODO: check if transformation works
                       break;
                    }
                    case TVG_PAINT_FLAG_HAS_CMP_TARGET: { // cmp target
                       if (block.lenght < 5) return LoaderResult::SizeCorruption;
                       LoaderResult result = tvgLoadCmpTarget(block.data, block.block_end);
                       if (result != LoaderResult::Success) return result;
                       break;
                    }
                 }
              }

              if (result > LoaderResult::Success) return result;
              pointer = block.block_end;
           }
           return LoaderResult::Success;
        }
    };


    template<class T>
    struct PaintMethod : StrategyMethod
    {
        T* inst = nullptr;

        PaintMethod(T* _inst) : inst(_inst) {}
        ~PaintMethod() {}

        bool bounds(float* x, float* y, float* w, float* h) const override
        {
            return inst->bounds(x, y, w, h);
        }

        RenderRegion bounds(RenderMethod& renderer) const override
        {
            return inst->bounds(renderer);
        }

        bool dispose(RenderMethod& renderer) override
        {
            return inst->dispose(renderer);
        }

        void* update(RenderMethod& renderer, const RenderTransform* transform, uint32_t opacity, Array<RenderData>& clips, RenderUpdateFlag flag) override
        {
            return inst->update(renderer, transform, opacity, clips, flag);
        }

        bool render(RenderMethod& renderer) override
        {
            return inst->render(renderer);
        }

        Paint* duplicate() override
        {
            return inst->duplicate();
        }

        LoaderResult tvgLoad(tvg_block block) override
        {
             return inst->tvgLoad(block);
        }
    };
}

#endif //_TVG_PAINT_H_
