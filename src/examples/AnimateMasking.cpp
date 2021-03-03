#include "Common.h"

/************************************************************************/
/* Drawing Commands                                                     */
/************************************************************************/

tvg::Shape *pMaskShape = nullptr;
tvg::Shape *pMask = nullptr;


void tvgDrawCmds(tvg::Canvas* canvas)
{
    if (!canvas) return;

    // background
    auto bg = tvg::Shape::gen();
    bg->appendRect(0,0,WIDTH, HEIGHT,0, 0);
    bg->fill(255, 255, 255, 255);
    canvas->push(move(bg));

    // image
    auto picture1 = tvg::Picture::gen();
    picture1->load(EXAMPLE_DIR"/human.svg");
    auto picture2 = tvg::Picture::gen();
    picture2->load(EXAMPLE_DIR"/bones.svg");
    canvas->push(move(picture1));

    //mask
    auto maskShape = tvg::Shape::gen();
    pMaskShape = maskShape.get();
    maskShape->appendCircle(100, 180, 125, 125);
    maskShape->fill(0, 0, 0, 125);
    maskShape->stroke(25, 25, 25, 255);
    maskShape->stroke(tvg::StrokeJoin::Round);
    maskShape->stroke(10);
    canvas->push(move(maskShape));

    auto mask = tvg::Shape::gen();
    pMask = mask.get();
    mask->appendCircle(100, 180, 125, 125);
    mask->fill(255, 0, 0, 255);

    picture2->composite(move(mask), tvg::CompositeMethod::AlphaMask);
    if (canvas->push(move(picture2)) != tvg::Result::Success) return;
}


/************************************************************************/
/* Sw Engine Test Code                                                  */
/************************************************************************/

static unique_ptr<tvg::SwCanvas> swCanvas;

void tvgSwTest(uint32_t* buffer)
{
    //Create a Canvas
    swCanvas = tvg::SwCanvas::gen();
    swCanvas->target(buffer, WIDTH, WIDTH, HEIGHT, tvg::SwCanvas::ARGB8888);

    /* Push the shape into the Canvas drawing list
       When this shape is into the canvas list, the shape could update & prepare
       internal data asynchronously for coming rendering.
       Canvas keeps this shape node unless user call canvas->clear() */
    tvgDrawCmds(swCanvas.get());
}

void drawSwView(void* data, Eo* obj)
{
    if (swCanvas->draw() == tvg::Result::Success) {
        swCanvas->sync();
    }
}

void tvgUpdateCmds(tvg::Canvas* canvas, float progress)
{
    if (!canvas) return;

    /* Update shape directly.
       You can update only necessary properties of this shape,
       while retaining other properties. */

    // Translate mask object with its stroke & update 
/*
    pMaskShape->translate(0 , progress * 300);
    pMask->translate(0 , progress * 300);
    canvas->update(pMaskShape);
    canvas->update(pMask);
*/
}

void transitSwCb(Elm_Transit_Effect *effect, Elm_Transit* transit, double progress)
{
    tvgUpdateCmds(swCanvas.get(), progress);

    Eo* img = (Eo*) effect;
    evas_object_image_data_update_add(img, 0, 0, WIDTH, HEIGHT);
    evas_object_image_pixels_dirty_set(img, EINA_TRUE);
}

/************************************************************************/
/* GL Engine Test Code                                                  */
/************************************************************************/

static unique_ptr<tvg::GlCanvas> glCanvas;

void initGLview(Evas_Object *obj)
{
    static constexpr auto BPP = 4;

    //Create a Canvas
    glCanvas = tvg::GlCanvas::gen();
    glCanvas->target(nullptr, WIDTH * BPP, WIDTH, HEIGHT);

    /* Push the shape into the Canvas drawing list
       When this shape is into the canvas list, the shape could update & prepare
       internal data asynchronously for coming rendering.
       Canvas keeps this shape node unless user call canvas->clear() */
    tvgDrawCmds(glCanvas.get());
}

void drawGLview(Evas_Object *obj)
{
    auto gl = elm_glview_gl_api_get(obj);
    gl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    gl->glClear(GL_COLOR_BUFFER_BIT);

    if (glCanvas->draw() == tvg::Result::Success) {
        glCanvas->sync();
    }
}

void transitGlCb(Elm_Transit_Effect *effect, Elm_Transit* transit, double progress)
{
}

/************************************************************************/
/* Main Code                                                            */
/************************************************************************/

int main(int argc, char **argv)
{
    tvg::CanvasEngine tvgEngine = tvg::CanvasEngine::Sw;

    if (argc > 1) {
        if (!strcmp(argv[1], "gl")) tvgEngine = tvg::CanvasEngine::Gl;
    }

    //Initialize ThorVG Engine
    if (tvgEngine == tvg::CanvasEngine::Sw) {
        cout << "tvg engine: software" << endl;
    } else {
        cout << "tvg engine: opengl" << endl;
    }

    //Threads Count
    auto threads = std::thread::hardware_concurrency();

    //Initialize ThorVG Engine
    if (tvg::Initializer::init(tvgEngine, threads) == tvg::Result::Success) {

        elm_init(argc, argv);

        Elm_Transit *transit = elm_transit_add();

        if (tvgEngine == tvg::CanvasEngine::Sw) {
            auto view = createSwView();
            elm_transit_effect_add(transit, transitSwCb, view, nullptr);
        } else {
            auto view = createGlView();
            elm_transit_effect_add(transit, transitGlCb, view, nullptr);
        }

        elm_transit_duration_set(transit, 5);
        elm_transit_repeat_times_set(transit, -1);
        elm_transit_auto_reverse_set(transit, EINA_TRUE);
        elm_transit_go(transit);

        elm_run();
        elm_shutdown();

        //Terminate ThorVG Engine
        tvg::Initializer::term(tvgEngine);

    } else {
        cout << "engine is not supported" << endl;
    }
    return 0;
}
