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
#ifndef _TVG_PAINT_H_
#define _TVG_PAINT_H_

#include <float.h>
#include <math.h>
#include "tvgRender.h"
#include "tvgTvgHelper.h"

namespace tvg
{
    struct StrategyMethod
    {
        virtual ~StrategyMethod() {}

        virtual bool dispose(RenderMethod& renderer) = 0;
        virtual void* update(RenderMethod& renderer, const RenderTransform* transform, uint32_t opacity, Array<RenderData>& clips, RenderUpdateFlag pFlag) = 0;   //Return engine data if it has.
        virtual bool render(RenderMethod& renderer) = 0;
        virtual bool bounds(float* x, float* y, float* w, float* h) const = 0;
        virtual bool bounds(RenderMethod& renderer, uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h) const = 0;
        virtual Paint* duplicate() = 0;
        virtual void serialize(char** pointer) = 0;
        virtual LoaderResult tvgLoad(const char* pointer, const char* end) = 0;
    };

    struct Paint::Impl
    {
        StrategyMethod* smethod = nullptr;
        RenderTransform *rTransform = nullptr;
        uint32_t flag = RenderUpdateFlag::None;

        Paint* cmpTarget = nullptr;
        CompositeMethod cmpMethod = CompositeMethod::None;

        uint8_t opacity = 255;

        ~Impl() {
cout << __FILE__ << " " << __func__ << endl;
            if (cmpTarget) delete(cmpTarget);
            if (smethod) delete(smethod);
            if (rTransform) delete(rTransform);
        }

        void method(StrategyMethod* method)
        {
cout << __FILE__ << " " << __func__ << endl;
            smethod = method;
        }

        bool rotate(float degree)
        {
cout << __FILE__ << " " << __func__ << endl;
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
cout << __FILE__ << " " << __func__ << endl;
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
cout << __FILE__ << " " << __func__ << endl;
            if (rTransform) {
                if (fabsf(x - rTransform->x) <= FLT_EPSILON && fabsf(y - rTransform->y) <= FLT_EPSILON) return true;
            } else {
                if (fabsf(x) <= FLT_EPSILON && fabsf(y) <= FLT_EPSILON) return true;
                rTransform = new RenderTransform();
                if (!rTransform) return false;
            }
            rTransform->x = x;
            rTransform->y = y;

            if (!rTransform->overriding) {
                flag |= RenderUpdateFlag::Transform;
            }

            return true;
        }

        bool transform(const Matrix& m)
        {
cout << __FILE__ << " " << __func__ << endl;
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
cout << __FILE__ << " " << __func__ << endl;
            return smethod->bounds(x, y, w, h);
        }

        bool bounds(RenderMethod& renderer, uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h) const
        {
cout << __FILE__ << " " << __func__ << endl;
            return smethod->bounds(renderer, x, y, w, h);
        }

        bool dispose(RenderMethod& renderer)
        {
cout << __FILE__ << " " << __func__ << endl;
            if (cmpTarget) cmpTarget->pImpl->dispose(renderer);
            return smethod->dispose(renderer);
        }

        void* update(RenderMethod& renderer, const RenderTransform* pTransform, uint32_t opacity, Array<RenderData>& clips, uint32_t pFlag)
        {
cout << __FILE__ << " " << __func__ << endl;
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

            if (cmpData) clips.pop();

            return edata;
        }

        bool render(RenderMethod& renderer)
        {
cout << __FILE__ << " " << __func__ << endl;
            Compositor* cmp = nullptr;

            /* Note: only ClipPath is processed in update() step.
               Create a composition image. */
            if (cmpTarget && cmpMethod != CompositeMethod::ClipPath) {
                uint32_t x, y, w, h;
                if (!cmpTarget->pImpl->bounds(renderer, &x, &y, &w, &h)) return false;
                cmp = renderer.target(x, y, w, h);
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
cout << __FILE__ << " " << __func__ << endl;
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
cout << __FILE__ << " " << __func__ << endl;
            if ((!target && method != CompositeMethod::None) || (target && method == CompositeMethod::None)) return false;
            cmpTarget = target;
            cmpMethod = method;
            return true;
        }


        void serializePaint(char** pointer)
        {
cout << __FILE__ << " " << __func__ << " Paint" << endl;
            FlagType flag;
            size_t flagSize = sizeof(FlagType);
            ByteCounter byteCnt = flagSize;
            size_t byteCntSize = sizeof(ByteCounter);

            // transform
            if (rTransform) {
                Matrix m = rTransform->m;
                // transform matrix flag
                flag = TVG_PAINT_FLAG_HAS_TRANSFORM_MATRIX;
                memcpy(*pointer, &flag, flagSize);
                *pointer += flagSize;
                // number of bytes associated with transf matrix
                byteCnt = sizeof(m); //MGS - check
                memcpy(*pointer, &byteCnt, byteCntSize);
                *pointer += byteCntSize;
                // bytes associated with transf matrix
                memcpy(*pointer, &m, byteCnt);
                *pointer += byteCnt;
            }

            // cmpTarget
            if (cmpTarget) {
                char* startCmp = *pointer;

                // cmpTarget flag
                flag = TVG_PAINT_FLAG_HAS_CMP_TARGET;
                memcpy(*pointer, &flag, flagSize);
                *pointer += flagSize;
                // number of bytes associated with cmpTarget - empty
                *pointer += byteCntSize;
                // bytes associated with cmpTrget: method and target

                // method flag
                flag = TVG_PAINT_FLAG_CMP_METHOD;
                memcpy(*pointer, &flag, flagSize);
                *pointer += flagSize;
                // number of bytes associated with method flag
                memcpy(*pointer, &byteCnt, byteCntSize);
                *pointer += byteCntSize;
                // bytes associated with method flag
                switch (cmpMethod) {
                    case CompositeMethod::ClipPath: {
                        flag = TVG_PAINT_FLAG_CMP_METHOD_CLIPPATH;
                        break;
                    }
                    case CompositeMethod::AlphaMask: {
                        flag = TVG_PAINT_FLAG_CMP_METHOD_ALPHAMASK;
                        break;
                    }
                    case CompositeMethod::InvAlphaMask: {
                        flag = TVG_PAINT_FLAG_CMP_METHOD_INV_ALPHAMASK;
                        break;
                    }
                    case CompositeMethod::None: {
                        // obsluzyc blad MGS
                        break;
                    }
                }
                memcpy(*pointer, &flag, flagSize);
                *pointer += flagSize;

                //target serialization
// to jest to zagniezdzenie
cout << "########################################### przed target" << endl;
                cmpTarget->pImpl->serialize(pointer);
cout << "########################################### po target" << endl;
                // number of bytes associated with cmpTarget - filled
                byteCnt = *pointer - startCmp - flagSize - byteCntSize;
                memcpy(*pointer - byteCnt - byteCntSize, &byteCnt, byteCntSize);
            }
        }

        void serialize(char** pointer)
        {
cout << __FILE__ << " " << __func__ << " Paint" << endl;
            smethod->serialize(pointer);
        }

        /*
         * Load paint and derived classes from .tvg binary file
         * Returns LoaderResult:: Success on success and moves pointer to next position,
         * LoaderResult::SizeCorruption if corrupted or LoaderResult::InvalidType if not applicable for paint.
         * Details:
         * TODO
         */
        LoaderResult tvgLoad(const char* pointer, const char* end)
        {
cout << __FILE__ << " " << __func__ << endl;
           LoaderResult result = smethod->tvgLoad(pointer, end);
           if (result != LoaderResult::InvalidType) return result;

           const tvg_block * block = (tvg_block*) pointer;
           switch (block->type)
              {
                 case TVG_PAINT_FLAG_HAS_OPACITY: {
                    if (block->lenght != 1) return LoaderResult::SizeCorruption;
                    opacity = block->data;
                    break;
                 }
                 case TVG_PAINT_FLAG_HAS_TRANSFORM_MATRIX: {
                    if (block->lenght != sizeof(Matrix)) return LoaderResult::SizeCorruption;
                    const Matrix * matrix = (Matrix *) &block->data;
                    transform(*matrix); // TODO: check if transformation works
                    break;
                 }
                 default: {
                    return LoaderResult::InvalidType;
                 }
              }

           return LoaderResult::Success;
        }

        /*
         * Store paint to .tvg binary file
         * Details: see above function tvgLoad
         */
        bool tvgStore()
        {
cout << __FILE__ << " " << __func__ << endl;
           return true;
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
cout << __FILE__ << " " << __func__ << endl;
            return inst->bounds(x, y, w, h);
        }

        bool bounds(RenderMethod& renderer, uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h) const override
        {
cout << __FILE__ << " " << __func__ << endl;
            return inst->bounds(renderer, x, y, w, h);
        }

        bool dispose(RenderMethod& renderer) override
        {
cout << __FILE__ << " " << __func__ << endl;
            return inst->dispose(renderer);
        }

        void* update(RenderMethod& renderer, const RenderTransform* transform, uint32_t opacity, Array<RenderData>& clips, RenderUpdateFlag flag) override
        {
cout << __FILE__ << " " << __func__ << endl;
            return inst->update(renderer, transform, opacity, clips, flag);
        }

        bool render(RenderMethod& renderer) override
        {
cout << __FILE__ << " " << __func__ << endl;
            return inst->render(renderer);
        }

        Paint* duplicate() override
        {
cout << __FILE__ << " " << __func__ << endl;
            return inst->duplicate();
        }

        LoaderResult tvgLoad(const char* pointer, const char* end) override
        {
cout << __FILE__ << " " << __func__ << endl;
             return inst->tvgLoad(pointer, end);
        }

        void serialize(char** pointer) override
        {
cout << __FILE__ << " " << __func__ << " smeth" << endl;
             inst->serialize(pointer);
        }
    };
}

#endif //_TVG_PAINT_H_
