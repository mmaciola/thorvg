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
#include <iostream> // MGS - temp

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
        size_t flagSize = sizeof(FlagType);
        ByteCounter byteCnt = flagSize;
        size_t byteCntSize = sizeof(ByteCounter);

        // fillspread indicator
        flag = TVG_FILL_FLAG_FILLSPREAD;
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;
        // number of bytes associated with fillspread
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;
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
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;

        // colorStops flag
        flag = TVG_FILL_FLAG_COLORSTOPS;
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;
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
            memcpy(*pointer, &flag, flagSize);
            *pointer += flagSize;
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
            memcpy(*pointer, &flag, flagSize);
            *pointer += flagSize;
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
        size_t flagSize = sizeof(FlagType);
        ByteCounter byteCnt = flagSize;  //MGS - cast?
        size_t byteCntSize = sizeof(ByteCounter);

        // cap indicator
        flag = TVG_SHAPE_STROKE_FLAG_CAP; //MGS - flag? indic?
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;
        // number of bytes associated with cap
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;
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
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;

        // join indicator
        flag = TVG_SHAPE_STROKE_FLAG_JOIN; //MGS - flag? indic?
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;
        // number of bytes associated with join
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
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;

        // width flag
        flag = TVG_SHAPE_STROKE_FLAG_WIDTH;
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;
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
            memcpy(*pointer, &flag, flagSize);
            *pointer += flagSize;
            // number of bytes associated with fill - empty
            *pointer += byteCntSize;
            // bytes associated with fill
            ByteCounter byteCnt = serializeFill(pointer, stroke->fill);
            if (!byteCnt) {
               // MGS log + change size of the buffer
               *pointer -= flagSize + byteCntSize;
               return;
            }
            // number of bytes associated with fill - filled
            memcpy(*pointer - byteCntSize - byteCnt, &byteCnt, byteCntSize);
        }
*/

        // color flag
        flag = TVG_SHAPE_STROKE_FLAG_COLOR;
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;
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
            memcpy(*pointer, &flag, flagSize);
            *pointer += flagSize;
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
        size_t flagSize = sizeof(FlagType);
        ByteCounter byteCnt = 0;
        size_t byteCntSize = sizeof(ByteCounter);
        auto sizeofCmdCnt = sizeof(path.cmdCnt);
        auto sizeofCmds = sizeof(path.cmds[0]);
        auto sizeofPtsCnt = sizeof(path.ptsCnt);
        auto sizeofPts = sizeof(path.pts[0]);

        // path commands flag
        flag = TVG_PATH_FLAG_CMDS;
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;
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
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;
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
        size_t flagSize = sizeof(FlagType);
        ByteCounter byteCnt = flagSize;
        size_t byteCntSize = sizeof(ByteCounter);

        // shape indicator
        flag = TVG_SHAPE_BEGIN_INDICATOR;
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;
        // number of bytes associated with shape - empty for now
        *pointer += byteCntSize;

        // fillrule indicator // MGS - indicator? flag? - check all flags indicators 
        flag = TVG_SHAPE_FLAG_FILLRULE;
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;
        // number of bytes associated with fillrule
        memcpy(*pointer, &byteCnt, byteCntSize);
        *pointer += byteCntSize;
        // bytes associated with fillrule flag
        flag = (rule == FillRule::EvenOdd) ? TVG_SHAPE_FLAG_FILLRULE_EVENODD : TVG_SHAPE_FLAG_FILLRULE_WINDING;
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;

        if (stroke) {
            // stroke flag
            flag = TVG_SHAPE_FLAG_HAS_STROKE;
            memcpy(*pointer, &flag, flagSize);
            *pointer += flagSize;
            // number of bytes associated with stroke - empty
            *pointer += byteCntSize;
            // bytes associated with stroke
            byteCnt = serializeStroke(pointer);
            if (!byteCnt) {
               // MGS log + change size of the buffer
               *pointer -= flagSize + byteCntSize;
               return;
            }
            // number of bytes associated with stroke - filled
            memcpy(*pointer - byteCntSize - byteCnt, &byteCnt, byteCntSize);
        }

        if (fill && false) { // *** problem: invalid size
            // fill flag
            flag = TVG_SHAPE_FLAG_HAS_FILL;
            memcpy(*pointer, &flag, flagSize);
            *pointer += flagSize;
            // number of bytes associated with fill - empty
            *pointer += byteCntSize;
            // bytes associated with fill
            ByteCounter byteCnt = serializeFill(pointer, fill);
            if (!byteCnt) {
               // MGS log + change size of the buffer
               *pointer -= flagSize + byteCntSize;
               return;
            }
            // number of bytes associated with fill - filled
            memcpy(*pointer - byteCntSize - byteCnt, &byteCnt, byteCntSize);
        }

        // color flag
        flag = TVG_SHAPE_FLAG_COLOR;
        memcpy(*pointer, &flag, flagSize);
        *pointer += flagSize;
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
            memcpy(*pointer, &flag, flagSize);
            *pointer += flagSize;
            // number of bytes associated with path - empty
            *pointer += byteCntSize;
            // bytes associated with path
            ByteCounter byteCnt = serializePath(pointer);
            if (!byteCnt) {
               // MGS log + change size of the buffer
               *pointer -= flagSize + byteCntSize;
               return;
            }
            // number of bytes associated with path - filled
            memcpy(*pointer - byteCntSize - byteCnt, &byteCnt, byteCntSize);
        }

        // number of bytes associated with shape - filled
        byteCnt = *pointer - start - flagSize - byteCntSize;
        memcpy(*pointer - byteCnt - byteCntSize, &byteCnt, byteCntSize);
        printf("byteCnt : %d \n", byteCnt);

        //return true;
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

       flag |= RenderUpdateFlag::Stroke;
       return LoaderResult::Success;
    }

    /*
     * Load paint and derived classes from .tvg binary file
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
                      stroke->width = _read_tvg_32(block.data);// TODO check conversion *******
                      break;
                   }
                   case TVG_SHAPE_STROKE_FLAG_COLOR: { // stroke color
                      if (block.lenght != sizeof(stroke->color)) return LoaderResult::SizeCorruption;
                      memcpy(stroke->color, &block.data, sizeof(stroke->color));
                      break;
                   }
                   case TVG_SHAPE_STROKE_FLAG_HAS_FILL: { // stroke fill
                      // TODO
                      break;
                   }
                   case TVG_SHAPE_STROKE_FLAG_HAS_DASHPTRN: { // dashed stroke
                      LoaderResult result = tvgLoadStrokeDashptrn(block.data, block.block_end);
                      if (result != LoaderResult::Success) return result;
                      break;
                   }
                   default: {
                      return LoaderResult::InvalidType;
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
       printf("Shape tvgLoad type %d \n", block.type);
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
                break;
             }
             case TVG_SHAPE_FLAG_HAS_FILL: { // fill (gradient)
                // TODO
                flag |= RenderUpdateFlag::Gradient;
                break;
             }
             case TVG_SHAPE_FLAG_COLOR: { // color
                if (block.lenght != sizeof(color)) return LoaderResult::SizeCorruption;
                memcpy(&color, &block.data, sizeof(color)); // TODO: przetestowac
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
};

#endif //_TVG_SHAPE_IMPL_H_
