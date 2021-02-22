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

    ByteCounter serializeFill(char** pointer, Fill* f)
    {
        if (!*pointer) return 0;

        const Fill::ColorStop* stops = nullptr;
        auto stopsCnt = f->colorStops(&stops);

        if (!stops || stopsCnt == 0) return 0;

        char* start = *pointer;
        FlagType flag;
        ByteCounter byteCnt = 0;
        size_t byteCntSize = sizeof(ByteCounter);

        // fillspread flag
        switch (f->spread()) {
            case FillSpread::Pad: {
                flag = TVG_FILL_FLAG_FILLSPREAD_PAD;
                break;
            }
            case FillSpread::Reflect: {
                flag = TVG_FILL_FLAG_FILLSPREAD_REFLECT;
                break;
            }
            case FillSpread::Repeat: {
                flag = TVG_FILL_FLAG_FILLSPREAD_REPEAT;
                break;
            }
        }
        memcpy(*pointer, &flag, sizeof(FlagType));
        *pointer += sizeof(FlagType);
        // number of bytes associated with fillspread
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;

        // colorStops flag
        flag = TVG_FILL_FLAG_COLORSTOPS;
        memcpy(*pointer, &flag, sizeof(FlagType));
        *pointer += sizeof(FlagType);
        // number of bytes associated with colorStops
        byteCnt = sizeof(stopsCnt) + stopsCnt * (sizeof(stops->offset) + 4 * sizeof(stops->r));  
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;
        // bytes associated with colorStops: [stopsCnt][stops]
        memcpy(*pointer, &stopsCnt, sizeof(stopsCnt));
        *pointer += sizeof(stopsCnt);

        for (uint32_t i = 0; i < stopsCnt; ++i) {  //MGS
            memcpy(*pointer, &stops[i].offset, sizeof(stops->offset));
            *pointer += sizeof(stops->offset);

            memcpy(*pointer, &stops[i].r, sizeof(stops->r));
            *pointer += sizeof(stops->r);
            memcpy(*pointer, &stops[i].g, sizeof(stops->g));
            *pointer += sizeof(stops->g);
            memcpy(*pointer, &stops[i].b, sizeof(stops->b));
            *pointer += sizeof(stops->b);
            memcpy(*pointer, &stops[i].a, sizeof(stops->a));
            *pointer += sizeof(stops->a);
        }

        // radial gradient
        if (f->id() == FILL_ID_RADIAL) {
            float argRadial[3];
            auto radGrad = static_cast<RadialGradient*>(f);
            if (radGrad->radial(argRadial, argRadial + 1,argRadial + 2) != Result::Success) {
                // MGS rewind pointer
                return 0;
            }
            // radial gradient flag
            flag = TVG_GRADIENT_FLAG_TYPE_RADIAL;
            memcpy(*pointer, &flag, sizeof(FlagType));
            *pointer += sizeof(FlagType);
            // number of bytes associated with radial gradinet
            byteCnt = sizeof(argRadial);
            memcpy(*pointer, &byteCnt, byteCntSize);
            *pointer += byteCntSize;
            // bytes associated with radial gradient: [cx][cy][radius]
            memcpy(*pointer, argRadial, byteCnt);
            *pointer += byteCnt;
        }
        // linear gradient
        else {
            float argLinear[4];
            auto linGrad = static_cast<LinearGradient*>(f);
            if (linGrad->linear(argLinear, argLinear + 1, argLinear + 2, argLinear + 3) != Result::Success) {
                // MGS rewind pointer
                return 0;
            }
            // linear gradient flag
            flag = TVG_GRADIENT_FLAG_TYPE_LINEAR;
            memcpy(*pointer, &flag, sizeof(FlagType));
            *pointer += sizeof(FlagType);
            // number of bytes associated with linear gradient
            byteCnt = sizeof(argLinear);
            memcpy(*pointer, &byteCnt, byteCntSize);
            *pointer += byteCntSize;
            // bytes associated with linear gradient: [x1][y1][x2][y2]
            memcpy(*pointer, argLinear, byteCnt);
            *pointer += byteCnt;
        }

        return *pointer - start;
    }

    /* assumption: all flags of given type have the same size */
    ByteCounter serializeStroke(char** pointer)
    {
        if (!*pointer) return 0;

        char* start = *pointer;
        FlagType flag;
        ByteCounter byteCnt = 0;
        size_t byteCntSize = sizeof(ByteCounter);

        // cap flag
        switch (stroke->cap) {
            case StrokeCap::Square: {
                flag = TVG_SHAPE_STROKE_FLAG_CAP_SQUARE;
                break;
            }
            case StrokeCap::Round: {
                flag = TVG_SHAPE_STROKE_FLAG_CAP_ROUND;
                break;
            }
            case StrokeCap::Butt: {
                flag = TVG_SHAPE_STROKE_FLAG_CAP_BUTT;
                break;
            }
        }
        memcpy(*pointer, &flag, sizeof(FlagType));
        *pointer += sizeof(FlagType);
        // number of bytes associated with cap
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;

        // join flag
        switch (stroke->join) {
            case StrokeJoin::Bevel: {
                flag = TVG_SHAPE_STROKE_FLAG_JOIN_BEVEL;
                break;
            }
            case StrokeJoin::Round: {
                flag = TVG_SHAPE_STROKE_FLAG_JOIN_ROUND;
                break;
            }
            case StrokeJoin::Miter: {
                flag = TVG_SHAPE_STROKE_FLAG_JOIN_MITER;
                break;
            }
        }
        memcpy(*pointer, &flag, sizeof(FlagType));
        *pointer += sizeof(FlagType);
        // number of bytes associated with join
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;

        // width flag
        flag = TVG_SHAPE_STROKE_FLAG_WIDTH;
        memcpy(*pointer, &flag, sizeof(FlagType));
        *pointer += sizeof(FlagType);
        // number of bytes associated with width
        byteCnt = sizeof(stroke->width);
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;
        // bytes associated with width
        memcpy(*pointer, &stroke->width, byteCnt);
        *pointer += byteCnt;

/*
        if (stroke->fill) {
            // fill flag
            flag = TVG_SHAPE_STROKE_FLAG_HAS_FILL;
            memcpy(*pointer, &flag, sizeof(FlagType));
            *pointer += sizeof(FlagType);
            // number of bytes associated with fill - empty
            *pointer += byteCntSize;
            // bytes associated with fill
            ByteCounter byteCnt = serializeFill(pointer, stroke->fill);
            if (!byteCnt) {
               // MGS log + change size of the buffer
               *pointer -= sizeof(FlagType) + byteCntSize;
               return;
            }
            // number of bytes associated with fill - filled
            memcpy(*pointer - byteCntSize - byteCnt, &byteCnt, byteCntSize);
        }
*/

        // color flag
        flag = TVG_SHAPE_STROKE_FLAG_COLOR;
        memcpy(*pointer, &flag, sizeof(FlagType));
        *pointer += sizeof(FlagType);
        // number of bytes associated with color
        byteCnt = sizeof(stroke->color);
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;
        // bytes associated with color
        memcpy(*pointer, stroke->color, byteCnt);
        *pointer += byteCnt;

        if (stroke->dashPattern && stroke->dashCnt > 0) {
            auto sizeofCnt = sizeof(stroke->dashCnt);
            auto sizeofPattern = sizeof(stroke->dashPattern[0]);
            // dash flag
            flag = TVG_SHAPE_STROKE_FLAG_HAS_DASHPTRN;
            memcpy(*pointer, &flag, sizeof(FlagType));
            *pointer += sizeof(FlagType);
            // number of bytes associated with dash
            byteCnt = sizeofCnt + stroke->dashCnt * sizeofPattern;
            memcpy(*pointer, &byteCnt, byteCntSize);
            *pointer += byteCntSize;
            // bytes associated with dash
            memcpy(*pointer, &stroke->dashCnt, sizeofCnt);
            *pointer += sizeofCnt;
            memcpy(*pointer, stroke->dashPattern, stroke->dashCnt * sizeofPattern);
            *pointer += stroke->dashCnt * sizeofPattern;
        }

        return *pointer - start;
    }


    ByteCounter serializePath(char** pointer)
    {
        if (!*pointer) return 0;
        if (!path.cmds || !path.pts || !path.cmdCnt || !path.ptsCnt) return 0;  // MGS - double check - needed?

        char* start = *pointer;
        FlagType flag;
        ByteCounter byteCnt = 0;
        size_t byteCntSize = sizeof(ByteCounter);
        auto sizeofCmdCnt = sizeof(path.cmdCnt);
        auto sizeofCmds = sizeof(path.cmds[0]);
        auto sizeofPtsCnt = sizeof(path.ptsCnt);
        auto sizeofPts = sizeof(path.pts[0]);

        // path commands flag
        flag = TVG_PATH_FLAG_CMDS;
        memcpy(*pointer, &flag, sizeof(FlagType));
        *pointer += sizeof(FlagType);
        // number of bytes associated with path commands
        byteCnt = sizeofCmdCnt + path.cmdCnt * sizeofCmds;
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;
        // bytes associated with path commands: [cmdCnt][cmds]
        memcpy(*pointer, &path.cmdCnt, sizeofCmdCnt);
        *pointer += sizeofCmdCnt;
        memcpy(*pointer, path.cmds, path.cmdCnt * sizeofCmds);
        *pointer += path.cmdCnt * sizeofCmds;

        // path points flag
        flag = TVG_PATH_FLAG_PTS;
        memcpy(*pointer, &flag, sizeof(FlagType));
        *pointer += sizeof(FlagType);
        // number of bytes associated with path points
        byteCnt = sizeofPtsCnt + path.ptsCnt * sizeofPts;
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;
        // bytes associated with path points: [ptsCnt][pts]
        memcpy(*pointer, &path.ptsCnt, sizeofPtsCnt);
        *pointer += sizeofPtsCnt;
        memcpy(*pointer, path.pts, path.ptsCnt * sizeofPts);
        *pointer += path.ptsCnt * sizeofPts;

        return *pointer - start;
    }


    void serialize(char** pointer)
    {
        if (!*pointer) return;// false;

        char* start = *pointer;
        FlagType flag;
        ByteCounter byteCnt = 0;
        size_t byteCntSize = sizeof(ByteCounter);

        // shape indicator
        flag = TVG_SHAPE_BEGIN_INDICATOR;
        memcpy(*pointer, &flag, sizeof(FlagType));
        *pointer += sizeof(FlagType);
        // number of bytes associated with shape - empty for now
        *pointer += byteCntSize;

        // fillrule flag
        flag = (rule == FillRule::EvenOdd) ? TVG_SHAPE_FLAG_FILLRULE_EVENODD : TVG_SHAPE_FLAG_FILLRULE_WINDING;
        memcpy(*pointer, &flag, sizeof(FlagType));
        *pointer += sizeof(FlagType);
        // number of bytes associated with fillrule (0)
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;

        if (stroke) {
            // stroke flag
            flag = TVG_SHAPE_FLAG_HAS_STROKE;
            memcpy(*pointer, &flag, sizeof(FlagType));
            *pointer += sizeof(FlagType);
            // number of bytes associated with stroke - empty
            *pointer += byteCntSize;
            // bytes associated with stroke
            byteCnt = serializeStroke(pointer);
            if (!byteCnt) {
               // MGS log + change size of the buffer
               *pointer -= sizeof(FlagType) + byteCntSize;
               return;
            }
            // number of bytes associated with stroke - filled
            memcpy(*pointer - byteCntSize - byteCnt, &byteCnt, byteCntSize);
        }

        if (fill) {
            // fill flag
            flag = TVG_SHAPE_FLAG_HAS_FILL;
            memcpy(*pointer, &flag, sizeof(FlagType));
            *pointer += sizeof(FlagType);
            // number of bytes associated with fill - empty
            *pointer += byteCntSize;
            // bytes associated with fill
            ByteCounter byteCnt = serializeFill(pointer, fill);
            if (!byteCnt) {
               // MGS log + change size of the buffer
               *pointer -= sizeof(FlagType) + byteCntSize;
               return;
            }
            // number of bytes associated with fill - filled
            memcpy(*pointer - byteCntSize - byteCnt, &byteCnt, byteCntSize);
        }

        // color flag
        flag = TVG_SHAPE_FLAG_COLOR;
        memcpy(*pointer, &flag, sizeof(FlagType));
        *pointer += sizeof(FlagType);
        // number of bytes associated with color
        byteCnt = sizeof(color);
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;
        // bytes associated with color
        memcpy(*pointer, color, byteCnt);
        *pointer += byteCnt;

        if (path.cmds && path.pts) {
            // path flag
            flag = TVG_SHAPE_FLAG_HAS_PATH;
            memcpy(*pointer, &flag, sizeof(FlagType));
            *pointer += sizeof(FlagType);
            // number of bytes associated with path - empty
            *pointer += byteCntSize;
            // bytes associated with path
            ByteCounter byteCnt = serializePath(pointer);
            if (!byteCnt) {
               // MGS log + change size of the buffer
               *pointer -= sizeof(FlagType) + byteCntSize;
               return;
            }
            // number of bytes associated with path - filled
            memcpy(*pointer - byteCntSize - byteCnt, &byteCnt, byteCntSize);
        }

        // number of bytes associated with shape - filled
        byteCnt = *pointer - start - sizeof(FlagType) - byteCntSize;
        memcpy(*pointer - byteCnt - byteCntSize, &byteCnt, byteCntSize);

        //return true;
    }

    /*
     * Load shape stroke from .tvg binary file
     * Returns true on success and moves pointer to next position or false if corrupted or other problem.
     * Details:
     * Stroke section starts with uint8 flags, these are describes below in Flags section.
     * Next is float stroke width.
     * If has_fill flag is set, next is uint32 fill identificator. If has_fill is clear, 4*uint8 fill color.
     * Note that gradient fill for stroke is future feature, not implemented in tvg yet- TODO.
     * If had_dash_pattern is set next is dash pattern: uint32 cnt, cnt*[float cmds].
     *
     * Flags:
     * xxxxxx00 - Error
     * xxxxxx01 - StrokeCap::Square
     * xxxxxx10 - StrokeCap::Round
     * xxxxxx11 - StrokeCap::Butt
     * xxxx00xx - Error
     * xxxx01xx - StrokeJoin::Bevel
     * xxxx10xx - StrokeJoin::Round
     * xxxx11xx - StrokeJoin::Miter
     * xxxx00xx - Error
     * xxx1xxxx - HAS_FILL - if set- fillid (gradient fill), if clear- color.
     * xx1xxxxx - HAS_DASH_PATTERN
     *
     * [uint8 flags][uint32 width][4*uint8 color OR uint32 fillid][dashPattern]
     */
    bool tvgLoadStroke(const char** pointer)
    {
       /*const tvg_shape_stroke * shape_stroke = (tvg_shape_stroke *) *pointer;

       if (!stroke) stroke = new ShapeStroke();
       if (!stroke) return false;

       // width
       stroke->width = shape_stroke->width;

       // color or fillid
       if (shape_stroke->flags & TVG_SHAPE_STROKE_FLAG_HAS_FILL)
          {
             // fill
             printf("TVG_LOADER: Stroke fill is not compatible now \n"); // TODO
          }
       else
          {
             // color
             memcpy(stroke->color, shape_stroke->color, sizeof(shape_stroke->color));
          }

       // stroke cap
       switch (shape_stroke->flags & TVG_SHAPE_STROKE_FLAG_MASK_CAP) {
          case TVG_SHAPE_STROKE_FLAG_CAP_SQUARE:
             stroke->cap = StrokeCap::Square;
             break;
          case TVG_SHAPE_STROKE_FLAG_CAP_ROUND:
             stroke->cap = StrokeCap::Round;
             break;
          case TVG_SHAPE_STROKE_FLAG_CAP_BUTT:
             stroke->cap = StrokeCap::Butt;
             break;
          default:
             return false;
       }

       // stroke join
       switch (shape_stroke->flags & TVG_SHAPE_STROKE_FLAG_MASK_JOIN) {
          case TVG_SHAPE_STROKE_FLAG_JOIN_BEVEL:
             stroke->join = StrokeJoin::Bevel;
             break;
          case TVG_SHAPE_STROKE_FLAG_JOIN_ROUND:
             stroke->join = StrokeJoin::Round;
             break;
          case TVG_SHAPE_STROKE_FLAG_JOIN_MITER:
             stroke->join = StrokeJoin::Miter;
             break;
          default:
             return false;
       }

       // move pointer
       *pointer += sizeof(tvg_shape_stroke);

       // dashPattern
       if (shape_stroke->flags & TVG_SHAPE_STROKE_FLAG_HAS_DASHPTRN)
          {
             const uint32_t dashPatternCnt = (uint32_t) **pointer;
             *pointer += sizeof(uint32_t);
             const float * dashPattern = (float *) *pointer;
             *pointer += sizeof(float) * dashPatternCnt;

             if (stroke->dashPattern) free(stroke->dashPattern);
             stroke->dashPattern = static_cast<float*>(malloc(sizeof(float) * dashPatternCnt));
             if (!stroke->dashPattern) return false;

             stroke->dashCnt = dashPatternCnt;
             memcpy(stroke->dashPattern, dashPattern, sizeof(float) * dashPatternCnt);

             flag |= RenderUpdateFlag::Stroke;
          }*/

       return true;
    }

    /*
     * Load shape from .tvg binary file
     * Returns true on success and moves pointer to next position or false if corrupted.
     * Details:
     * Shape section starts with uint8 flags, these are describes below in Flags section.
     * If has_fill flag is set, next is uint32 fill identificator. If has_fill is clear, 4*uint8 fill color.
     * Next are ShapePath: uint32 cmdCnt, uint32 ptsCnt, cmdCnt*[PathCommand cmds], ptsCnt*[Point pts].
     * If has_stroke is set, next is stroke informations- see stroke section
     *
     * Flags:
     * xxxxxxx1 - FillRule.EvenOdd (TVG_SHAPE_FLAG_MASK_FILLRULE)
     * xxxxxxx0 - FillRule.Winding
     * xxxxxx1x - HAS_STROKE (TVG_SHAPE_FLAG_HAS_STROKE)
     * xxxxx1xx - HAS_FILL (TVG_SHAPE_FLAG_HAS_FILL) - if set- fillid (gradient fill), if clear- color.
     *
     * [uint8 flags][uint8 lenght][4*uint8 color OR uint32 fillid][ShapePath][9xfloat matrix][stroke]
     */
    bool tvgLoad(const char** pointer)
    {
       const char * moving_pointer = *pointer;
       // flag
       const uint8_t flags = (uint8_t) *moving_pointer;
       moving_pointer += sizeof(uint8_t);

       // lenght
       const uint8_t lenght = (uint8_t) *moving_pointer;
       moving_pointer += sizeof(uint8_t);
       *pointer += sizeof(uint8_t) * lenght;
       // validate lenght
       if (lenght < 8) return false;

       // rule (from flag)
       //rule = (flags & TVG_SHAPE_FLAG_MASK_FILLRULE) ? FillRule::EvenOdd : FillRule::Winding;

       // colors or fill
       if (flags & TVG_SHAPE_FLAG_HAS_FILL)
          {
             // TODO [mmaciola] how store gradients- by id or duplication
             flag |= RenderUpdateFlag::Gradient;
          }
       else
          {
             // colors
             const uint8_t * colors = (uint8_t*) moving_pointer;
             moving_pointer += sizeof(uint8_t) * 4;

             memcpy(&color, colors, sizeof(color));
             flag = RenderUpdateFlag::Color;
          }

       // ShapePath
       const uint32_t cmdCnt = (uint32_t) *moving_pointer;
       moving_pointer += sizeof(uint32_t);
       const uint32_t ptsCnt = (uint32_t) *moving_pointer;
       moving_pointer += sizeof(uint32_t);
       const PathCommand * cmds = (PathCommand *) moving_pointer;
       moving_pointer += sizeof(PathCommand) * cmdCnt;
       const Point * pts = (Point *) moving_pointer;
       moving_pointer += sizeof(Point) * ptsCnt;

       path.cmdCnt = cmdCnt;
       path.ptsCnt = ptsCnt;
       path.reserveCmd(cmdCnt);
       path.reservePts(ptsCnt);
       memcpy(path.cmds, cmds, sizeof(PathCommand) * cmdCnt);
       memcpy(path.pts, pts, sizeof(Point) * ptsCnt);
       flag |= RenderUpdateFlag::Path;

       // ShapeStroke
       if (flags & TVG_SHAPE_FLAG_HAS_STROKE)
          {
             if (!tvgLoadStroke(&moving_pointer)) return false;
          }

       return true;
    }

    /*
     * Store stroke into .tvg binary file
     * Details: see above function tvgLoadStroke
     */
    void tvgStoreStroke(char** pointer)
    {
       /*tvg_shape_stroke * shape_stroke = (tvg_shape_stroke *) *pointer;

       shape_stroke->flags = 0;
       switch (stroke->cap)
         {
         case StrokeCap::Square:
            shape_stroke->flags |= TVG_SHAPE_STROKE_FLAG_CAP_SQUARE;
            break;
         case StrokeCap::Round:
            shape_stroke->flags |= TVG_SHAPE_STROKE_FLAG_CAP_ROUND;
            break;
         case StrokeCap::Butt:
            shape_stroke->flags |= TVG_SHAPE_STROKE_FLAG_CAP_BUTT;
            break;
         }

       switch (stroke->join)
         {
         case StrokeJoin::Bevel:
            shape_stroke->flags |= TVG_SHAPE_STROKE_FLAG_JOIN_BEVEL;
            break;
         case StrokeJoin::Round:
            shape_stroke->flags |= TVG_SHAPE_STROKE_FLAG_JOIN_ROUND;
            break;
         case StrokeJoin::Miter:
            shape_stroke->flags |= TVG_SHAPE_STROKE_FLAG_JOIN_MITER;
            break;
         }

       // width
       shape_stroke->width = stroke->width;

       // color vs fill todo
       memcpy(shape_stroke->color, stroke->color, sizeof(stroke->color));

       // move pointer
       *pointer += sizeof(tvg_shape_stroke);

       // dashPattern
       if (stroke->dashCnt)
          {
             memcpy(*pointer, &stroke->dashCnt, sizeof(uint32_t));
             *pointer += sizeof(uint32_t);
             memcpy(*pointer, stroke->dashPattern, sizeof(float) * stroke->dashCnt);
             *pointer += sizeof(float) * stroke->dashCnt;

             shape_stroke->flags |= TVG_SHAPE_STROKE_FLAG_HAS_DASHPTRN;
          }*/
    }

    /*
     * Store shape from .tvg binary file
     * TODO
     * Details: see above function tvgLoad
     * [uint8 flags][color][path][stroke][fill]
     */
    bool tvgStore()
    {
       // Function below is for testing purposes only. Final tvgStore will be changed completely
       char * buffer = (char *) malloc(2048);
       char * pointer = buffer;

       // flags
       //*pointer = (rule == FillRule::EvenOdd) ? TVG_SHAPE_FLAG_MASK_FILLRULE : 0;
       if (stroke) *pointer |= TVG_SHAPE_FLAG_HAS_STROKE;
       if (fill) *pointer |= TVG_SHAPE_FLAG_HAS_FILL;
       // TODO: matrix store: *pointer |= TVG_SHAPE_FLAG_HAS_TRANSFORM_MATRIX;
       pointer += 1;

       // lenght
       *pointer = 'T';
       pointer += 1;

       // colors or fill
       if (fill)
          {
             // TODO [mmaciola] how store gradients- by id or duplication
          }
       else
          {
             memcpy(pointer, &color, sizeof(color));
             pointer += sizeof(color);
          }

       // ShapePath
       char * pthBuffer;
       uint32_t pthSize;
       path.tvgStore(&pthBuffer, &pthSize);
       memcpy(pointer, pthBuffer, pthSize);
       pointer += pthSize;
       free(pthBuffer);

       // Transform matrix
       // TODO: Matrix store

       // ShapeStroke
       if (stroke)
          {
             tvgStoreStroke(&pointer);
          }

       // lenght
       *(buffer + 1) = pointer - buffer;

       // For testing print
       printf("SHAPE tvgStore:");
       for (char * ptr = buffer; ptr < pointer; ptr++) {
             printf(" %02X", (uint8_t)(*ptr));
       }
       printf(".\n");

       return true;
    }
};

#endif //_TVG_SHAPE_IMPL_H_
