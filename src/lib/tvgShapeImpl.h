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
#ifndef _TVG_SHAPE_IMPL_H_
#define _TVG_SHAPE_IMPL_H_

#include <memory.h>
#include "tvgPaint.h"

/************************************************************************/
/* Internal Class Implementation                                        */
/************************************************************************/

struct ShapeStroke
{
    float width = 0;
    uint8_t color[4] = {0, 0, 0, 0};
    Fill *fill = nullptr;
    float* dashPattern = nullptr;
    uint32_t dashCnt = 0;
    StrokeCap cap = StrokeCap::Square;
    StrokeJoin join = StrokeJoin::Bevel;

    ShapeStroke() {}

    ShapeStroke(const ShapeStroke* src)
     : width(src->width),
       dashCnt(src->dashCnt),
       cap(src->cap),
       join(src->join)
    {
        memcpy(color, src->color, sizeof(color));
        dashPattern = static_cast<float*>(malloc(sizeof(float) * dashCnt));
        memcpy(dashPattern, src->dashPattern, sizeof(float) * dashCnt);
        if (src->fill)
            fill = src->fill->duplicate();
    }

    ~ShapeStroke()
    {
        if (dashPattern) free(dashPattern);
        if (fill) delete(fill);
    }
};


struct ShapePath
{
    PathCommand* cmds = nullptr;
    uint32_t cmdCnt = 0;
    uint32_t reservedCmdCnt = 0;

    Point *pts = nullptr;
    uint32_t ptsCnt = 0;
    uint32_t reservedPtsCnt = 0;

    ~ShapePath()
    {
        if (cmds) free(cmds);
        if (pts) free(pts);
    }

    ShapePath()
    {
    }

    void duplicate(const ShapePath* src)
    {
        cmdCnt = src->cmdCnt;
        reservedCmdCnt = src->reservedCmdCnt;
        ptsCnt = src->ptsCnt;
        reservedPtsCnt = src->reservedPtsCnt;

        cmds = static_cast<PathCommand*>(malloc(sizeof(PathCommand) * reservedCmdCnt));
        if (!cmds) return;
        memcpy(cmds, src->cmds, sizeof(PathCommand) * cmdCnt);

        pts = static_cast<Point*>(malloc(sizeof(Point) * reservedPtsCnt));
        if (!pts) {
            free(cmds);
            return;
        }
        memcpy(pts, src->pts, sizeof(Point) * ptsCnt);
    }

    void reserveCmd(uint32_t cmdCnt)
    {
        if (cmdCnt <= reservedCmdCnt) return;
        reservedCmdCnt = cmdCnt;
        cmds = static_cast<PathCommand*>(realloc(cmds, sizeof(PathCommand) * reservedCmdCnt));
    }

    void reservePts(uint32_t ptsCnt)
    {
        if (ptsCnt <= reservedPtsCnt) return;
        reservedPtsCnt = ptsCnt;
        pts = static_cast<Point*>(realloc(pts, sizeof(Point) * reservedPtsCnt));
    }

    void grow(uint32_t cmdCnt, uint32_t ptsCnt)
    {
        reserveCmd(this->cmdCnt + cmdCnt);
        reservePts(this->ptsCnt + ptsCnt);
    }

    void reset()
    {
        cmdCnt = 0;
        ptsCnt = 0;
    }

    void append(const PathCommand* cmds, uint32_t cmdCnt, const Point* pts, uint32_t ptsCnt)
    {
        memcpy(this->cmds + this->cmdCnt, cmds, sizeof(PathCommand) * cmdCnt);
        memcpy(this->pts + this->ptsCnt, pts, sizeof(Point) * ptsCnt);
        this->cmdCnt += cmdCnt;
        this->ptsCnt += ptsCnt;
    }

    void moveTo(float x, float y)
    {
        if (cmdCnt + 1 > reservedCmdCnt) reserveCmd((cmdCnt + 1) * 2);
        if (ptsCnt + 2 > reservedPtsCnt) reservePts((ptsCnt + 2) * 2);

        cmds[cmdCnt++] = PathCommand::MoveTo;
        pts[ptsCnt++] = {x, y};
    }

    void lineTo(float x, float y)
    {
        if (cmdCnt + 1 > reservedCmdCnt) reserveCmd((cmdCnt + 1) * 2);
        if (ptsCnt + 2 > reservedPtsCnt) reservePts((ptsCnt + 2) * 2);

        cmds[cmdCnt++] = PathCommand::LineTo;
        pts[ptsCnt++] = {x, y};
    }

    void cubicTo(float cx1, float cy1, float cx2, float cy2, float x, float y)
    {
        if (cmdCnt + 1 > reservedCmdCnt) reserveCmd((cmdCnt + 1) * 2);
        if (ptsCnt + 3 > reservedPtsCnt) reservePts((ptsCnt + 3) * 2);

        cmds[cmdCnt++] = PathCommand::CubicTo;
        pts[ptsCnt++] = {cx1, cy1};
        pts[ptsCnt++] = {cx2, cy2};
        pts[ptsCnt++] = {x, y};
    }

    void close()
    {
        if (cmdCnt > 0 && cmds[cmdCnt - 1] == PathCommand::Close) return;

        if (cmdCnt + 1 > reservedCmdCnt) reserveCmd((cmdCnt + 1) * 2);
        cmds[cmdCnt++] = PathCommand::Close;
    }

    bool bounds(float* x, float* y, float* w, float* h) const
    {
        if (ptsCnt == 0) return false;

        Point min = { pts[0].x, pts[0].y };
        Point max = { pts[0].x, pts[0].y };

        for (uint32_t i = 1; i < ptsCnt; ++i) {
            if (pts[i].x < min.x) min.x = pts[i].x;
            if (pts[i].y < min.y) min.y = pts[i].y;
            if (pts[i].x > max.x) max.x = pts[i].x;
            if (pts[i].y > max.y) max.y = pts[i].y;
        }

        if (x) *x = min.x;
        if (y) *y = min.y;
        if (w) *w = max.x - min.x;
        if (h) *h = max.y - min.y;

        return true;
    }
};


struct Shape::Impl
{
    ShapePath path;
    Fill *fill = nullptr;
    ShapeStroke *stroke = nullptr;
    uint8_t color[4] = {0, 0, 0, 0};    //r, g, b, a
    FillRule rule = FillRule::Winding;
    RenderData rdata = nullptr;         //engine data
    Shape *shape = nullptr;
    uint32_t flag = RenderUpdateFlag::None;

    Impl(Shape* s) : shape(s)
    {
    }

    ~Impl()
    {
        if (fill) delete(fill);
        if (stroke) delete(stroke);
    }

    bool dispose(RenderMethod& renderer)
    {
        auto ret = renderer.dispose(rdata);
        rdata = nullptr;
        return ret;
    }

    bool render(RenderMethod& renderer)
    {
        return renderer.renderShape(rdata);
    }

    void* update(RenderMethod& renderer, const RenderTransform* transform, uint32_t opacity, Array<RenderData>& clips, RenderUpdateFlag pFlag)
    {
        this->rdata = renderer.prepare(*shape, this->rdata, transform, opacity, clips, static_cast<RenderUpdateFlag>(pFlag | flag));
        flag = RenderUpdateFlag::None;
        return this->rdata;
    }

    RenderRegion bounds(RenderMethod& renderer)
    {
        return renderer.region(rdata);
    }

    bool bounds(float* x, float* y, float* w, float* h)
    {
        auto ret = path.bounds(x, y, w, h);

        //Stroke feathering
        if (stroke) {
            if (x) *x -= stroke->width * 0.5f;
            if (y) *y -= stroke->width * 0.5f;
            if (w) *w += stroke->width;
            if (h) *h += stroke->width;
        }
        return ret;
    }

    bool strokeWidth(float width)
    {
        //TODO: Size Exception?

        if (!stroke) stroke = new ShapeStroke();
        if (!stroke) return false;

        stroke->width = width;
        flag |= RenderUpdateFlag::Stroke;

        return true;
    }

    bool strokeCap(StrokeCap cap)
    {
        if (!stroke) stroke = new ShapeStroke();
        if (!stroke) return false;

        stroke->cap = cap;
        flag |= RenderUpdateFlag::Stroke;

        return true;
    }

    bool strokeJoin(StrokeJoin join)
    {
        if (!stroke) stroke = new ShapeStroke();
        if (!stroke) return false;

        stroke->join = join;
        flag |= RenderUpdateFlag::Stroke;

        return true;
    }

    bool strokeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        if (!stroke) stroke = new ShapeStroke();
        if (!stroke) return false;

        if (stroke->fill) {
            delete(stroke->fill);
            stroke->fill = nullptr;
            flag |= RenderUpdateFlag::GradientStroke;
        }

        stroke->color[0] = r;
        stroke->color[1] = g;
        stroke->color[2] = b;
        stroke->color[3] = a;

        flag |= RenderUpdateFlag::Stroke;

        return true;
    }

    Result strokeFill(unique_ptr<Fill> f)
    {
        auto p = f.release();
        if (!p) return Result::MemoryCorruption;

        if (!stroke) stroke = new ShapeStroke();
        if (!stroke) return Result::FailedAllocation;

        if (stroke->fill && stroke->fill != p) delete(stroke->fill);
        stroke->fill = p;

        flag |= RenderUpdateFlag::Stroke;
        flag |= RenderUpdateFlag::GradientStroke;

        return Result::Success;
    }

    bool strokeDash(const float* pattern, uint32_t cnt)
    {
       if (!stroke) stroke = new ShapeStroke();
       if (!stroke) return false;

        if (stroke->dashCnt != cnt) {
            if (stroke->dashPattern) free(stroke->dashPattern);
            stroke->dashPattern = nullptr;
        }

        if (!stroke->dashPattern) {
            stroke->dashPattern = static_cast<float*>(malloc(sizeof(float) * cnt));
            if (!stroke->dashPattern) return false;
        }

        for (uint32_t i = 0; i < cnt; ++i)
            stroke->dashPattern[i] = pattern[i];

        stroke->dashCnt = cnt;
        flag |= RenderUpdateFlag::Stroke;

        return true;
    }

    Paint* duplicate()
    {
        auto ret = Shape::gen();
        if (!ret) return nullptr;

        auto dup = ret.get()->pImpl;
        dup->rule = rule;

        //Color
        memcpy(dup->color, color, sizeof(color));
        dup->flag = RenderUpdateFlag::Color;

        //Path
        dup->path.duplicate(&path);
        dup->flag |= RenderUpdateFlag::Path;

        //Stroke
        if (stroke) {
            dup->stroke = new ShapeStroke(stroke);
            dup->flag |= RenderUpdateFlag::Stroke;

            if (stroke->fill)
                dup->flag |= RenderUpdateFlag::GradientStroke;
        }

        //Fill
        if (fill) {
            dup->fill = fill->duplicate();
            dup->flag |= RenderUpdateFlag::Gradient;
        }

        return ret.release();
    }

    /*
     * Load paint and derived classes from .tvg binary file
     * Returns LoaderResult:: Success on success and moves pointer to next position,
     * LoaderResult::SizeCorruption if corrupted or LoaderResult::InvalidType if not applicable for paint.
     * Details:
     * TODO
     */
    LoaderResult tvgLoadPath(const char* pointer, const char* end)
    {
         // ShapePath
         const uint32_t cmdCnt = (uint32_t) *pointer;
         pointer += sizeof(uint32_t);
         const uint32_t ptsCnt = (uint32_t) *pointer;
         pointer += sizeof(uint32_t);
         const PathCommand * cmds = (PathCommand *) pointer;
         pointer += sizeof(PathCommand) * cmdCnt;
         const Point * pts = (Point *) pointer;
         pointer += sizeof(Point) * ptsCnt;

         if (pointer > end) return LoaderResult::SizeCorruption;

         path.cmdCnt = cmdCnt;
         path.ptsCnt = ptsCnt;
         path.reserveCmd(cmdCnt);
         path.reservePts(ptsCnt);
         memcpy(path.cmds, cmds, sizeof(PathCommand) * cmdCnt);
         memcpy(path.pts, pts, sizeof(Point) * ptsCnt);

         flag |= RenderUpdateFlag::Path;
         return LoaderResult::Success;
    }

    /*
     * Load fill for shape or shape stroke from .tvg binary file
     * Returns LoaderResult:: Success on success and moves pointer to next position,
     * LoaderResult::SizeCorruption if corrupted or LoaderResult::InvalidType if not applicable for paint.
     * Details:
     * TODO
     */
    LoaderResult tvgLoadFill(const char* pointer, const char* end, Fill ** fillOutside)
    {
       unique_ptr<Fill> fillGrad;

       while (pointer < end)
          {
             tvg_block block = read_tvg_block(pointer);
             if (block.block_end > end) return LoaderResult::SizeCorruption;

             switch (block.type)
                {
                   case TVG_GRADIENT_FLAG_TYPE_RADIAL: { // radial gradient
                      if (block.lenght != 12) return LoaderResult::SizeCorruption;
                      float x, y, radius;
                      _read_tvg_float(&x, block.data);
                      _read_tvg_float(&y, block.data + 4);
                      _read_tvg_float(&radius, block.data + 8);

                      auto fillGradRadial = RadialGradient::gen();
                      fillGradRadial->radial(x, y, radius);
                      fillGrad = move(fillGradRadial);
                      break;
                   }
                   case TVG_GRADIENT_FLAG_TYPE_LINEAR: { // linear gradient
                      if (block.lenght != 16) return LoaderResult::SizeCorruption;
                      float x1, y1, x2, y2;
                      _read_tvg_float(&x1, block.data);
                      _read_tvg_float(&y1, block.data + 4);
                      _read_tvg_float(&x2, block.data + 8);
                      _read_tvg_float(&y2, block.data + 12);

                      auto fillGradLinear = LinearGradient::gen();
                      fillGradLinear->linear(x1, y1, x2, y2);
                      fillGrad = move(fillGradLinear);
                      break;
                   }
                   case TVG_FILL_FLAG_FILLSPREAD: { // fill spread
                      if (!fillGrad) return LoaderResult::LogicalCorruption;
                      if (block.lenght != 1) return LoaderResult::SizeCorruption;
                      switch (*block.data) {
                         case TVG_FILL_FLAG_FILLSPREAD_PAD:
                            fillGrad->spread(FillSpread::Pad);
                            break;
                         case TVG_FILL_FLAG_FILLSPREAD_REFLECT:
                            fillGrad->spread(FillSpread::Reflect);
                            break;
                         case TVG_FILL_FLAG_FILLSPREAD_REPEAT:
                            fillGrad->spread(FillSpread::Repeat);
                            break;
                      }
                      break;
                   }
                   case TVG_FILL_FLAG_COLORSTOPS: { // color stops
                      if (!fillGrad) return LoaderResult::LogicalCorruption;
                      if (block.lenght == 0 || block.lenght & 0x07) return LoaderResult::SizeCorruption;
                      uint32_t stopsCnt = block.lenght >> 3; // 8 bytes per ColorStop
                      if (stopsCnt > 1023) return LoaderResult::SizeCorruption;
                      Fill::ColorStop stops [stopsCnt];
                      const char* p = block.data;
                      for (uint32_t i = 0; i < stopsCnt; i++, p += 8) {
                            _read_tvg_float(&stops[i].offset, p);
                            stops[i].r = p[4];
                            stops[i].g = p[5];
                            stops[i].b = p[6];
                            stops[i].a = p[7];
                      }
                      fillGrad->colorStops(stops, stopsCnt);
                      break;
                   }
                }

             pointer = block.block_end;
          }

       *fillOutside = fillGrad.release();
       return LoaderResult::Success;
    }

    LoaderResult tvgLoadStrokeDashptrn(const char* pointer, const char* end)
    {
       const uint32_t dashPatternCnt = (uint32_t) *pointer;
       pointer += sizeof(uint32_t);
       const float * dashPattern = (float *) pointer;
       pointer += sizeof(float) * dashPatternCnt;

       if (pointer > end) return LoaderResult::SizeCorruption;

       if (stroke->dashPattern) free(stroke->dashPattern);
       stroke->dashPattern = static_cast<float*>(malloc(sizeof(float) * dashPatternCnt));
       if (!stroke->dashPattern) return LoaderResult::MemoryCorruption;

       stroke->dashCnt = dashPatternCnt;
       memcpy(stroke->dashPattern, dashPattern, sizeof(float) * dashPatternCnt);

       return LoaderResult::Success;
    }

    /*
     * Load stroke from .tvg binary file
     * Returns LoaderResult:: Success on success and moves pointer to next position,
     * LoaderResult::SizeCorruption if corrupted or LoaderResult::InvalidType if not applicable for paint.
     * Details:
     * TODO
     */
    LoaderResult tvgLoadStroke(const char* pointer, const char* end)
    {
       if (!stroke) stroke = new ShapeStroke();
       if (!stroke) return LoaderResult::MemoryCorruption;

       while (pointer < end)
          {
             tvg_block block = read_tvg_block(pointer);
             if (block.block_end > end) return LoaderResult::SizeCorruption;

             switch (block.type)
                {
                   case TVG_SHAPE_STROKE_FLAG_CAP: { // stroke cap
                      if (block.lenght != 1) return LoaderResult::SizeCorruption;
                      switch (*block.data) {
                         case TVG_SHAPE_STROKE_FLAG_CAP_SQUARE:
                            stroke->cap = StrokeCap::Square;
                            break;
                         case TVG_SHAPE_STROKE_FLAG_CAP_ROUND:
                            stroke->cap = StrokeCap::Round;
                            break;
                         case TVG_SHAPE_STROKE_FLAG_CAP_BUTT:
                            stroke->cap = StrokeCap::Butt;
                            break;
                      }
                      break;
                   }
                   case TVG_SHAPE_STROKE_FLAG_JOIN: { // stroke join
                      if (block.lenght != 1) return LoaderResult::SizeCorruption;
                      switch (*block.data) {
                         case TVG_SHAPE_STROKE_FLAG_JOIN_BEVEL:
                            stroke->join = StrokeJoin::Bevel;
                            break;
                         case TVG_SHAPE_STROKE_FLAG_JOIN_ROUND:
                            stroke->join = StrokeJoin::Round;
                            break;
                         case TVG_SHAPE_STROKE_FLAG_JOIN_MITER:
                            stroke->join = StrokeJoin::Miter;
                            break;
                      }
                      break;
                   }
                   case TVG_SHAPE_STROKE_FLAG_WIDTH: { // stroke width
                      if (block.lenght != sizeof(float)) return LoaderResult::SizeCorruption;
                      _read_tvg_float(&stroke->width, block.data);
                      break;
                   }
                   case TVG_SHAPE_STROKE_FLAG_COLOR: { // stroke color
                      if (block.lenght != sizeof(stroke->color)) return LoaderResult::SizeCorruption;
                      memcpy(&stroke->color, block.data, sizeof(stroke->color));
                      break;
                   }
                   case TVG_SHAPE_STROKE_FLAG_HAS_FILL: { // stroke fill
                      LoaderResult result = tvgLoadFill(block.data, block.block_end, &stroke->fill);
                      if (result != LoaderResult::Success) return result;
                      flag |= RenderUpdateFlag::GradientStroke;
                      break;
                   }
                   case TVG_SHAPE_STROKE_FLAG_HAS_DASHPTRN: { // dashed stroke
                      LoaderResult result = tvgLoadStrokeDashptrn(block.data, block.block_end);
                      if (result != LoaderResult::Success) return result;
                      break;
                   }
                }

             pointer = block.block_end;
          }

       return LoaderResult::Success;
    }

    /*
     * Load paint and derived classes from .tvg binary file
     * Returns LoaderResult:: Success on success and moves pointer to next position,
     * LoaderResult::SizeCorruption if corrupted or LoaderResult::InvalidType if not applicable for paint.
     * Details:
     * TODO
     */
    LoaderResult tvgLoad(tvg_block block)
    {
       switch (block.type)
          {
             case TVG_SHAPE_FLAG_HAS_PATH: { // path
                LoaderResult result = tvgLoadPath(block.data, block.block_end);
                if (result != LoaderResult::Success) return result;
                break;
             }
             case TVG_SHAPE_FLAG_HAS_STROKE: { // stroke section
                LoaderResult result = tvgLoadStroke(block.data, block.block_end);
                printf("TVG_SHAPE_FLAG_HAS_STROKE result %s \n", (result != LoaderResult::Success) ? "ERROR" : "OK");
                if (result != LoaderResult::Success) return result;
                flag |= RenderUpdateFlag::Stroke;
                break;
             }
             case TVG_SHAPE_FLAG_HAS_FILL: { // fill (gradient)
                LoaderResult result = tvgLoadFill(block.data, block.block_end, &fill);
                printf("TVG_SHAPE_FLAG_HAS_FILL result %s \n", (result != LoaderResult::Success) ? "ERROR" : "OK");
                if (result != LoaderResult::Success) return result;
                flag |= RenderUpdateFlag::Gradient;
                break;
             }
             case TVG_SHAPE_FLAG_COLOR: { // color
                if (block.lenght != sizeof(color)) return LoaderResult::SizeCorruption;
                memcpy(&color, block.data, sizeof(color));
                flag = RenderUpdateFlag::Color;
                break;
             }
             case TVG_SHAPE_FLAG_FILLRULE: { // fill rule
                if (block.lenght != sizeof(uint8_t)) return LoaderResult::SizeCorruption;
                switch (*block.data)
                {
                   case TVG_SHAPE_FLAG_FILLRULE_WINDING:
                      rule = FillRule::Winding;
                      break;
                   case TVG_SHAPE_FLAG_FILLRULE_EVENODD:
                      rule = FillRule::EvenOdd;
                      break;
                }
                break;
             }
             default: {
                return LoaderResult::InvalidType;
             }
          }

       return LoaderResult::Success;
    }

    ByteCounter serializeFill(TvgSaver* tvgSaver, Fill* f, TvgIndicator fillTvgFlag)
    {
        if (!tvgSaver) return 0;

        ByteCounter fillDataByteCnt = 0;
        TvgFlag strokeTvgFlag;
        const Fill::ColorStop* stops = nullptr;
        auto stopsCnt = f->colorStops(&stops);
        if (!stops || stopsCnt == 0) return 0;

        tvgSaver->saveMemberIndicator(fillTvgFlag);
        tvgSaver->skipMemberDataSize();

        if (f->id() == FILL_ID_RADIAL) {
            float argRadial[3];
            auto radGrad = static_cast<RadialGradient*>(f);
            if (radGrad->radial(argRadial, argRadial + 1,argRadial + 2) != Result::Success) {
                tvgSaver->rewindBuffer(TVG_FLAG_SIZE + BYTE_COUNTER_SIZE);
                return 0;
            }
            fillDataByteCnt += tvgSaver->saveMember(TVG_GRADIENT_FLAG_TYPE_RADIAL, sizeof(argRadial), argRadial);
        }
        else {
            float argLinear[4];
            auto linGrad = static_cast<LinearGradient*>(f);
            if (linGrad->linear(argLinear, argLinear + 1, argLinear + 2, argLinear + 3) != Result::Success) {
                tvgSaver->rewindBuffer(TVG_FLAG_SIZE + BYTE_COUNTER_SIZE);
                return 0;
            }
            fillDataByteCnt += tvgSaver->saveMember(TVG_GRADIENT_FLAG_TYPE_LINEAR, sizeof(argLinear), argLinear);
        }

        switch (f->spread()) {
            case FillSpread::Pad: {
                strokeTvgFlag = TVG_FILL_FLAG_FILLSPREAD_PAD;
                break;
            }
            case FillSpread::Reflect: {
                strokeTvgFlag = TVG_FILL_FLAG_FILLSPREAD_REFLECT;
                break;
            }
            case FillSpread::Repeat: {
                strokeTvgFlag = TVG_FILL_FLAG_FILLSPREAD_REPEAT;
                break;
            }
        }
        fillDataByteCnt += tvgSaver->saveMember(TVG_FILL_FLAG_FILLSPREAD, TVG_FLAG_SIZE, &strokeTvgFlag);

        fillDataByteCnt += tvgSaver->saveMember(TVG_FILL_FLAG_COLORSTOPS, stopsCnt * sizeof(stops), stops);

        tvgSaver->saveMemberDataSizeAt(fillDataByteCnt);

        return TVG_INDICATOR_SIZE + BYTE_COUNTER_SIZE + fillDataByteCnt;
    }

    ByteCounter serializeStroke(TvgSaver* tvgSaver)
    {
        if (!tvgSaver) return 0;

        ByteCounter strokeDataByteCnt = 0;
        TvgFlag strokeTvgFlag;

        tvgSaver->saveMemberIndicator(TVG_SHAPE_FLAG_HAS_STROKE);
        tvgSaver->skipMemberDataSize();

        switch (stroke->cap) {
            case StrokeCap::Square: {
                strokeTvgFlag = TVG_SHAPE_STROKE_FLAG_CAP_SQUARE;
                break;
            }
            case StrokeCap::Round: {
                strokeTvgFlag = TVG_SHAPE_STROKE_FLAG_CAP_ROUND;
                break;
            }
            case StrokeCap::Butt: {
                strokeTvgFlag = TVG_SHAPE_STROKE_FLAG_CAP_BUTT;
                break;
            }
        }
        strokeDataByteCnt += tvgSaver->saveMember(TVG_SHAPE_STROKE_FLAG_CAP, TVG_FLAG_SIZE, &strokeTvgFlag);

        switch (stroke->join) {
            case StrokeJoin::Bevel: {
                strokeTvgFlag = TVG_SHAPE_STROKE_FLAG_JOIN_BEVEL;
                break;
            }
            case StrokeJoin::Round: {
                strokeTvgFlag = TVG_SHAPE_STROKE_FLAG_JOIN_ROUND;
                break;
            }
            case StrokeJoin::Miter: {
                strokeTvgFlag = TVG_SHAPE_STROKE_FLAG_JOIN_MITER;
                break;
            }
        }
        strokeDataByteCnt += tvgSaver->saveMember(TVG_SHAPE_STROKE_FLAG_JOIN, TVG_FLAG_SIZE, &strokeTvgFlag);

        strokeDataByteCnt += tvgSaver->saveMember(TVG_SHAPE_STROKE_FLAG_WIDTH, sizeof(stroke->width), &stroke->width);

        if (stroke->fill) {
            strokeDataByteCnt += serializeFill(tvgSaver, stroke->fill, TVG_SHAPE_STROKE_FLAG_HAS_FILL);
        }

        strokeDataByteCnt += tvgSaver->saveMember(TVG_SHAPE_STROKE_FLAG_COLOR, sizeof(stroke->color), &stroke->color);

        if (stroke->dashPattern && stroke->dashCnt > 0) {
            ByteCounter dashCntByteCnt = sizeof(stroke->dashCnt);
            ByteCounter dashPtrnByteCnt = stroke->dashCnt * sizeof(stroke->dashPattern[0]);

            tvgSaver->saveMemberIndicator(TVG_SHAPE_STROKE_FLAG_HAS_DASHPTRN);
            tvgSaver->saveMemberDataSize(dashCntByteCnt + dashPtrnByteCnt);
            strokeDataByteCnt += tvgSaver->saveMemberData(&stroke->dashCnt, dashCntByteCnt);
            strokeDataByteCnt += tvgSaver->saveMemberData(stroke->dashPattern, dashPtrnByteCnt);
            strokeDataByteCnt += TVG_INDICATOR_SIZE + BYTE_COUNTER_SIZE;
        }

        tvgSaver->saveMemberDataSizeAt(strokeDataByteCnt);

        return TVG_INDICATOR_SIZE + BYTE_COUNTER_SIZE + strokeDataByteCnt;
    }

    ByteCounter serializePath(TvgSaver* tvgSaver)
    {
        if (!tvgSaver) return 0;
        if (!path.cmds || !path.pts || !path.cmdCnt || !path.ptsCnt) return 0;  // MGS - double check - do we need this ?

        ByteCounter pathDataByteCnt = 0;

        tvgSaver->saveMemberIndicator(TVG_SHAPE_FLAG_HAS_PATH);
        tvgSaver->skipMemberDataSize();

        pathDataByteCnt += tvgSaver->saveMemberData(&path.cmdCnt, sizeof(path.cmdCnt));
        pathDataByteCnt += tvgSaver->saveMemberData(&path.ptsCnt, sizeof(path.ptsCnt));
        pathDataByteCnt += tvgSaver->saveMemberData(path.cmds, path.cmdCnt * sizeof(path.cmds[0]));
        pathDataByteCnt += tvgSaver->saveMemberData(path.pts, path.ptsCnt * sizeof(path.pts[0]));

        tvgSaver->saveMemberDataSizeAt(pathDataByteCnt);

        return TVG_INDICATOR_SIZE + BYTE_COUNTER_SIZE + pathDataByteCnt;
    }

    ByteCounter serialize(TvgSaver* tvgSaver)
    {
        if (!tvgSaver) return 0;

        ByteCounter shapeDataByteCnt = 0;

        tvgSaver->saveMemberIndicator(TVG_SHAPE_BEGIN_INDICATOR);
        tvgSaver->skipMemberDataSize();

        TvgFlag ruleTvgFlag = (rule == FillRule::EvenOdd) ? TVG_SHAPE_FLAG_FILLRULE_EVENODD : TVG_SHAPE_FLAG_FILLRULE_WINDING;
        shapeDataByteCnt += tvgSaver->saveMember(TVG_SHAPE_FLAG_FILLRULE, TVG_FLAG_SIZE, &ruleTvgFlag);

        if (stroke) {
            shapeDataByteCnt += serializeStroke(tvgSaver);
        }

        if (fill) {
            shapeDataByteCnt += serializeFill(tvgSaver, fill, TVG_SHAPE_FLAG_HAS_FILL);
        }

        shapeDataByteCnt += tvgSaver->saveMember(TVG_SHAPE_FLAG_COLOR, sizeof(color), color);

        if (path.cmds && path.pts) {
            shapeDataByteCnt += serializePath(tvgSaver);
        }

        shapeDataByteCnt += shape->Paint::pImpl->serializePaint(tvgSaver);

        tvgSaver->saveMemberDataSizeAt(shapeDataByteCnt);

        return TVG_INDICATOR_SIZE + BYTE_COUNTER_SIZE + shapeDataByteCnt;
    }
};

#endif //_TVG_SHAPE_IMPL_H_
