﻿/*
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
#include "tvgSwCommon.h"
#include "tvgRender.h"
#include <iostream>
#include <float.h>
#include <math.h>

/************************************************************************/
/* Internal Class Implementation                                        */
/************************************************************************/

static uint32_t _colorAlpha(uint32_t c)
{
    return (c >> 24);
}


static uint32_t _abgrJoin(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (a << 24 | b << 16 | g << 8 | r);
}


static uint32_t _argbJoin(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (a << 24 | r << 16 | g << 8 | b);
}


static bool _inverse(const Matrix* transform, Matrix* invM)
{
    // computes the inverse of a matrix m
    auto det = transform->e11 * (transform->e22 * transform->e33 - transform->e32 * transform->e23) -
               transform->e12 * (transform->e21 * transform->e33 - transform->e23 * transform->e31) +
               transform->e13 * (transform->e21 * transform->e32 - transform->e22 * transform->e31);

    if (fabsf(det) < FLT_EPSILON) return false;

    auto invDet = 1 / det;

    invM->e11 = (transform->e22 * transform->e33 - transform->e32 * transform->e23) * invDet;
    invM->e12 = (transform->e13 * transform->e32 - transform->e12 * transform->e33) * invDet;
    invM->e13 = (transform->e12 * transform->e23 - transform->e13 * transform->e22) * invDet;
    invM->e21 = (transform->e23 * transform->e31 - transform->e21 * transform->e33) * invDet;
    invM->e22 = (transform->e11 * transform->e33 - transform->e13 * transform->e31) * invDet;
    invM->e23 = (transform->e21 * transform->e13 - transform->e11 * transform->e23) * invDet;
    invM->e31 = (transform->e21 * transform->e32 - transform->e31 * transform->e22) * invDet;
    invM->e32 = (transform->e31 * transform->e12 - transform->e11 * transform->e32) * invDet;
    invM->e33 = (transform->e11 * transform->e22 - transform->e21 * transform->e12) * invDet;

    return true;
}


static bool _identify(const Matrix* transform)
{
    if (transform) {
        if (transform->e11 != 1.0f || transform->e12 != 0.0f || transform->e13 != 0.0f ||
            transform->e21 != 0.0f || transform->e22 != 1.0f || transform->e23 != 0.0f ||
            transform->e31 != 0.0f || transform->e32 != 0.0f || transform->e33 != 1.0f) {
            return false;
        }
    }

    return true;
}


static bool _translucent(const SwSurface* surface, uint8_t a)
{
    if (a < 255 || surface->blendingMode != BlendingMode::Normal) return true;
    if (!surface->compositor || surface->compositor->method == CompositeMethod::None) return false;
    return true;
}

static uint32_t _blendLayers(uint32_t src, uint32_t dst, uint8_t alpha, uint8_t ialpha, uint32_t src_blended, BlendingMode mode)
{
    // https://love2d.org/wiki/BlendMode_Formulas
    // http://www.deepskycolors.com/archive/2010/04/21/formulas-for-Photoshop-blending-modes.html
    // http://www.simplefilter.de/en/basics/mixmods.html
    // https://stackoverflow.com/questions/5919663/how-does-photoshop-blend-two-images-together
    switch (mode)
    {
        default:
        case BlendingMode::Normal:
        {
            // Normal: A + B
            return src_blended + ALPHA_BLEND(dst, ialpha);
        }
        case BlendingMode::Screen:
        {
            // Screen: 1 - (1-A)*(1-B) = A + B - A*B
            // src_blended - (src_blended * dst) >> 8 + dst
            // ((src_blended >> i) & 0xff) - ((((src_blended >> i) & 0xff) * ((dst >> i) & 0xff)) >> 8) + ((dst >> i) & 0xff)
            return (LIMIT_BYTE(((src_blended >> 16) & 0xff) - ((((src_blended >> 16) & 0xff) * ((dst >> 16) & 0xff)) >> 8) + ((dst >> 16) & 0xff)) << 16)
                    | (LIMIT_BYTE(((src_blended >> 8) & 0xff) - ((((src_blended >> 8) & 0xff) * ((dst >> 8) & 0xff)) >> 8) + ((dst >> 8) & 0xff)) << 8)
                    | LIMIT_BYTE((src_blended & 0xff) - (((src_blended & 0xff) * (dst & 0xff)) >> 8) + (dst & 0xff));
        }
        case BlendingMode::Multiply:
        {
            // Multiply: A*B
            // alpha (A*B) + ialpha B = B * (A_blended + ialpha)
            // ((dst >> i) & 0xff) * (((src_blended >> i) & 0xff) + ialpha) >> 8
            return (LIMIT_BYTE(((dst >> 16) & 0xff) * (((src_blended >> 16) & 0xff) + ialpha) >> 8) << 16)
                    | (LIMIT_BYTE(((dst >> 8) & 0xff) * (((src_blended >> 8) & 0xff) + ialpha) >> 8) << 8)
                    | LIMIT_BYTE((dst & 0xff) * ((src_blended & 0xff) + ialpha) >> 8);
        }
        case BlendingMode::Overlay:
        {
            // Overlay: B<=0.5: 2*A*B or B>0.5: 1-2*(1-A)*(1-B)
            uint32_t result = (((dst >> 16) & 0xff) <= 0x80)
                        ? ((2 * ((src >> 16) & 0xff) * ((dst >> 16) & 0xff)) >> 8) << 16 // multiply
                        : (0xff - (2*(0xff - ((src >> 16) & 0xff))*(0xff - ((dst >> 16) & 0xff)) >> 8)) << 16; // screen
            result |= (((dst >> 8) & 0xff) <= 0x80)
                        ? ((2 * ((src >> 8) & 0xff) * ((dst >> 8) & 0xff)) >> 8) << 8 // multiply
                        : (0xff - (2*(0xff - ((src >> 8) & 0xff))*(0xff - ((dst >> 8) & 0xff)) >> 8)) << 8; // screen
            result |= ((dst & 0xff) <= 0x80)
                        ? ((2 * (src & 0xff) * (dst & 0xff)) >> 8) // multiply
                        : (0xff - (2*(0xff - (src & 0xff))*(0xff - (dst & 0xff)) >> 8)); // screen
            return BLEND_COLORS(result, dst, alpha, ialpha);
        }
        case BlendingMode::Darken:
        {
            // Darken: min(A, B)
            uint32_t result = (min((src >> 16) & 0xff, (dst >> 16) & 0xff) << 16)
                    | (min((src >> 8) & 0xff, (dst >> 8) & 0xff) << 8)
                    | (min(src & 0xff, dst & 0xff));
            return BLEND_COLORS(result, dst, alpha, ialpha);
        }
        case BlendingMode::Lighten:
        {
            // Lighten: max(A, B)
            uint32_t result = (max((src >> 16) & 0xff, (dst >> 16) & 0xff) << 16)
                    | (max((src >> 8) & 0xff, (dst >> 8) & 0xff) << 8)
                    | (max(src & 0xff, dst & 0xff));
            return BLEND_COLORS(result, dst, alpha, ialpha);
        }
        case BlendingMode::ColorDodge:
        {
            // ColorDodge: B / (1-A)
            uint32_t src_inverted = 0xffffffff - src;
            uint32_t result = (((src_inverted >> 16) & 0xff) == 0) ? (dst & 0xff0000) : (LIMIT_BYTE(((dst >> 8) & 0xff00) / ((src_inverted >> 16) & 0xff)) << 16);
            result |= (((src_inverted >> 8) & 0xff) == 0) ? (dst & 0xff00) : (LIMIT_BYTE((dst & 0xff00) / ((src_inverted >> 8) & 0xff)) << 8);
            result |= ((src_inverted & 0xff) == 0) ? (dst & 0xff) : LIMIT_BYTE(((dst & 0xff) << 8) / (src_inverted & 0xff));
            return BLEND_COLORS(result, dst, alpha, ialpha);
        }
        case BlendingMode::ColorBurn:
        {
            // ColorBurn: 1 - (1-B) / A
            uint32_t dst_inverted = 0xffffffff - dst;
            uint32_t result = (((src >> 16) & 0xff) == 0) ? (dst & 0xff0000) : (LIMIT_BYTE_LOW(0xff - ((((dst_inverted >> 16) & 0xff) << 8) / ((src >> 16) & 0xff))) << 16);
            result |= (((src >> 8) & 0xff) == 0) ? (dst & 0xff00) : (LIMIT_BYTE_LOW(0xff - ((((dst_inverted >> 8) & 0xff) << 8) / ((src >> 8) & 0xff))) << 8);
            result |= ((src & 0xff) == 0) ? (dst & 0xff) : (LIMIT_BYTE_LOW(0xff - (((dst_inverted & 0xff) << 8) / (src & 0xff))));
            return BLEND_COLORS(result, dst, alpha, ialpha);
        }
        case BlendingMode::HardLight:
        {
            // HardLight Layers-inverted overlay
            uint32_t result = (((src >> 16) & 0xff) <= 0x80)
                        ? ((2 * ((dst >> 16) & 0xff) * ((src >> 16) & 0xff)) >> 8) << 16 // multiply
                        : (0xff - (2*(0xff - ((dst >> 16) & 0xff))*(0xff - ((src >> 16) & 0xff)) >> 8)) << 16; // screen
            result |= (((src >> 8) & 0xff) <= 0x80)
                        ? ((2 * ((dst >> 8) & 0xff) * ((src >> 8) & 0xff)) >> 8) << 8 // multiply
                        : (0xff - (2*(0xff - ((dst >> 8) & 0xff))*(0xff - ((src >> 8) & 0xff)) >> 8)) << 8; // screen
            result |= ((src & 0xff) <= 0x80)
                        ? ((2 * (dst & 0xff) * (src & 0xff)) >> 8) // multiply
                        : (0xff - (2*(0xff - (dst & 0xff))*(0xff - (src & 0xff)) >> 8)); // screen
            return BLEND_COLORS(result, dst, alpha, ialpha);
        }
        case BlendingMode::SoftLight:
        {
            // SoftLight: A<=0.5: (2*A-1)*(B-B^2)+B or A>0.5: (2*A-1)*(sqrt(B)-B)+B
            uint32_t result = 0x000000;
            for (uint8_t i = 0; i <= 16; i += 8) {
                uint8_t src_b = (src >> i) & 0xff;
                uint8_t dst_b = (dst >> i) & 0xff;
                uint8_t soft = ((src_b < 0x80) ? ((2 * dst_b * src_b >> 8) + (dst_b * dst_b * (0xff - 2 * src_b) >> 16)) : (sqrt((float)dst_b/255) * (2 * src_b - 0xff) + (2 * dst_b * (0xff - src_b) >> 8)));
                result |= soft << i;
            }
            return BLEND_COLORS(result, dst, alpha, ialpha);


        }
        case BlendingMode::Difference:
        {
            // Difference: |Target - Blend|
            uint32_t result = (ABS_DIFFERENCE((src >> 16) & 0xff, (dst >> 16) & 0xff) << 16)
                    | (ABS_DIFFERENCE((src >> 8) & 0xff, (dst >> 8) & 0xff) << 8)
                    | (ABS_DIFFERENCE(src & 0xff, dst & 0xff));
            return BLEND_COLORS(result, dst, alpha, ialpha);
        }
        case BlendingMode::Exclusion:
        {
            // Exclusion: 0.5 - 2*(A-0.5)*(B-0.5) = A + B - 2AB
            return (LIMIT_BYTE(((src_blended >> 16) & 0xff) - ((((src_blended >> 16) & 0xff) * ((dst >> 16) & 0xff) * 2) >> 8) + ((dst >> 16) & 0xff)) << 16)
                    | (LIMIT_BYTE(((src_blended >> 8) & 0xff) - ((((src_blended >> 8) & 0xff) * ((dst >> 8) & 0xff) * 2) >> 8) + ((dst >> 8) & 0xff)) << 8)
                    | LIMIT_BYTE((src_blended & 0xff) - (((src_blended & 0xff) * (dst & 0xff) * 2) >> 8) + (dst & 0xff));
        }
        case BlendingMode::Hue:
        {
            // Hue: TODO
            return 0;
        }
        case BlendingMode::Saturation:
        {
            // Saturation: TODO
            return 0;
        }
        case BlendingMode::Color:
        {
            // Color: TODO
            return 0;
        }
        case BlendingMode::Luminosity:
        {
            // Luminosity: TODO
            return 0;
        }
    }
}


/************************************************************************/
/* Rect                                                                 */
/************************************************************************/

static bool _translucentRect(SwSurface* surface, const SwBBox& region, uint32_t color)
{
    auto buffer = surface->buffer + (region.min.y * surface->stride) + region.min.x;
    auto h = static_cast<uint32_t>(region.max.y - region.min.y);
    auto w = static_cast<uint32_t>(region.max.x - region.min.x);
    auto ialpha = 255 - surface->blender.alpha(color);

    for (uint32_t y = 0; y < h; ++y) {
        auto dst = &buffer[y * surface->stride];
        for (uint32_t x = 0; x < w; ++x) {
            dst[x] = color + ALPHA_BLEND(dst[x], ialpha);
        }
    }
    return true;
}

static bool _translucentRectBlending(SwSurface* surface, const SwBBox& region, uint32_t color)
{
    printf("_translucentRectBlending \n"); // TODO mmaciola
    auto buffer = surface->buffer + (region.min.y * surface->stride) + region.min.x;
    auto h = static_cast<uint32_t>(region.max.y - region.min.y);
    auto w = static_cast<uint32_t>(region.max.x - region.min.x);
    auto alpha = surface->blender.alpha(color);
    auto ialpha = 255 - alpha;

    for (uint32_t y = 0; y < h; ++y) {
        auto dst = &buffer[y * surface->stride];
        for (uint32_t x = 0; x < w; ++x) {
            dst[x] = _blendLayers(color, dst[x], alpha, ialpha, color, surface->blendingMode); // color?
        }
    }
    return true;
}


static bool _translucentRectAlphaMask(SwSurface* surface, const SwBBox& region, uint32_t color)
{
    auto buffer = surface->buffer + (region.min.y * surface->stride) + region.min.x;
    auto h = static_cast<uint32_t>(region.max.y - region.min.y);
    auto w = static_cast<uint32_t>(region.max.x - region.min.x);

#ifdef THORVG_LOG_ENABLED
    cout <<"SW_ENGINE: Rectangle Alpha Mask Composition" << endl;
#endif

    auto cbuffer = surface->compositor->image.data + (region.min.y * surface->stride) + region.min.x;   //compositor buffer

    for (uint32_t y = 0; y < h; ++y) {
        auto dst = &buffer[y * surface->stride];
        auto cmp = &cbuffer[y * surface->stride];
        for (uint32_t x = 0; x < w; ++x) {
            auto tmp = ALPHA_BLEND(color, surface->blender.alpha(*cmp));
            dst[x] = tmp + ALPHA_BLEND(dst[x], 255 - surface->blender.alpha(tmp));
            ++cmp;
        }
    }
    return true;
}

static bool _translucentRectInvAlphaMask(SwSurface* surface, const SwBBox& region, uint32_t color)
{
    auto buffer = surface->buffer + (region.min.y * surface->stride) + region.min.x;
    auto h = static_cast<uint32_t>(region.max.y - region.min.y);
    auto w = static_cast<uint32_t>(region.max.x - region.min.x);

#ifdef THORVG_LOG_ENABLED
    cout <<"SW_ENGINE: Rectangle Inverse Alpha Mask Composition" << endl;
#endif

    auto cbuffer = surface->compositor->image.data + (region.min.y * surface->stride) + region.min.x;   //compositor buffer

    for (uint32_t y = 0; y < h; ++y) {
        auto dst = &buffer[y * surface->stride];
        auto cmp = &cbuffer[y * surface->stride];
        for (uint32_t x = 0; x < w; ++x) {
            auto tmp = ALPHA_BLEND(color, 255 - surface->blender.alpha(*cmp));
            dst[x] = tmp + ALPHA_BLEND(dst[x], 255 - surface->blender.alpha(tmp));
            ++cmp;
        }
    }
    return true;
}

static bool _rasterTranslucentRect(SwSurface* surface, const SwBBox& region, uint32_t color)
{
    if (surface->compositor) {
        if (surface->compositor->method == CompositeMethod::AlphaMask) {
            return _translucentRectAlphaMask(surface, region, color);
        }
        if (surface->compositor->method == CompositeMethod::InvAlphaMask) {
            return _translucentRectInvAlphaMask(surface, region, color);
        }
    }
    if (surface->blendingMode != BlendingMode::Normal) {
        return _translucentRectBlending(surface, region, color);
    }
    return _translucentRect(surface, region, color);
}


static bool _rasterSolidRect(SwSurface* surface, const SwBBox& region, uint32_t color)
{
    auto buffer = surface->buffer + (region.min.y * surface->stride);
    auto w = static_cast<uint32_t>(region.max.x - region.min.x);
    auto h = static_cast<uint32_t>(region.max.y - region.min.y);

    for (uint32_t y = 0; y < h; ++y) {
        rasterRGBA32(buffer + y * surface->stride, color, region.min.x, w);
    }
    return true;
}


/************************************************************************/
/* Rle                                                                  */
/************************************************************************/


static bool _translucentRle(SwSurface* surface, const SwRleData* rle, uint32_t color)
{
    auto span = rle->spans;
    uint32_t src;

    for (uint32_t i = 0; i < rle->size; ++i) {
        auto dst = &surface->buffer[span->y * surface->stride + span->x];
        if (span->coverage < 255) src = ALPHA_BLEND(color, span->coverage);
        else src = color;
        auto ialpha = 255 - surface->blender.alpha(src);
        for (uint32_t x = 0; x < span->len; ++x) {
            dst[x] = src + ALPHA_BLEND(dst[x], ialpha);
        }
        ++span;
    }
    return true;
}


static bool _translucentRleBlending(SwSurface* surface, SwRleData* rle, uint32_t color)
{
   printf("_translucentRleBlending \n"); // TODO mmaciola
   auto span = rle->spans;
   uint32_t src;

   for (uint32_t i = 0; i < rle->size; ++i) {
       auto dst = &surface->buffer[span->y * surface->stride + span->x];
       if (span->coverage < 255) src = ALPHA_BLEND(color, span->coverage);
       else src = color;
       auto ialpha = 255 - surface->blender.alpha(src);
       for (uint32_t x = 0; x < span->len; ++x) {
            dst[x] = _blendLayers(color, dst[x], span->coverage, ialpha, src, surface->blendingMode);
       }
       ++span;
   }
   return true;
}


static bool _translucentRleAlphaMask(SwSurface* surface, const SwRleData* rle, uint32_t color)
{
#ifdef THORVG_LOG_ENABLED
    cout <<"SW_ENGINE: Rle Alpha Mask Composition" << endl;
#endif
    auto span = rle->spans;
    uint32_t src;
    auto cbuffer = surface->compositor->image.data;

    for (uint32_t i = 0; i < rle->size; ++i) {
        auto dst = &surface->buffer[span->y * surface->stride + span->x];
        auto cmp = &cbuffer[span->y * surface->stride + span->x];
        if (span->coverage < 255) src = ALPHA_BLEND(color, span->coverage);
        else src = color;
        for (uint32_t x = 0; x < span->len; ++x) {
            auto tmp = ALPHA_BLEND(src, surface->blender.alpha(*cmp));
            dst[x] = tmp + ALPHA_BLEND(dst[x], 255 - surface->blender.alpha(tmp));
            ++cmp;
        }
        ++span;
    }
    return true;
}

static bool _translucentRleInvAlphaMask(SwSurface* surface, SwRleData* rle, uint32_t color)
{
#ifdef THORVG_LOG_ENABLED
    cout <<"SW_ENGINE: Rle Inverse Alpha Mask Composition" << endl;
#endif
    auto span = rle->spans;
    uint32_t src;
    auto cbuffer = surface->compositor->image.data;

    for (uint32_t i = 0; i < rle->size; ++i) {
        auto dst = &surface->buffer[span->y * surface->stride + span->x];
        auto cmp = &cbuffer[span->y * surface->stride + span->x];
        if (span->coverage < 255) src = ALPHA_BLEND(color, span->coverage);
        else src = color;
        for (uint32_t x = 0; x < span->len; ++x) {
            auto tmp = ALPHA_BLEND(src, 255 - surface->blender.alpha(*cmp));
            dst[x] = tmp + ALPHA_BLEND(dst[x], 255 - surface->blender.alpha(tmp));
            ++cmp;
        }
        ++span;
    }
    return true;
}

static bool _rasterTranslucentRle(SwSurface* surface, SwRleData* rle, uint32_t color)
{
    if (!rle) return false;

    if (surface->compositor) {
        if (surface->compositor->method == CompositeMethod::AlphaMask) {
            return _translucentRleAlphaMask(surface, rle, color);
        }
        if (surface->compositor->method == CompositeMethod::InvAlphaMask) {
            return _translucentRleInvAlphaMask(surface, rle, color);
        }
    }
    if (surface->blendingMode != BlendingMode::Normal) {
        return _translucentRleBlending(surface, rle, color);
    }
    return _translucentRle(surface, rle, color);
}


static bool _rasterSolidRle(SwSurface* surface, const SwRleData* rle, uint32_t color)
{
    if (!rle) return false;

    auto span = rle->spans;

    for (uint32_t i = 0; i < rle->size; ++i) {
        if (span->coverage == 255) {
            rasterRGBA32(surface->buffer + span->y * surface->stride, color, span->x, span->len);
        } else {
            auto dst = &surface->buffer[span->y * surface->stride + span->x];
            auto src = ALPHA_BLEND(color, span->coverage);
            auto ialpha = 255 - span->coverage;
            for (uint32_t i = 0; i < span->len; ++i) {
                dst[i] = src + ALPHA_BLEND(dst[i], ialpha);
            }
        }
        ++span;
    }
    return true;
}


/************************************************************************/
/* Image                                                                */
/************************************************************************/

static bool _rasterTranslucentImageRle(SwSurface* surface, const SwRleData* rle, uint32_t *img, uint32_t w, uint32_t h, uint32_t opacity)
{
    auto span = rle->spans;

    for (uint32_t i = 0; i < rle->size; ++i, ++span) {
        auto dst = &surface->buffer[span->y * surface->stride + span->x];
        auto src = img + span->x + span->y * w;    //TODO: need to use image's stride
        auto alpha = ALPHA_MULTIPLY(span->coverage, opacity);
        for (uint32_t x = 0; x < span->len; ++x, ++dst, ++src) {
            *src = ALPHA_BLEND(*src, alpha);
            *dst = *src + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(*src));
        }
    }
    return true;
}


static bool _rasterTranslucentImageRle(SwSurface* surface, const SwRleData* rle, uint32_t *img, uint32_t w, uint32_t h, uint32_t opacity, const Matrix* invTransform)
{
    auto span = rle->spans;

    for (uint32_t i = 0; i < rle->size; ++i, ++span) {
        auto ey1 = span->y * invTransform->e12 + invTransform->e13;
        auto ey2 = span->y * invTransform->e22 + invTransform->e23;
        auto dst = &surface->buffer[span->y * surface->stride + span->x];
        auto alpha = ALPHA_MULTIPLY(span->coverage, opacity);
        for (uint32_t x = 0; x < span->len; ++x, ++dst) {
            auto rX = static_cast<uint32_t>(roundf((span->x + x) * invTransform->e11 + ey1));
            auto rY = static_cast<uint32_t>(roundf((span->x + x) * invTransform->e21 + ey2));
            if (rX >= w || rY >= h) continue;
            auto src = ALPHA_BLEND(img[rY * w + rX], alpha);     //TODO: need to use image's stride
            *dst = src + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(src));
        }
    }
    return true;
}


static bool _rasterImageRle(SwSurface* surface, SwRleData* rle, uint32_t *img, uint32_t w, uint32_t h)
{
    auto span = rle->spans;

    for (uint32_t i = 0; i < rle->size; ++i, ++span) {
        auto dst = &surface->buffer[span->y * surface->stride + span->x];
        auto src = img + span->x + span->y * w;    //TODO: need to use image's stride
        for (uint32_t x = 0; x < span->len; ++x, ++dst, ++src) {
            *src = ALPHA_BLEND(*src, span->coverage);
            *dst = *src + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(*src));
        }
    }
    return true;
}


static bool _rasterImageRle(SwSurface* surface, SwRleData* rle, uint32_t *img, uint32_t w, uint32_t h, const Matrix* invTransform)
{
    auto span = rle->spans;

    for (uint32_t i = 0; i < rle->size; ++i, ++span) {
        auto ey1 = span->y * invTransform->e12 + invTransform->e13;
        auto ey2 = span->y * invTransform->e22 + invTransform->e23;
        auto dst = &surface->buffer[span->y * surface->stride + span->x];
        for (uint32_t x = 0; x < span->len; ++x, ++dst) {
            auto rX = static_cast<uint32_t>(roundf((span->x + x) * invTransform->e11 + ey1));
            auto rY = static_cast<uint32_t>(roundf((span->x + x) * invTransform->e21 + ey2));
            if (rX >= w || rY >= h) continue;
            auto src = ALPHA_BLEND(img[rY * w + rX], span->coverage);    //TODO: need to use image's stride
            *dst = src + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(src));
        }
    }
    return true;
}


static bool _translucentImage(SwSurface* surface, const uint32_t *img, uint32_t w, TVG_UNUSED uint32_t h, uint32_t opacity, const SwBBox& region, const Matrix* invTransform)
{
    auto dbuffer = &surface->buffer[region.min.y * surface->stride + region.min.x];

    for (auto y = region.min.y; y < region.max.y; ++y) {
        auto dst = dbuffer;
        auto ey1 = y * invTransform->e12 + invTransform->e13;
        auto ey2 = y * invTransform->e22 + invTransform->e23;
        for (auto x = region.min.x; x < region.max.x; ++x, ++dst) {
            auto rX = static_cast<uint32_t>(roundf(x * invTransform->e11 + ey1));
            auto rY = static_cast<uint32_t>(roundf(x * invTransform->e21 + ey2));
            if (rX >= w || rY >= h) continue;
            auto src = ALPHA_BLEND(img[rX + (rY * w)], opacity);    //TODO: need to use image's stride
            *dst = src + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(src));
        }
        dbuffer += surface->stride;
    }
    return true;
}


static bool _translucentImageBlending(SwSurface* surface, const uint32_t *img, uint32_t w, TVG_UNUSED uint32_t h, uint32_t opacity, const SwBBox& region, const Matrix* invTransform)
{
    printf("_translucentImageBlending B \n"); // TODO mmaciola
    auto dbuffer = &surface->buffer[region.min.y * surface->stride + region.min.x];

    for (auto y = region.min.y; y < region.max.y; ++y) {
        auto dst = dbuffer;
        auto ey1 = y * invTransform->e12 + invTransform->e13;
        auto ey2 = y * invTransform->e22 + invTransform->e23;
        for (auto x = region.min.x; x < region.max.x; ++x, ++dst) {
            auto rX = static_cast<uint32_t>(roundf(x * invTransform->e11 + ey1));
            auto rY = static_cast<uint32_t>(roundf(x * invTransform->e21 + ey2));
            if (rX >= w || rY >= h) continue;
            auto src = img[rX + (rY * w)];
            auto src_blended = ALPHA_BLEND(src, opacity);
            auto ialpha = 255 - surface->blender.alpha(src_blended);
            *dst = _blendLayers(img[rX + (rY * w)], *dst, opacity, ialpha, src_blended, surface->blendingMode);
        }
        dbuffer += surface->stride;
    }
    return true;
}


static bool _translucentImageAlphaMask(SwSurface* surface, const uint32_t *img, uint32_t w, TVG_UNUSED uint32_t h, uint32_t opacity, const SwBBox& region, const Matrix* invTransform)
{
#ifdef THORVG_LOG_ENABLED
    cout <<"SW_ENGINE: Transformed Image Alpha Mask Composition" << endl;
#endif
    auto dbuffer = &surface->buffer[region.min.y * surface->stride + region.min.x];
    auto cbuffer = &surface->compositor->image.data[region.min.y * surface->stride + region.min.x];

    for (auto y = region.min.y; y < region.max.y; ++y) {
        auto dst = dbuffer;
        auto cmp = cbuffer;
        float ey1 = y * invTransform->e12 + invTransform->e13;
        float ey2 = y * invTransform->e22 + invTransform->e23;
        for (auto x = region.min.x; x < region.max.x; ++x, ++dst, ++cmp) {
            auto rX = static_cast<uint32_t>(roundf(x * invTransform->e11 + ey1));
            auto rY = static_cast<uint32_t>(roundf(x * invTransform->e21 + ey2));
            if (rX >= w || rY >= h) continue;
            auto tmp = ALPHA_BLEND(img[rX + (rY * w)], ALPHA_MULTIPLY(opacity, surface->blender.alpha(*cmp)));  //TODO: need to use image's stride
            *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
        }
        dbuffer += surface->stride;
        cbuffer += surface->stride;
    }
    return true;
}

static bool _translucentImageInvAlphaMask(SwSurface* surface, const uint32_t *img, uint32_t w, uint32_t h, uint32_t opacity, const SwBBox& region, const Matrix* invTransform)
{
#ifdef THORVG_LOG_ENABLED
    cout <<"SW_ENGINE: Transformed Image Inverse Alpha Mask Composition" << endl;
#endif
    auto dbuffer = &surface->buffer[region.min.y * surface->stride + region.min.x];
    auto cbuffer = &surface->compositor->image.data[region.min.y * surface->stride + region.min.x];

    for (auto y = region.min.y; y < region.max.y; ++y) {
        auto dst = dbuffer;
        auto cmp = cbuffer;
        float ey1 = y * invTransform->e12 + invTransform->e13;
        float ey2 = y * invTransform->e22 + invTransform->e23;
        for (auto x = region.min.x; x < region.max.x; ++x, ++dst, ++cmp) {
            auto rX = static_cast<uint32_t>(roundf(x * invTransform->e11 + ey1));
            auto rY = static_cast<uint32_t>(roundf(x * invTransform->e21 + ey2));
            if (rX >= w || rY >= h) continue;
            auto tmp = ALPHA_BLEND(img[rX + (rY * w)], ALPHA_MULTIPLY(opacity, 255 - surface->blender.alpha(*cmp)));  //TODO: need to use image's stride
            *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
        }
        dbuffer += surface->stride;
        cbuffer += surface->stride;
    }
    return true;
}

static bool _rasterTranslucentImage(SwSurface* surface, const uint32_t *img, uint32_t w, uint32_t h, uint32_t opacity, const SwBBox& region, const Matrix* invTransform)
{
    if (surface->compositor) {
        if (surface->compositor->method == CompositeMethod::AlphaMask) {
            return _translucentImageAlphaMask(surface, img, w, h, opacity, region, invTransform);
        }
        if (surface->compositor->method == CompositeMethod::InvAlphaMask) {
            return _translucentImageInvAlphaMask(surface, img, w, h, opacity, region, invTransform);
        }
    }
    if (surface->blendingMode != BlendingMode::Normal) {
        return _translucentImageBlending(surface, img, w, h, opacity, region, invTransform);
    }
    return _translucentImage(surface, img, w, h, opacity, region, invTransform);
}


static bool _translucentImage(SwSurface* surface, uint32_t *img, uint32_t w, uint32_t h, uint32_t opacity, const SwBBox& region)
{
    auto dbuffer = &surface->buffer[region.min.y * surface->stride + region.min.x];
    auto sbuffer = img + region.min.x + region.min.y * w;    //TODO: need to use image's stride

    for (auto y = region.min.y; y < region.max.y; ++y) {
        auto dst = dbuffer;
        auto src = sbuffer;
        for (auto x = region.min.x; x < region.max.x; ++x, ++dst, ++src) {
            auto p = ALPHA_BLEND(*src, opacity);
            *dst = p + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(p));
        }
        dbuffer += surface->stride;
        sbuffer += w;    //TODO: need to use image's stride
    }
    return true;
}


static bool _translucentImageBlending(SwSurface* surface, uint32_t *img, uint32_t w, uint32_t h, uint32_t opacity, const SwBBox& region)
{
    printf("_translucentImageBlending A \n"); // TODO mmaciola
    auto dbuffer = &surface->buffer[region.min.y * surface->stride + region.min.x];
    auto sbuffer = img + region.min.x + region.min.y * w;    //TODO: need to use image's stride

    for (auto y = region.min.y; y < region.max.y; ++y) {
        auto dst = dbuffer;
        auto src = sbuffer;
        for (auto x = region.min.x; x < region.max.x; ++x, ++dst, ++src) {
            auto src_blended = ALPHA_BLEND(*src, opacity);
            auto ialpha = 255 - surface->blender.alpha(src_blended);
            *dst = _blendLayers(*src, *dst, opacity, ialpha, src_blended, surface->blendingMode);
        }
        dbuffer += surface->stride;
        sbuffer += w;    //TODO: need to use image's stride
    }
    return true;
}


static bool _translucentImageAlphaMask(SwSurface* surface, uint32_t *img, uint32_t w, uint32_t h, uint32_t opacity, const SwBBox& region)
{
    auto buffer = surface->buffer + (region.min.y * surface->stride) + region.min.x;
    auto h2 = static_cast<uint32_t>(region.max.y - region.min.y);
    auto w2 = static_cast<uint32_t>(region.max.x - region.min.x);

#ifdef THORVG_LOG_ENABLED
    cout <<"SW_ENGINE: Image Alpha Mask Composition" << endl;
#endif

    auto sbuffer = img + (region.min.y * w) + region.min.x;
    auto cbuffer = surface->compositor->image.data + (region.min.y * surface->stride) + region.min.x;   //compositor buffer

    for (uint32_t y = 0; y < h2; ++y) {
        auto dst = buffer;
        auto cmp = cbuffer;
        auto src = sbuffer;
        for (uint32_t x = 0; x < w2; ++x, ++dst, ++src, ++cmp) {
            auto tmp = ALPHA_BLEND(*src, ALPHA_MULTIPLY(opacity, surface->blender.alpha(*cmp)));
            *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
        }
        buffer += surface->stride;
        cbuffer += surface->stride;
        sbuffer += w;   //TODO: need to use image's stride
    }
    return true;
}


static bool _translucentImageInvAlphaMask(SwSurface* surface, uint32_t *img, uint32_t w, uint32_t h, uint32_t opacity, const SwBBox& region)
{
    auto buffer = surface->buffer + (region.min.y * surface->stride) + region.min.x;
    auto h2 = static_cast<uint32_t>(region.max.y - region.min.y);
    auto w2 = static_cast<uint32_t>(region.max.x - region.min.x);

#ifdef THORVG_LOG_ENABLED
    cout <<"SW_ENGINE: Image Inverse Alpha Mask Composition" << endl;
#endif

    auto sbuffer = img + (region.min.y * w) + region.min.x;
    auto cbuffer = surface->compositor->image.data + (region.min.y * surface->stride) + region.min.x;   //compositor buffer

    for (uint32_t y = 0; y < h2; ++y) {
        auto dst = buffer;
        auto cmp = cbuffer;
        auto src = sbuffer;
        for (uint32_t x = 0; x < w2; ++x, ++dst, ++src, ++cmp) {
            auto tmp = ALPHA_BLEND(*src, ALPHA_MULTIPLY(opacity, 255 - surface->blender.alpha(*cmp)));
            *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
        }
        buffer += surface->stride;
        cbuffer += surface->stride;
        sbuffer += w;   //TODO: need to use image's stride
    }
    return true;
}

static bool _rasterTranslucentImage(SwSurface* surface, uint32_t *img, uint32_t w, uint32_t h, uint32_t opacity, const SwBBox& region)
{
    if (surface->compositor) {
        if (surface->compositor->method == CompositeMethod::AlphaMask) {
            return _translucentImageAlphaMask(surface, img, w, h, opacity, region);
        }
        if (surface->compositor->method == CompositeMethod::InvAlphaMask) {
            return _translucentImageInvAlphaMask(surface, img, w, h, opacity, region);
        }
    }
    if (surface->blendingMode != BlendingMode::Normal) {
        return _translucentImageBlending(surface, img, w, h, opacity, region);
    }
    return _translucentImage(surface, img, w, h, opacity, region);
}


static bool _rasterImage(SwSurface* surface, uint32_t *img, uint32_t w, TVG_UNUSED uint32_t h, const SwBBox& region)
{
    auto dbuffer = &surface->buffer[region.min.y * surface->stride + region.min.x];
    auto sbuffer = img + region.min.x + region.min.y * w;   //TODO: need to use image's stride

    for (auto y = region.min.y; y < region.max.y; ++y) {
        auto dst = dbuffer;
        auto src = sbuffer;
        for (auto x = region.min.x; x < region.max.x; x++, dst++, src++) {
            *dst = *src + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(*src));
        }
        dbuffer += surface->stride;
        sbuffer += w;    //TODO: need to use image's stride
    }
    return true;
}


static bool _rasterImage(SwSurface* surface, const uint32_t *img, uint32_t w, uint32_t h, const SwBBox& region, const Matrix* invTransform)
{
    for (auto y = region.min.y; y < region.max.y; ++y) {
        auto dst = &surface->buffer[y * surface->stride + region.min.x];
        auto ey1 = y * invTransform->e12 + invTransform->e13;
        auto ey2 = y * invTransform->e22 + invTransform->e23;
        for (auto x = region.min.x; x < region.max.x; ++x, ++dst) {
            auto rX = static_cast<uint32_t>(roundf(x * invTransform->e11 + ey1));
            auto rY = static_cast<uint32_t>(roundf(x * invTransform->e21 + ey2));
            if (rX >= w || rY >= h) continue;
            auto src = img[rX + (rY * w)];    //TODO: need to use image's stride
            *dst = src + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(src));
        }
    }
    return true;
}


/************************************************************************/
/* Gradient                                                             */
/************************************************************************/

static bool _rasterTranslucentLinearGradientRect(SwSurface* surface, const SwBBox& region, const SwFill* fill)
{
    if (fill->linear.len < FLT_EPSILON) return false;

    auto buffer = surface->buffer + (region.min.y * surface->stride) + region.min.x;
    auto h = static_cast<uint32_t>(region.max.y - region.min.y);
    auto w = static_cast<uint32_t>(region.max.x - region.min.x);

    auto sbuffer = static_cast<uint32_t*>(alloca(w * sizeof(uint32_t)));
    if (!sbuffer) return false;

    if (surface->compositor) {
        auto method = surface->compositor->method;
        auto cbuffer = surface->compositor->image.data + (region.min.y * surface->stride) + region.min.x;

        if (method == CompositeMethod::AlphaMask) {
            for (uint32_t y = 0; y < h; ++y) {
                fillFetchLinear(fill, sbuffer, region.min.y + y, region.min.x, w);
                auto dst = buffer;
                auto cmp = cbuffer;
                auto src = sbuffer;
                for (uint32_t x = 0; x < w; ++x, ++dst, ++cmp, ++src) {
                    auto tmp = ALPHA_BLEND(*src, surface->blender.alpha(*cmp));
                    *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
                }
                buffer += surface->stride;
                cbuffer += surface->stride;
            }
            return true;
        } else if (method == CompositeMethod::InvAlphaMask) {
            for (uint32_t y = 0; y < h; ++y) {
                fillFetchLinear(fill, sbuffer, region.min.y + y, region.min.x, w);
                auto dst = buffer;
                auto cmp = cbuffer;
                auto src = sbuffer;
                for (uint32_t x = 0; x < w; ++x, ++dst, ++cmp, ++src) {
                    auto tmp = ALPHA_BLEND(*src, 255 - surface->blender.alpha(*cmp));
                    *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
                }
                buffer += surface->stride;
                cbuffer += surface->stride;
            }
            return true;
        }
    }

    if (surface->blendingMode != BlendingMode::Normal) {
        // TODO
    }

    for (uint32_t y = 0; y < h; ++y) {
        auto dst = &buffer[y * surface->stride];
        fillFetchLinear(fill, sbuffer, region.min.y + y, region.min.x, w);
        for (uint32_t x = 0; x < w; ++x) {
            dst[x] = sbuffer[x] + ALPHA_BLEND(dst[x], 255 - surface->blender.alpha(sbuffer[x]));
        }
    }
    return true;
}


static bool _rasterOpaqueLinearGradientRect(SwSurface* surface, const SwBBox& region, const SwFill* fill)
{
    if (fill->linear.len < FLT_EPSILON) return false;

    auto buffer = surface->buffer + (region.min.y * surface->stride) + region.min.x;
    auto h = static_cast<uint32_t>(region.max.y - region.min.y);
    auto w = static_cast<uint32_t>(region.max.x - region.min.x);

    for (uint32_t y = 0; y < h; ++y) {
        fillFetchLinear(fill, buffer + y * surface->stride, region.min.y + y, region.min.x, w);
    }
    return true;
}


static bool _rasterTranslucentRadialGradientRect(SwSurface* surface, const SwBBox& region, const SwFill* fill)
{
    if (fill->radial.a < FLT_EPSILON) return false;

    auto buffer = surface->buffer + (region.min.y * surface->stride) + region.min.x;
    auto h = static_cast<uint32_t>(region.max.y - region.min.y);
    auto w = static_cast<uint32_t>(region.max.x - region.min.x);

    auto sbuffer = static_cast<uint32_t*>(alloca(w * sizeof(uint32_t)));
    if (!sbuffer) return false;

    if (surface->compositor) {
        auto method = surface->compositor->method;
        auto cbuffer = surface->compositor->image.data + (region.min.y * surface->stride) + region.min.x;

        if (method == CompositeMethod::AlphaMask) {
            for (uint32_t y = 0; y < h; ++y) {
                fillFetchRadial(fill, sbuffer, region.min.y + y, region.min.x, w);
                auto dst = buffer;
                auto cmp = cbuffer;
                auto src = sbuffer;
                for (uint32_t x = 0; x < w; ++x, ++dst, ++cmp, ++src) {
                    auto tmp = ALPHA_BLEND(*src, surface->blender.alpha(*cmp));
                    *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
                }
                buffer += surface->stride;
                cbuffer += surface->stride;
            }
            return true;
        } else if (method == CompositeMethod::InvAlphaMask) {
            for (uint32_t y = 0; y < h; ++y) {
                fillFetchRadial(fill, sbuffer, region.min.y + y, region.min.x, w);
                auto dst = buffer;
                auto cmp = cbuffer;
                auto src = sbuffer;
                for (uint32_t x = 0; x < w; ++x, ++dst, ++cmp, ++src) {
                    auto tmp = ALPHA_BLEND(*src, 255 - surface->blender.alpha(*cmp));
                    *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
                }
                buffer += surface->stride;
                cbuffer += surface->stride;
            }
            return true;
        }
    }

    if (surface->blendingMode != BlendingMode::Normal) {
        // TODO
    }

    for (uint32_t y = 0; y < h; ++y) {
        auto dst = &buffer[y * surface->stride];
        fillFetchRadial(fill, sbuffer, region.min.y + y, region.min.x, w);
        for (uint32_t x = 0; x < w; ++x) {
            dst[x] = sbuffer[x] + ALPHA_BLEND(dst[x], 255 - surface->blender.alpha(sbuffer[x]));
        }
    }
    return true;
}


static bool _rasterOpaqueRadialGradientRect(SwSurface* surface, const SwBBox& region, const SwFill* fill)
{
    if (fill->radial.a < FLT_EPSILON) return false;

    auto buffer = surface->buffer + (region.min.y * surface->stride) + region.min.x;
    auto h = static_cast<uint32_t>(region.max.y - region.min.y);
    auto w = static_cast<uint32_t>(region.max.x - region.min.x);

    for (uint32_t y = 0; y < h; ++y) {
        auto dst = &buffer[y * surface->stride];
        fillFetchRadial(fill, dst, region.min.y + y, region.min.x, w);
    }
    return true;
}


static bool _rasterTranslucentLinearGradientRle(SwSurface* surface, const SwRleData* rle, const SwFill* fill)
{
    if (fill->linear.len < FLT_EPSILON) return false;

    auto buf = static_cast<uint32_t*>(alloca(surface->w * sizeof(uint32_t)));
    if (!buf) return false;

    auto span = rle->spans;

    if (surface->compositor) {
        auto method = surface->compositor->method;
        auto cbuffer = surface->compositor->image.data;

        if (method == CompositeMethod::AlphaMask) {
            for (uint32_t i = 0; i < rle->size; ++i, ++span) {
                fillFetchLinear(fill, buf, span->y, span->x, span->len);
                auto dst = &surface->buffer[span->y * surface->stride + span->x];
                auto cmp = &cbuffer[span->y * surface->stride + span->x];
                auto src = buf;
                if (span->coverage == 255) {
                    for (uint32_t x = 0; x < span->len; ++x, ++dst, ++cmp, ++src) {
                        auto tmp = ALPHA_BLEND(*src, surface->blender.alpha(*cmp));
                        *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
                    }
                } else {
                    auto ialpha = 255 - span->coverage;
                    for (uint32_t x = 0; x < span->len; ++x, ++dst, ++cmp, ++src) {
                        auto tmp = ALPHA_BLEND(*src, surface->blender.alpha(*cmp));
                        tmp = ALPHA_BLEND(tmp, span->coverage) + ALPHA_BLEND(*dst, ialpha);
                        *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
                    }
                }
            }
            return true;
        } else if (method == CompositeMethod::InvAlphaMask) {
            for (uint32_t i = 0; i < rle->size; ++i, ++span) {
                fillFetchLinear(fill, buf, span->y, span->x, span->len);
                auto dst = &surface->buffer[span->y * surface->stride + span->x];
                auto cmp = &cbuffer[span->y * surface->stride + span->x];
                auto src = buf;
                if (span->coverage == 255) {
                   for (uint32_t x = 0; x < span->len; ++x, ++dst, ++cmp, ++src) {
                        auto tmp = ALPHA_BLEND(*src, 255 - surface->blender.alpha(*cmp));
                        *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
                    }
                } else {
                    auto ialpha = 255 - span->coverage;
                    for (uint32_t x = 0; x < span->len; ++x, ++dst, ++cmp, ++src) {
                        auto tmp = ALPHA_BLEND(*src, 255 - surface->blender.alpha(*cmp));
                        tmp = ALPHA_BLEND(tmp, span->coverage) + ALPHA_BLEND(*dst, ialpha);
                        *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
                    }
                }
            }
            return true;
        }
    }

    if (surface->blendingMode != BlendingMode::Normal) {
        // TODO
    }

    for (uint32_t i = 0; i < rle->size; ++i) {
        auto dst = &surface->buffer[span->y * surface->stride + span->x];
        fillFetchLinear(fill, buf, span->y, span->x, span->len);
        if (span->coverage == 255) {
            for (uint32_t i = 0; i < span->len; ++i) {
                dst[i] = buf[i] + ALPHA_BLEND(dst[i], 255 - surface->blender.alpha(buf[i]));
            }
        } else {
            for (uint32_t i = 0; i < span->len; ++i) {
                auto tmp = ALPHA_BLEND(buf[i], span->coverage);
                dst[i] = tmp + ALPHA_BLEND(dst[i], 255 - surface->blender.alpha(tmp));
            }
        }
        ++span;
    }
    return true;
}


static bool _rasterOpaqueLinearGradientRle(SwSurface* surface, const SwRleData* rle, const SwFill* fill)
{
    if (fill->linear.len < FLT_EPSILON) return false;

    auto buf = static_cast<uint32_t*>(alloca(surface->w * sizeof(uint32_t)));
    if (!buf) return false;

    auto span = rle->spans;

    for (uint32_t i = 0; i < rle->size; ++i) {
        if (span->coverage == 255) {
            fillFetchLinear(fill, surface->buffer + span->y * surface->stride + span->x, span->y, span->x, span->len);
        } else {
            fillFetchLinear(fill, buf, span->y, span->x, span->len);
            auto ialpha = 255 - span->coverage;
            auto dst = &surface->buffer[span->y * surface->stride + span->x];
            for (uint32_t i = 0; i < span->len; ++i) {
                dst[i] = ALPHA_BLEND(buf[i], span->coverage) + ALPHA_BLEND(dst[i], ialpha);
            }
        }
        ++span;
    }
    return true;
}


static bool _rasterTranslucentRadialGradientRle(SwSurface* surface, const SwRleData* rle, const SwFill* fill)
{
    if (fill->radial.a < FLT_EPSILON) return false;

    auto buf = static_cast<uint32_t*>(alloca(surface->w * sizeof(uint32_t)));
    if (!buf) return false;

    auto span = rle->spans;

    if (surface->compositor) {
        auto method = surface->compositor->method;
        auto cbuffer = surface->compositor->image.data;

        if (method == CompositeMethod::AlphaMask) {
            for (uint32_t i = 0; i < rle->size; ++i, ++span) {
                fillFetchRadial(fill, buf, span->y, span->x, span->len);
                auto dst = &surface->buffer[span->y * surface->stride + span->x];
                auto cmp = &cbuffer[span->y * surface->stride + span->x];
                auto src = buf;
                if (span->coverage == 255) {
                    for (uint32_t x = 0; x < span->len; ++x, ++dst, ++cmp, ++src) {
                        auto tmp = ALPHA_BLEND(*src, surface->blender.alpha(*cmp));
                        *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
                    }
                } else {
                    auto ialpha = 255 - span->coverage;
                    for (uint32_t x = 0; x < span->len; ++x, ++dst, ++cmp, ++src) {
                        auto tmp = ALPHA_BLEND(*src, surface->blender.alpha(*cmp));
                        tmp = ALPHA_BLEND(tmp, span->coverage) + ALPHA_BLEND(*dst, ialpha);
                        *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
                    }
                }
            }
            return true;
        } else if (method == CompositeMethod::InvAlphaMask) {
            for (uint32_t i = 0; i < rle->size; ++i, ++span) {
                fillFetchRadial(fill, buf, span->y, span->x, span->len);
                auto dst = &surface->buffer[span->y * surface->stride + span->x];
                auto cmp = &cbuffer[span->y * surface->stride + span->x];
                auto src = buf;
                if (span->coverage == 255) {
                   for (uint32_t x = 0; x < span->len; ++x, ++dst, ++cmp, ++src) {
                        auto tmp = ALPHA_BLEND(*src, 255 - surface->blender.alpha(*cmp));
                        *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
                    }
                } else {
                    auto ialpha = 255 - span->coverage;
                    for (uint32_t x = 0; x < span->len; ++x, ++dst, ++cmp, ++src) {
                        auto tmp = ALPHA_BLEND(*src, 255 - surface->blender.alpha(*cmp));
                        tmp = ALPHA_BLEND(tmp, span->coverage) + ALPHA_BLEND(*dst, ialpha);
                        *dst = tmp + ALPHA_BLEND(*dst, 255 - surface->blender.alpha(tmp));
                    }
                }
            }
            return true;
        }
    }

    if (surface->blendingMode != BlendingMode::Normal) {
        // TODO
    }

    for (uint32_t i = 0; i < rle->size; ++i) {
        auto dst = &surface->buffer[span->y * surface->stride + span->x];
        fillFetchRadial(fill, buf, span->y, span->x, span->len);
        if (span->coverage == 255) {
            for (uint32_t i = 0; i < span->len; ++i) {
                dst[i] = buf[i] + ALPHA_BLEND(dst[i], 255 - surface->blender.alpha(buf[i]));
            }
        } else {
            for (uint32_t i = 0; i < span->len; ++i) {
                auto tmp = ALPHA_BLEND(buf[i], span->coverage);
                dst[i] = tmp + ALPHA_BLEND(dst[i], 255 - surface->blender.alpha(tmp));
            }
        }
        ++span;
    }
    return true;
}


static bool _rasterOpaqueRadialGradientRle(SwSurface* surface, const SwRleData* rle, const SwFill* fill)
{
    if (fill->radial.a < FLT_EPSILON) return false;

    auto buf = static_cast<uint32_t*>(alloca(surface->w * sizeof(uint32_t)));
    if (!buf) return false;

    auto span = rle->spans;

    for (uint32_t i = 0; i < rle->size; ++i) {
        auto dst = &surface->buffer[span->y * surface->stride + span->x];
        if (span->coverage == 255) {
            fillFetchRadial(fill, dst, span->y, span->x, span->len);
        } else {
            fillFetchRadial(fill, buf, span->y, span->x, span->len);
            auto ialpha = 255 - span->coverage;
            for (uint32_t i = 0; i < span->len; ++i) {
                dst[i] = ALPHA_BLEND(buf[i], span->coverage) + ALPHA_BLEND(dst[i], ialpha);
            }
        }
        ++span;
    }
    return true;
}


/************************************************************************/
/* External Class Implementation                                        */
/************************************************************************/

bool rasterCompositor(SwSurface* surface)
{
    if (surface->cs == SwCanvas::ABGR8888) {
        surface->blender.alpha = _colorAlpha;
        surface->blender.join = _abgrJoin;
    } else if (surface->cs == SwCanvas::ARGB8888) {
        surface->blender.alpha = _colorAlpha;
        surface->blender.join = _argbJoin;
    } else {
        //What Color Space ???
        return false;
    }

    return true;
}


bool rasterGradientShape(SwSurface* surface, SwShape* shape, unsigned id)
{
    if (!shape->fill) return false;

    auto translucent = shape->fill->translucent || (surface->compositor && surface->compositor->method != CompositeMethod::None);

    //Fast Track
    if (shape->rect) {
        if (id == FILL_ID_LINEAR) {
            if (translucent) return _rasterTranslucentLinearGradientRect(surface, shape->bbox, shape->fill);
            return _rasterOpaqueLinearGradientRect(surface, shape->bbox, shape->fill);
        } else {
            if (translucent) return _rasterTranslucentRadialGradientRect(surface, shape->bbox, shape->fill);
            return _rasterOpaqueRadialGradientRect(surface, shape->bbox, shape->fill);
        }
    } else {
        if (!shape->rle) return false;
        if (id == FILL_ID_LINEAR) {
            if (translucent) return _rasterTranslucentLinearGradientRle(surface, shape->rle, shape->fill);
            return _rasterOpaqueLinearGradientRle(surface, shape->rle, shape->fill);
        } else {
            if (translucent) return _rasterTranslucentRadialGradientRle(surface, shape->rle, shape->fill);
            return _rasterOpaqueRadialGradientRle(surface, shape->rle, shape->fill);
        }
    }
    return false;
}


bool rasterSolidShape(SwSurface* surface, SwShape* shape, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (a < 255) {
        r = ALPHA_MULTIPLY(r, a);
        g = ALPHA_MULTIPLY(g, a);
        b = ALPHA_MULTIPLY(b, a);
    }
    auto color = surface->blender.join(r, g, b, a);
    auto translucent = _translucent(surface, a);

    //Fast Track
    if (shape->rect) {
        if (translucent) return _rasterTranslucentRect(surface, shape->bbox, color);
        return _rasterSolidRect(surface, shape->bbox, color);
    }
    if (translucent) {
        return _rasterTranslucentRle(surface, shape->rle, color);
    }
    return _rasterSolidRle(surface, shape->rle, color);
}


bool rasterStroke(SwSurface* surface, SwShape* shape, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (a < 255) {
        r = ALPHA_MULTIPLY(r, a);
        g = ALPHA_MULTIPLY(g, a);
        b = ALPHA_MULTIPLY(b, a);
    }

    auto color = surface->blender.join(r, g, b, a);
    auto translucent = _translucent(surface, a);

    if (translucent) return _rasterTranslucentRle(surface, shape->strokeRle, color);
    return _rasterSolidRle(surface, shape->strokeRle, color);
}


bool rasterGradientStroke(SwSurface* surface, SwShape* shape, unsigned id)
{
    if (!shape->stroke || !shape->stroke->fill || !shape->strokeRle) return false;

    auto translucent = shape->stroke->fill->translucent || (surface->compositor && surface->compositor->method != CompositeMethod::None);

    if (id == FILL_ID_LINEAR) {
        if (translucent) return _rasterTranslucentLinearGradientRle(surface, shape->strokeRle, shape->stroke->fill);
        return _rasterOpaqueLinearGradientRle(surface, shape->strokeRle, shape->stroke->fill);
    } else {
        if (translucent) return _rasterTranslucentRadialGradientRle(surface, shape->strokeRle, shape->stroke->fill);
        return _rasterOpaqueRadialGradientRle(surface, shape->strokeRle, shape->stroke->fill);
    }

    return false;
}


bool rasterClear(SwSurface* surface)
{
    if (!surface || !surface->buffer || surface->stride <= 0 || surface->w <= 0 || surface->h <= 0) return false;

    if (surface->w == surface->stride) {
        rasterRGBA32(surface->buffer, 0x00000000, 0, surface->w * surface->h);
    } else {
        for (uint32_t i = 0; i < surface->h; i++) {
            rasterRGBA32(surface->buffer + surface->stride * i, 0x00000000, 0, surface->w);
        }
    }
    return true;
}


bool rasterImage(SwSurface* surface, SwImage* image, const Matrix* transform, const SwBBox& bbox, uint32_t opacity)
{
    Matrix invTransform;

    if (transform) {
        if (!_inverse(transform, &invTransform)) return false;
    }
    else invTransform = {1, 0, 0, 0, 1, 0, 0, 0, 1};

    auto translucent = _translucent(surface, opacity);

    if (image->rle) {
        //Fast track
        if (_identify(transform)) {
            //OPTIMIZE ME: Support non transformed image. Only shifted image can use these routines.
            if (translucent) return _rasterTranslucentImageRle(surface, image->rle, image->data, image->w, image->h, opacity);
            return _rasterImageRle(surface, image->rle, image->data, image->w, image->h);
        } else {
            if (translucent) return _rasterTranslucentImageRle(surface, image->rle, image->data, image->w, image->h, opacity, &invTransform);
            return _rasterImageRle(surface, image->rle, image->data, image->w, image->h, &invTransform);
        }
    }
    else {
        //Fast track
        if (_identify(transform)) {
            //OPTIMIZE ME: Support non transformed image. Only shifted image can use these routines.
            if (translucent) return _rasterTranslucentImage(surface, image->data, image->w, image->h, opacity, bbox);
            else return _rasterImage(surface, image->data, image->w, image->h, bbox);
        } else {
            if (translucent) return _rasterTranslucentImage(surface, image->data, image->w, image->h, opacity, bbox, &invTransform);
            else return _rasterImage(surface, image->data, image->w, image->h, bbox, &invTransform);
        }
    }
}
