#include "stroke_renderer.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/camera2d.hpp>

StrokeRenderer::StrokeRenderer() {
    // Set up initial page; do not request render here because _ready() hasn't run yet.
}

void StrokeRenderer::go_to_page(int p, int w, int h) {

    render_mutex->lock();
    bool change = current_page != p;

    current_page = p;

    // Clean up any previous Cairo resources
    if (cairo_surface) {
        cairo_surface_destroy(cairo_surface);
        cairo_destroy(cairo_ctx);
    }

    // Create a new Cairo surface and context
    cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_ctx = cairo_create(cairo_surface);

    // Create a Godot image and texture
    image = Image::create(w, h, false, Image::FORMAT_RGBA8);
    texture = ImageTexture::create_from_image(image);
    set_texture(texture);

    // Initialize the page map if this page doesn't exist
    if (page_map.find(p) == page_map.end()) {
        page_map.emplace(p, std::vector<Stroke>());
    }
    strokes = &page_map[p];

    render_mutex->unlock();

    if(change)
    {
        request_render();
    }

}

StrokeRenderer::~StrokeRenderer() {
    running = false;
    if (render_semaphore.is_valid()) {
        render_semaphore->post();  // Wake up worker thread so it can exit
    }
    if (render_thread.is_valid()) {
        render_thread->wait_to_finish(); // Ensure the thread stops safely
    }

    cairo_destroy(cairo_ctx);
    cairo_surface_destroy(cairo_surface);
}

void StrokeRenderer::_ready() {
    set_process(true);
    set_process_input(true);

    // Instantiate threading objects in _ready()
    render_thread.instantiate();
    render_mutex.instantiate();
    render_semaphore.instantiate();

    running = true;
    render_thread->start(callable_mp(this, &StrokeRenderer::render_worker));

    go_to_page(0, 800, 600);
    // Optionally, trigger an initial render if needed:
    request_render();
}

void StrokeRenderer::_process(double delta) {

        // request_render();
    
}

void StrokeRenderer::_input(const Ref<InputEvent>& event) {
    Ref<InputEventMouseButton> mouse_button = event;
    Ref<InputEventMouseMotion> mouse_motion = event;

    if (mouse_button.is_valid()) {
        Vector2 local_pos = to_local(get_global_mouse_position());
        if (mouse_button->is_pressed() &&
            mouse_button->get_button_index() == MouseButton::MOUSE_BUTTON_LEFT) {
            start_stroke(local_pos.x, local_pos.y);
        } else if (!mouse_button->is_pressed()) {
            end_stroke();
        }
    }
    if (mouse_motion.is_valid() && drawing) {
        Vector2 local_pos = to_local(get_global_mouse_position());
        update_stroke(local_pos.x, local_pos.y);
    }
}

void StrokeRenderer::start_stroke(double x, double y) {
    drawing = true;
    strokes->push_back(Stroke());
    Stroke& last_stroke = strokes->back();
    last_stroke.points.push_back({ x, y });
    last_stroke.width = 10.0;
    last_stroke.color[0] = 255;
    last_stroke.color[1] = 0;
    last_stroke.color[2] = 0;
    request_render();
}

void StrokeRenderer::update_stroke(double x, double y) {
    if (!drawing || strokes->empty()) return;
    strokes->back().points.push_back({ x, y });
    request_render();
}

void StrokeRenderer::end_stroke() {
    drawing = false;
    request_render();
}

void StrokeRenderer::request_render() {
    if (!render_mutex.is_valid()) {
        // If render_mutex isn't valid, we're not ready to render.
        return;
    }
    render_mutex->lock();
    if (!render_requested) {
        render_requested = true;
        render_semaphore->post();  // Signal the worker thread
    }
    render_mutex->unlock();
}

void StrokeRenderer::render_worker() {
    while (running) {
        render_semaphore->wait();
        if (!running)
            break;

        // Lock briefly to copy strokes and take a reference to the Cairo surface.
        render_mutex->lock();
        auto local_strokes = *strokes;
        cairo_surface_t* local_surface = cairo_surface_reference(cairo_surface);
        render_mutex->unlock();

        // Create a temporary Cairo context for this local surface.
        cairo_t* local_ctx = cairo_create(local_surface);

        // Render strokes using the local context.
        render_cairo(local_ctx, local_strokes);

        // After drawing, perform pixel conversion using our local surface.
        {
            cairo_surface_flush(local_surface);
            int width = cairo_image_surface_get_width(local_surface);
            int height = cairo_image_surface_get_height(local_surface);
            int stride = cairo_image_surface_get_stride(local_surface);
            int bytes_total = width * height * 4;
            updated_data.resize(bytes_total);

            uint8_t* dst_ptr = updated_data.ptrw();
            unsigned char* data = cairo_image_surface_get_data(local_surface);

            for (int y = 0; y < height; y++) {
                uint8_t* src_row = data + y * stride;
                for (int x = 0; x < width; x++) {
                    int src_i = x * 4;
                    int dst_i = (y * width + x) * 4;
                    dst_ptr[dst_i + 0] = src_row[src_i + 2]; // R
                    dst_ptr[dst_i + 1] = src_row[src_i + 1]; // G
                    dst_ptr[dst_i + 2] = src_row[src_i + 0]; // B
                    dst_ptr[dst_i + 3] = src_row[src_i + 3]; // A
                }
            }
        }

        new_texture_ready = true;
        render_requested = false;

        // Clean up the temporary context and referenced surface.
        cairo_destroy(local_ctx);
        cairo_surface_destroy(local_surface);

        // Schedule a deferred call on the main thread to update the texture.
        call_deferred("apply_texture");
    }
}

void StrokeRenderer::render_cairo(cairo_t* local_ctx, const std::vector<Stroke>& in_strokes) {
    // Clear the entire surface to transparent.
    cairo_set_operator(local_ctx, CAIRO_OPERATOR_CLEAR);
    cairo_paint(local_ctx);
    cairo_set_operator(local_ctx, CAIRO_OPERATOR_OVER);

    // Now draw the strokes.
    for (const auto& stroke : in_strokes) {
        // Use your desired transparency here (e.g., 0.5)
        cairo_set_source_rgba(local_ctx, stroke.color[0], stroke.color[1], stroke.color[2], 0.5);
        cairo_set_line_width(local_ctx, stroke.width);
        if (!stroke.points.empty()) {
            cairo_move_to(local_ctx, stroke.points[0].x, stroke.points[0].y);
            for (size_t i = 1; i < stroke.points.size(); ++i) {
                cairo_line_to(local_ctx, stroke.points[i].x, stroke.points[i].y);
            }
            cairo_stroke(local_ctx);
        }
    }
}

// void StrokeRenderer::render_cairo(std::vector<Stroke>& in_strokes) {
//     // Assumes render_mutex is held.
//     // cairo_set_source_rgb(cairo_ctx, 1, 1, 1);
//     // cairo_paint(cairo_ctx);

//     for (const auto& stroke : in_strokes) {
//         cairo_set_source_rgb(cairo_ctx, stroke.color[0], stroke.color[1], stroke.color[2]);
//         cairo_set_line_width(cairo_ctx, stroke.width);
//         if (!stroke.points.empty()) {
//             cairo_move_to(cairo_ctx, stroke.points[0].x, stroke.points[0].y);
//             for (size_t i = 1; i < stroke.points.size(); ++i) {
//                 cairo_line_to(cairo_ctx, stroke.points[i].x, stroke.points[i].y);
//             }
//             cairo_stroke(cairo_ctx);
//         }
//     }

//     // Convert the Cairo surface data (BGRA on little-endian) to Godot's RGBA.
//     cairo_surface_flush(cairo_surface);
//     unsigned char* data = cairo_image_surface_get_data(cairo_surface);
//     int width = cairo_image_surface_get_width(cairo_surface);
//     int height = cairo_image_surface_get_height(cairo_surface);
//     int stride = cairo_image_surface_get_stride(cairo_surface);
//     int bytes_total = width * height * 4;
//     updated_data.resize(bytes_total);

//     uint8_t* dst_ptr = updated_data.ptrw();
//     for (int y = 0; y < height; y++) {
//         uint8_t* src_row = data + y * stride;
//         for (int x = 0; x < width; x++) {
//             int src_i = x * 4;
//             int dst_i = (y * width + x) * 4;
//             dst_ptr[dst_i + 0] = src_row[src_i + 2]; // R
//             dst_ptr[dst_i + 1] = src_row[src_i + 1]; // G
//             dst_ptr[dst_i + 2] = src_row[src_i + 0]; // B
//             dst_ptr[dst_i + 3] = src_row[src_i + 3]; // A
//         }
//     }
// }

void StrokeRenderer::apply_texture() {
    // This function is called on the main thread.
    render_mutex->lock();
    if (new_texture_ready) {
        int width = cairo_image_surface_get_width(cairo_surface);
        int height = cairo_image_surface_get_height(cairo_surface);
        image = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, updated_data);
        texture->update(image);
        set_texture(texture);
        new_texture_ready = false;
    }
    render_mutex->unlock();
}

Ref<Texture2D> StrokeRenderer::get_texture() {
    return texture;
}

void StrokeRenderer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_stroke", "x", "y"), &StrokeRenderer::start_stroke);
    ClassDB::bind_method(D_METHOD("update_stroke", "x", "y"), &StrokeRenderer::update_stroke);
    ClassDB::bind_method(D_METHOD("end_stroke"), &StrokeRenderer::end_stroke);
    ClassDB::bind_method(D_METHOD("go_to_page", "p", "w", "h"), &StrokeRenderer::go_to_page);
    ClassDB::bind_method(D_METHOD("get_texture"), &StrokeRenderer::get_texture);
    ClassDB::bind_method(D_METHOD("apply_texture"), &StrokeRenderer::apply_texture);
    // 'apply_texture' is called via call_deferred so no need to bind it.
}
