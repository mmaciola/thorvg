#include "Common.h"
#include <fstream>

/************************************************************************/
/* Drawing Commands                                                     */
/************************************************************************/

uint32_t *data = nullptr;

void tvgDrawCmds(tvg::Canvas* canvas)
{
    if (!canvas) return;

    //Create the main scene
    auto scene = tvg::Scene::gen();

    //Image
    ifstream file(EXAMPLE_DIR"/rawimage_200x300.raw");
    if (!file.is_open()) return;
    data = (uint32_t*) malloc(sizeof(uint32_t) * (200 * 300));
    file.read(reinterpret_cast<char *>(data), sizeof (data) * 200 * 300);
    file.close();

    //RadialGradient
    auto fill = tvg::RadialGradient::gen();
    fill->radial(201, 199, 200);
    //Gradient Color Stops
    tvg::Fill::ColorStop colorStops[2];
    colorStops[0] = {0, 255, 203, 255, 255};
    colorStops[1] = {1, 0, 24, 0, 255};
    fill->colorStops(colorStops, 2);

    //Round Rectangle
    auto shape1 = tvg::Shape::gen();
    shape1->appendRect(2, 9, 401, 404, 49, 51);
    shape1->stroke(5);
    shape1->stroke(255,0,0,200);
    float dashPattern[3] = {20, 10, 17.98};
    shape1->stroke(dashPattern, 3);
    shape1->fill(move(fill));

    //Image
    auto image = tvg::Picture::gen();
    if (image->load(data, 200, 300, true) != tvg::Result::Success) return;
    image->translate(500, 400);

    //Scene - child
    auto scene2 = tvg::Scene::gen();
    auto shape2 = tvg::Shape::gen();
    shape2->appendRect(30, 40, 200, 200, 10, 40);
    shape2->fill(255, 0, 255, 125);
    scene2->push(move(shape2));
    scene2->rotate(10);
    scene2->translate(300,0);

    //Mask + shape
    auto shape3 = tvg::Shape::gen();
    shape3->appendRect(0, 400, 400, 400, 0, 0);
    shape3->fill(0, 0, 255, 255);

    auto mask = tvg::Shape::gen();
    mask->appendCircle(200, 600, 125, 125);
    mask->fill(255, 0, 0, 255);
    shape3->composite(move(mask), tvg::CompositeMethod::AlphaMask);

    //svg
    auto svg = tvg::Picture::gen();
    if (svg->load(EXAMPLE_DIR"/tiger.svg") != tvg::Result::Success) return;
    svg->opacity(100);
    svg->scale(1);
    svg->translate(100, 100);

    scene->push(move(shape1));
    scene->push(move(image));

    scene->push(move(scene2));
    scene->push(move(shape3));
    scene->push(move(svg));

    if (scene->save(EXAMPLE_DIR"/tvg_file.tvg") != tvg::Result::Success) return;

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
    auto threads = 0; //std::thread::hardware_concurrency();

    //Initialize ThorVG Engine
    if (tvg::Initializer::init(tvgEngine, threads) == tvg::Result::Success) {

        elm_init(argc, argv);

        if (tvgEngine == tvg::CanvasEngine::Sw) {
            createSwView();
        } else {
            createGlView();
        }

        elm_run();
        elm_shutdown();

        //Terminate ThorVG Engine
        tvg::Initializer::term(tvg::CanvasEngine::Sw);

        if (data) free(data);

    } else {
        cout << "engine is not supported" << endl;
    }
    return 0;
}
