#include "stroke_renderer.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/viewport.hpp>

StrokeRenderer::StrokeRenderer() {
    int width = 800, height = 600;

    // Create Cairo Surface & Context
    cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_ctx = cairo_create(cairo_surface);

    // Clear canvas (white background)
    // cairo_set_source_rgb(cairo_ctx, 1, 1, 1);
    // cairo_paint(cairo_ctx);

    // Create Godot Image & Texture
    image = Image::create(width, height, false, Image::FORMAT_RGBA8);
    texture = ImageTexture::create_from_image(image);
}

StrokeRenderer::~StrokeRenderer() {
    cairo_destroy(cairo_ctx);
    cairo_surface_destroy(cairo_surface);
}

void StrokeRenderer::_ready() {
    set_process(true);
    set_process_input(true);
}

void StrokeRenderer::_process(double delta) {
    if (drawing) {
        redraw();
    }
}

void StrokeRenderer::_input(const Ref<InputEvent> &event) {
    Ref<InputEventMouseButton> mouse_button = event;
    Ref<InputEventMouseMotion> mouse_motion = event;

    double zoom = get_viewport()->get_camera_2d()->get_zoom().x;

    
    if (mouse_button.is_valid()) {
        Vector2 cairo_pos = to_local(get_global_mouse_position());
        cairo_pos.x = cairo_pos.x/zoom;
        cairo_pos.y = cairo_pos.y/zoom;


        if (mouse_button->is_pressed() && mouse_button->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
            start_stroke(cairo_pos.x, cairo_pos.y);
        } else if (!mouse_button->is_pressed()) {
            end_stroke();
        }
    }

    if (mouse_motion.is_valid() && drawing) {
        Vector2 cairo_pos = to_local(get_global_mouse_position());
        cairo_pos.x = cairo_pos.x/zoom;
        cairo_pos.y = cairo_pos.y/zoom;

        update_stroke(cairo_pos.x, cairo_pos.y);
    }
}


Ref<Texture2D> StrokeRenderer::get_texture() {
	return texture;
}

void StrokeRenderer::start_stroke(double x, double y) {
    UtilityFunctions::print("Starting stroke!");
    drawing = true;

    // Create a new stroke and add it to the strokes vector
    strokes.push_back(Stroke());
    Stroke& last_stroke = strokes.back(); // Reference to the last stroke

    last_stroke.points.push_back({x, y});
    last_stroke.width = 2.0;
    last_stroke.color[0] = 0;  // Black
    last_stroke.color[1] = 0;
    last_stroke.color[2] = 0;
}

void StrokeRenderer::update_stroke(double x, double y) {
    if (!drawing || strokes.empty()) return;

    // Append new points to the last stroke in the vector
    strokes.back().points.push_back({x, y});
}

void StrokeRenderer::end_stroke() {
    UtilityFunctions::print("Finishing stroke!");
    drawing = false;
}

void StrokeRenderer::redraw() {
    // Clear canvas
    // cairo_set_source_rgb(cairo_ctx, 1, 1, 1);
    // cairo_paint(cairo_ctx);

    for (const auto& stroke : strokes) {
        cairo_set_source_rgb(cairo_ctx, stroke.color[0], stroke.color[1], stroke.color[2]);
        cairo_set_line_width(cairo_ctx, stroke.width);

        if (!stroke.points.empty()) {
            cairo_move_to(cairo_ctx, stroke.points[0].x, stroke.points[0].y);
            for (size_t i = 1; i < stroke.points.size(); ++i) {
                cairo_line_to(cairo_ctx, stroke.points[i].x, stroke.points[i].y);
            }
            cairo_stroke(cairo_ctx);
        }
    }

    // Convert Cairo surface to Godot texture
    // image->lock();
    unsigned char* data = cairo_image_surface_get_data(cairo_surface);
    int stride = cairo_image_surface_get_stride(cairo_surface);
    int width = cairo_image_surface_get_width(cairo_surface);
    int height = cairo_image_surface_get_height(cairo_surface);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * stride + x * 4;
            Color pixel(data[index + 2] / 255.0, data[index + 1] / 255.0, data[index] / 255.0, data[index + 3] / 255.0);
            image->set_pixel(x, y, pixel);
        }
    }
    // image->unlock();

    texture->update(image);
}

void StrokeRenderer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_stroke", "x", "y"), &StrokeRenderer::start_stroke);
    ClassDB::bind_method(D_METHOD("update_stroke", "x", "y"), &StrokeRenderer::update_stroke);
    ClassDB::bind_method(D_METHOD("end_stroke"), &StrokeRenderer::end_stroke);
    ClassDB::bind_method(D_METHOD("get_texture"), &StrokeRenderer::get_texture);
}
