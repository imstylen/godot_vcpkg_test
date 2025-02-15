#pragma once

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/texture.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <cairo/cairo.h>
#include <vector>

using namespace godot;

struct Point {
    double x, y;
};

struct Stroke {
    std::vector<Point> points;
    double width;
    double color[3]; // RGB (0-1)
};

class StrokeRenderer : public Node2D {
    GDCLASS(StrokeRenderer, Node2D)

private:
    cairo_surface_t* cairo_surface = nullptr;
    cairo_t* cairo_ctx = nullptr;
   
    Ref<ImageTexture> texture;
    
    Ref<Image> image;
    std::vector<Stroke> strokes;
    Stroke current_stroke;
    bool drawing = false;

protected:
    static void _bind_methods();

public:
    StrokeRenderer();
    ~StrokeRenderer();

    void _ready() override;
    void _process(double delta) override;
    void _input(const Ref<InputEvent> &event) override;

    void start_stroke(double x, double y);
    void update_stroke(double x, double y);
    void end_stroke();
    void redraw();

    Ref<Texture2D> get_texture();
};
