#pragma once

#include <godot_cpp/classes/ref.hpp>

#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/sprite2d.hpp>

using namespace godot;

class PDFSprite : public Sprite2D
{
    GDCLASS(PDFSprite, Sprite2D);

    int count;
    String pdf_path;

protected:
    static void _bind_methods();

public:
    PDFSprite();
    ~PDFSprite();

    bool dirty = true;

    Ref<Texture2D> get_page_image(int page_index, double dpi = 25);
    void get_num_pages(); 
    void set_pdf_path(String in_path);
};
