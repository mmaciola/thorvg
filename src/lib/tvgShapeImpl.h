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
#ifndef _TVG_SHAPE_IMPL_H_
#define _TVG_SHAPE_IMPL_H_

#include <memory.h>
#include "tvgPaint.h"
#include "tvgTvgHelper.h"

/************************************************************************/
/* Internal Class Implementation                                        */
/************************************************************************/

struct ShapeStroke
{
    float width = 0;
    uint8_t color[4] = {0, 0, 0, 0};
    float* dashPattern = nullptr;
    uint32_t dashCnt = 0;
    StrokeCap cap = StrokeCap::Square;
    StrokeJoin join = StrokeJoin::Bevel;

    ShapeStroke() {}

    ShapeStroke(const ShapeStroke* src)
    {
        width = src->width;
        dashCnt = src->dashCnt;
        cap = src->cap;
        join = src->join;
        memcpy(color, src->color, sizeof(color));
        dashPattern = static_cast<float*>(malloc(sizeof(float) * dashCnt));
        memcpy(dashPattern, src->dashPattern, sizeof(float) * dashCnt);
    }

    ~ShapeStroke()
    {
        if (dashPattern) free(dashPattern);
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

    bool bounds(float* x, float* y, float* w, float* h)
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

    bool tvgStore(char ** buffer, uint32_t * size)
    {
       *size = 8 + sizeof(PathCommand) * cmdCnt + sizeof(Point) * ptsCnt;
       *buffer = (char *) malloc(*size);
       if (!*buffer) return false;

       char * pointer = *buffer;
       memcpy(pointer, &cmdCnt, sizeof(uint32_t));
       pointer += sizeof(uint32_t);
       memcpy(pointer, &ptsCnt, sizeof(uint32_t));
       pointer += sizeof(uint32_t);
       memcpy(pointer, cmds, sizeof(PathCommand) * cmdCnt);
       pointer += sizeof(PathCommand) * cmdCnt;
       memcpy(pointer, pts, sizeof(Point) * ptsCnt);
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

    bool bounds(RenderMethod& renderer, uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h)
    {
        return renderer.region(rdata, x, y, w, h);
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

        stroke->color[0] = r;
        stroke->color[1] = g;
        stroke->color[2] = b;
        stroke->color[3] = a;

        flag |= RenderUpdateFlag::Stroke;

        return true;
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

    void reset()
    {
        path.reset();

        if (fill) {
            delete(fill);
            fill = nullptr;
        }
        if (stroke) {
            delete(stroke);
            stroke = nullptr;
        }

        color[0] = color[1] = color[2] = color[3] = 0;

        flag = RenderUpdateFlag::All;
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
        }

        //Fill
        if (fill) {
            dup->fill = fill->duplicate();
            dup->flag |= RenderUpdateFlag::Gradient;
        }

        return ret.release();
    }

    /*
     * Load shape stroke from .tvg binary file
     */
    bool tvgLoadStroke(const tvg_shape_stroke * shape_stroke)
    {
       if (!strokeDash(&shape_stroke->dashPattern, shape_stroke->dashPatternCnt))
          {
             return false;
          }

       stroke->width = shape_stroke->width;
       memcpy(stroke->color, shape_stroke->color, sizeof(shape_stroke->color));

       switch (shape_stroke->flags & TVG_STROKE_FLAG_MASK_CAP) {
          case TVG_STROKE_FLAG_CAP_SQUARE:
             stroke->cap = StrokeCap::Square;
             break;
          case TVG_STROKE_FLAG_CAP_ROUND:
             stroke->cap = StrokeCap::Round;
             break;
          case TVG_STROKE_FLAG_CAP_BUTT:
             stroke->cap = StrokeCap::Butt;
             break;
          default:
             return false;
       }

       switch (shape_stroke->flags & TVG_STROKE_FLAG_MASK_JOIN) {
          case TVG_STROKE_FLAG_JOIN_BEVEL:
             stroke->join = StrokeJoin::Bevel;
             break;
          case TVG_STROKE_FLAG_JOIN_ROUND:
             stroke->join = StrokeJoin::Round;
             break;
          case TVG_STROKE_FLAG_JOIN_MITER:
             stroke->join = StrokeJoin::Miter;
             break;
          default:
             return false;
       }

       return true;
    }

    /*
     * Load shape from .tvg binary file
     * Returns true on success and moves pointer to next position or false if corrupted.
     * Details:
     * Flags:
     * xxxxxxx0 - FillRule.Winding
     * xxxxxxx1 - FillRule.EvenOdd
     * [uint8 flags][color][path][stroke][fill]
     */
    bool tvgLoad(const char** pointer)
    {
       // flags
       const uint8_t flags = (uint8_t) **pointer;
       *pointer += sizeof(uint8_t);

       rule = (flags & TVG_SHAPE_FLAG_MASK_FILLRULE) ? FillRule::EvenOdd : FillRule::Winding;

       // colors
       const uint8_t * colors = (uint8_t*) *pointer;
       *pointer += sizeof(uint8_t) * 4;

       memcpy(&color, colors, sizeof(color));
       flag = RenderUpdateFlag::Color;

       // ShapePath
       const uint32_t cmdCnt = (uint32_t) **pointer;
       *pointer += sizeof(uint32_t);
       const uint32_t ptsCnt = (uint32_t) **pointer;
       *pointer += sizeof(uint32_t);
       const PathCommand * cmds = (PathCommand *) *pointer;
       *pointer += sizeof(PathCommand) * cmdCnt;
       const Point * pts = (Point *) *pointer;
       *pointer += sizeof(Point) * ptsCnt;

       path.reserveCmd(cmdCnt);
       path.reservePts(ptsCnt);
       memcpy(path.cmds, cmds, sizeof(PathCommand) * cmdCnt);
       memcpy(path.pts, pts, sizeof(Point) * ptsCnt);
       flag |= RenderUpdateFlag::Path;

       // ShapeStroke
       if (flags & TVG_SHAPE_FLAG_HAS_STROKE)
          {
             const tvg_shape_stroke * shape_stroke = (tvg_shape_stroke *) *pointer;
             *pointer += sizeof(tvg_shape_stroke) + sizeof(float) * (shape_stroke->dashPatternCnt - 1);

             tvgLoadStroke(shape_stroke); // flag is set inside strokeDash
          }

       // Fill
       if (flags & TVG_SHAPE_FLAG_HAS_FILL)
          {
             // TODO [mmaciola] how store gradients- by id or duplication
             flag |= RenderUpdateFlag::Gradient;
          }

       return true;
    }

    /*
     * Store stroke into .tvg binary file
     */
    void tvgStoreStroke(tvg_shape_stroke * shape_stroke)
    {
       // TODO [mmaciola] jak przechowywac dashPattern
       if (!shape_stroke)
          {
             shape_stroke = (tvg_shape_stroke *) malloc(sizeof(tvg_shape_stroke) + sizeof(float) * (stroke->dashCnt - 1));
             if (!shape_stroke)
               {
                   return;
               }
          }

       shape_stroke->width = stroke->width;
       memcpy(shape_stroke->color, stroke->color, sizeof(stroke->color));

       shape_stroke->dashPatternCnt = stroke->dashCnt;
       memcpy(&shape_stroke->dashPattern, stroke->dashPattern, sizeof(float) * stroke->dashCnt);

       shape_stroke->flags = 0;
       switch (stroke->cap)
         {
         case StrokeCap::Square:
            shape_stroke->flags |= TVG_STROKE_FLAG_CAP_SQUARE;
            break;
         case StrokeCap::Round:
            shape_stroke->flags |= TVG_STROKE_FLAG_CAP_ROUND;
            break;
         case StrokeCap::Butt:
            shape_stroke->flags |= TVG_STROKE_FLAG_CAP_BUTT;
            break;
         }
       switch (stroke->join)
         {
         case StrokeJoin::Bevel:
            shape_stroke->flags |= TVG_STROKE_FLAG_JOIN_BEVEL;
            break;
         case StrokeJoin::Round:
            shape_stroke->flags |= TVG_STROKE_FLAG_JOIN_ROUND;
            break;
         case StrokeJoin::Miter:
            shape_stroke->flags |= TVG_STROKE_FLAG_JOIN_MITER;
            break;
         }
    }

    /*
     * Store shape from .tvg binary file
     * Returns true on success and moves pointer to next position or false if corrupted.
     * Details: see above function tvgLoad
     * [uint8 flags][color][path][stroke][fill]
     */
    bool tvgStore(char * unused)
    {
       // Function below is fuct for testing, will be changed completely
       char * buffer = (char *) malloc(2048);
       char * pointer = buffer;

       // flags
       *pointer = (rule == FillRule::EvenOdd) ? TVG_SHAPE_FLAG_MASK_FILLRULE : 0;
       if (stroke) *pointer |= TVG_SHAPE_FLAG_HAS_STROKE;
       if (fill) *pointer |= TVG_SHAPE_FLAG_HAS_FILL;
       pointer += 1;

       // colors
       memcpy(pointer, &color, sizeof(color));
       pointer += sizeof(color);

       // ShapePath
       char * pthBuffer;
       uint32_t pthSize;
       path.tvgStore(&pthBuffer, &pthSize);
       memcpy(pointer, pthBuffer, pthSize);
       pointer += pthSize;
       free(pthBuffer);

       // ShapeStroke
       if (stroke)
          {
             tvg_shape_stroke * shape_stroke = (tvg_shape_stroke *) pointer;
             *pointer += sizeof(tvg_shape_stroke) + sizeof(float) * (shape_stroke->dashPatternCnt - 1);

             tvgStoreStroke(shape_stroke);
          }

       // Fill
       if (fill)
          {
             // TODO [mmaciola] how store gradients- by id or duplication
             //flag |= RenderUpdateFlag::Gradient;
          }

       // For testing print
       printf("tvgStore:");
       for (char * ptr = buffer; ptr < pointer; ptr++) {
             printf(" %02X", (uint8_t)(*ptr));
       }
       printf(".\n");

       return true;
    }
};

#endif //_TVG_SHAPE_IMPL_H_
