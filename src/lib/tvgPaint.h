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

#include "tvgRender.h"
//#include "tvgSaverImpl.h"
#include "tvgTvgHelper.h" //MGS

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
        virtual bool interpolate(Paint* from, Paint* to, double pos_map) = 0;
        virtual ByteCounter serialize(Saver* saver) = 0;
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

        bool composite(Paint* target, CompositeMethod method)
        {
            if ((!target && method != CompositeMethod::None) || (target && method == CompositeMethod::None)) return false;
            if (cmpTarget) delete(cmpTarget);
            cmpTarget = target;
            cmpMethod = method;
            return true;
        }

        bool interpolate(Paint* from, Paint* to, double pos_map)
        {
           bool result = smethod->interpolate(from, to, pos_map);
           if (!result) return false;

           Impl* impl_from = from->pImpl;
           Impl* impl_to = to->pImpl;

           // TODO Interpolate general attributes (opacity, matrix) and composition

           return true;
        }

        bool rotate(float degree);
        bool scale(float factor);
        bool translate(float x, float y);
        void* update(RenderMethod& renderer, const RenderTransform* pTransform, uint32_t opacity, Array<RenderData>& clips, uint32_t pFlag);
        bool render(RenderMethod& renderer);
        Paint* duplicate();

        ByteCounter serializePaint(Saver* saver)
        {
            if (!saver) return 0;
            ByteCounter paintDataByteCnt = 0;

            if (opacity < 255) {
                paintDataByteCnt += saver->saveMember(TVG_PAINT_OPACITY_INDICATOR, sizeof(opacity), &opacity);
            }

            if (rTransform) {
                rTransform->update();
                paintDataByteCnt += saver->saveMember(TVG_PAINT_TRANSFORM_MATRIX_INDICATOR, sizeof(rTransform->m), &rTransform->m);
            }

            if (cmpTarget) {
                ByteCounter cmpDataByteCnt = 0;

                saver->saveMemberIndicator(TVG_PAINT_CMP_TARGET_INDICATOR);
                saver->skipMemberDataSize();

                TvgFlag cmpMethodTvgFlag;
                switch (cmpMethod) {
                    case CompositeMethod::ClipPath: {
                        cmpMethodTvgFlag = TVG_PAINT_CMP_METHOD_CLIPPATH_FLAG;
                        break;
                    }
                    case CompositeMethod::AlphaMask: {
                        cmpMethodTvgFlag = TVG_PAINT_CMP_METHOD_ALPHAMASK_FLAG;
                        break;
                    }
                    case CompositeMethod::InvAlphaMask: {
                        cmpMethodTvgFlag = TVG_PAINT_CMP_METHOD_INV_ALPHAMASK_FLAG;
                        break;
                    }
                    case CompositeMethod::None: {
                        break;
                    }
                }
                cmpDataByteCnt += saver->saveMember(TVG_PAINT_CMP_METHOD_INDICATOR, TVG_FLAG_SIZE, &cmpMethodTvgFlag);

                cmpDataByteCnt += cmpTarget->pImpl->serialize(saver);

                saver->saveMemberDataSizeAt(cmpDataByteCnt);
                paintDataByteCnt += TVG_INDICATOR_SIZE + BYTE_COUNTER_SIZE + cmpDataByteCnt;
            }

            return paintDataByteCnt;
        }

        ByteCounter serialize(Saver* saver)
        {
            return smethod->serialize(saver);
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

        bool interpolate(Paint* from, Paint* to, double pos_map) override
        {
            return inst->interpolate(from, to, pos_map);
        }

        ByteCounter serialize(Saver* saver) override
        {
             return inst->serialize(saver);
        }
    };
}

#endif //_TVG_PAINT_H_
