#include "pdf_sprite.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>


#include <cairo/cairo.h>

#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-image.h>

void PDFSprite::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_page_image","index","dpi"),&PDFSprite::get_page_image);   
    ClassDB::bind_method(D_METHOD("set_pdf_path","in_path"),&PDFSprite::set_pdf_path);   
}

PDFSprite::PDFSprite() {
    UtilityFunctions::print("Test node has been created!");

}

PDFSprite::~PDFSprite() {


    UtilityFunctions::print("Test node has been destroyed!");
}

Ref<Texture2D> PDFSprite::get_page_image(int page_index, double dpi) {
    // 1) Load the PDF Document
    if(pdf_path.is_empty())
    {
        return Ref<Texture2D>();
    }

    std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(pdf_path.utf8().get_data()));
    if (!doc) {
        UtilityFunctions::print("Error loading document!");
        return Ref<Texture2D>();
    }

    // 2) Get the desired page
    std::unique_ptr<poppler::page> page(doc->create_page(page_index));
    if (!page) {
        UtilityFunctions::print("Error creating page!");
        return Ref<Texture2D>();
    }

    UtilityFunctions::print("Loaded a PDF, rendering page...");

    // 3) Render to a poppler::image
    poppler::page_renderer renderer;
    // The arguments are DPI in X and Y or a scale factor. For example 200 DPI in X and Y:
    poppler::image poppler_img = renderer.render_page(page.get(), dpi, dpi);
    if (!poppler_img.is_valid()) {
        UtilityFunctions::print("Poppler rendered an invalid image!");
        return Ref<Texture2D>();
    }

    // 4) Determine image size in *pixels* (not PDF points!)
    int img_width = poppler_img.width();
    int img_height = poppler_img.height();
    UtilityFunctions::print("Rendered Poppler image size: ", String::num_int64(img_width), "x", String::num_int64(img_height));

    if (img_width <= 0 || img_height <= 0) {
        UtilityFunctions::print("Invalid Poppler image dimensions!");
        return Ref<Texture2D>();
    }

    // 5) Match Poppler's format to a Cairo format
    cairo_format_t cairo_fmt;
    switch (poppler_img.format()) {
        case poppler::image::format_argb32:
            // Typically BGRA in memory -> we can treat as CAIRO_FORMAT_ARGB32
            cairo_fmt = CAIRO_FORMAT_ARGB32;
            break;
        case poppler::image::format_rgb24:
            // Usually 24-bit color but stored in a 32-bit word with alpha=0xFF or unused
            cairo_fmt = CAIRO_FORMAT_RGB24;
            break;
        default:
            UtilityFunctions::print("Unsupported poppler::image format! Returning empty.");
            return Ref<Texture2D>();
    }

    // 6) Create a Cairo surface for that data
    //    NOTE: poppler_img.data() remains valid as long as poppler_img is in scope.
    cairo_surface_t* surface = cairo_image_surface_create_for_data(
        (unsigned char*)(poppler_img.data()), // Poppler returns const; we cast away here
        cairo_fmt,
        img_width,
        img_height,
        poppler_img.bytes_per_row() // The stride
    );


    if (!surface) {
        UtilityFunctions::print("Failed to create Cairo surface!");
        return Ref<Texture2D>();
    }

    // 7) Access the raw cairo buffer (likely BGRA or BGRx)
    unsigned char* cairo_data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);

    // 8) Convert BGRA -> RGBA in one pass (assuming 4 bytes/pixel).
    //    Even for CAIRO_FORMAT_RGB24, the actual memory layout is 4 bytes per pixel
    //    (the alpha byte might be 0xFF or unused).
    int bytes_total = img_width * img_height * 4;
    PackedByteArray out_data;
    out_data.resize(bytes_total);
    
    memcpy(out_data.ptrw(), cairo_image_surface_get_data(surface), bytes_total);


    // 9) Create a Godot Image (RGBA8) from that data
    //    (FORMAT_RGB8 would be 3 bytes/pixel - mismatch!)
    Ref<Image> gd_image = Image::create_from_data(
        img_width, 
        img_height, 
        false,            // no mipmaps
        Image::FORMAT_RGBA8, 
        out_data
    );
    if (!gd_image.is_valid()) {
        UtilityFunctions::print("Failed to create Godot Image!");
        cairo_surface_destroy(surface);
        return Ref<Texture2D>();
    }

    // 10) Create a Texture2D (ImageTexture) from the Image
    Ref<Texture2D> texture = ImageTexture::create_from_image(gd_image);

    // set_texture(texture);
    if (!texture.is_valid()) {
        UtilityFunctions::print("Failed to create ImageTexture!");
        cairo_surface_destroy(surface);
        return Ref<Texture2D>();
    }

    cairo_surface_destroy(surface);

    
    return texture;
}

void PDFSprite::set_pdf_path(String in_path) {
    pdf_path = in_path;
    dirty = true;
}
