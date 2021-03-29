#include "Common.h"
#include <fstream>
#include <iostream>
#include <chrono>

/************************************************************************/
/* Drawing Commands                                                     */
/************************************************************************/
using namespace std::chrono;

uint32_t *data = nullptr;

void tvgDrawCmds(tvg::Canvas* canvas)
{
    if (!canvas) return;

    //Create the main scene
    auto scene = tvg::Scene::gen();

    auto start = high_resolution_clock::now();
    for (int ii = -5; ii < 5; ++ii)
    {
        for (int jj = -5; jj < 20; ++jj)
        {
            auto svg = tvg::Picture::gen();
//            svg->load(EXAMPLE_DIR"/tiger.svg"); -5 0, -5 0
            svg->load(EXAMPLE_DIR"/car.svg"); //-5,5 -5,20
//            svg->load(EXAMPLE_DIR"/cartman.svg");//0-80, 0-80
            svg->translate(20*ii,20*jj);

            scene->push(move(svg));
        }
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Svg file load time: " << duration.count() << " microseconds" << endl;

    scene->save(EXAMPLE_DIR"/tvg_file.tvg");

    //Draw the Scene onto the Canvas
    canvas->push(move(scene));
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
    auto threads = std::thread::hardware_concurrency() - 1;

    //Initialize ThorVG Engine
    if (tvg::Initializer::init(tvgEngine, threads) == tvg::Result::Success) {

        elm_init(argc, argv);

        auto start_tot = high_resolution_clock::now();
        if (tvgEngine == tvg::CanvasEngine::Sw) {
            createSwView();
        } else {
            createGlView();
        }
        auto stop_tot = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop_tot - start_tot);
        cout << "Total view creating time: " << duration.count() << " microseconds (number of threads: " << threads << ")" << endl;

        elm_run();
        elm_shutdown();

        //Terminate ThorVG Engine
        tvg::Initializer::term(tvgEngine);

        if (data) free(data);

    } else {
        cout << "engine is not supported" << endl;
    }
    return 0;
}
