#pragma once

#include <godot_cpp/classes/sprite2d.hpp>
#include <godot_cpp/classes/texture.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/semaphore.hpp>
#include <cairo/cairo.h>
#include <vector>
#include <unordered_map>

using namespace godot;

struct Point {
    double x, y;
};

struct Stroke {
    std::vector<Point> points;
    double width;
    double color[3]; // RGB (0-1)
};

class StrokeRenderer : public Sprite2D {
    GDCLASS(StrokeRenderer, Sprite2D)

private:
    // Data for drawing
    std::unordered_map<int, std::vector<Stroke>> page_map;
    int current_page = 0;

    cairo_surface_t* cairo_surface = nullptr;
    cairo_t* cairo_ctx = nullptr;

    Ref<ImageTexture> texture;
    Ref<Image> image;
    std::vector<Stroke>* strokes;
    bool drawing = false;

    // Godot threading objects
    Ref<Thread> render_thread;
    Ref<Mutex> render_mutex;
    Ref<Semaphore> render_semaphore;
    bool running = true;
    bool render_requested = false;

    // Buffer for updated texture data produced by Cairo
    PackedByteArray updated_data;
    bool new_texture_ready = false;

    // Private helper functions (assume mutex held when appropriate)
    void render_cairo(cairo_t* local_ctx, const std::vector<Stroke>& in_strokes);
    void apply_texture(); // Must be called on the main thread

protected:
    static void _bind_methods();

public:
    StrokeRenderer();
    ~StrokeRenderer();

    void _ready() override;
    void _process(double delta) override;
    void _input(const Ref<InputEvent>& event) override;

    void start_stroke(double x, double y);
    void update_stroke(double x, double y);
    void end_stroke();
    void go_to_page(int p, int w, int h);

    // Worker thread function & render trigger
    void render_worker();
    void request_render();

    Ref<Texture2D> get_texture();
};
