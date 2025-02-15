#pragma once

#include <godot_cpp/classes/ref.hpp>

#include <godot_cpp/classes/texture2d.hpp>

using namespace godot;

class TestNode : public RefCounted
{
    GDCLASS(TestNode, RefCounted);

    int count;
    String pdf_path;

protected:
    static void _bind_methods();

public:
    TestNode();
    ~TestNode();

    Ref<Texture2D> get_page_image(int page_index, double dpi = 25);
    
    void set_pdf_path(String in_path);
};
