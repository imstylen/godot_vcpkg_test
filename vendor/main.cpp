#include <iostream>

#include <cairo/cairo.h>

#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler-image.h>

int main()
{
    poppler::document* doc = poppler::document::load_from_file("Test.pdf");

    poppler::page* page = doc->create_page(0);

    // Get the page rectangle (default: crop box)
    poppler::rectf rect = page->page_rect(poppler::page_box_enum::crop_box);

    // The width/height are in PDF points (1 inch = 72 points)
    double width = rect.width()/72.0;
    double height = rect.height()/72.0;

    std::cout << "Page  size: " << width << " x " << height << " inches\n";


    std::cout << "Num pages: " << doc->pages() << std::endl;

    poppler::page_renderer renderer;

    auto image = renderer.render_page(page,200.0,200.0);

    // ------------------------------------------------------------------
    // Step 1: Figure out which Cairo format matches the poppler::image
    //         (most often you’ll see 24-bit RGB or 32-bit BGRA).
    // ------------------------------------------------------------------
    cairo_format_t cairo_fmt;
    switch (image.format()) {
    case poppler::image::format_argb32:
        // Usually 'bgra32' in Poppler lines up with CAIRO_FORMAT_ARGB32
        cairo_fmt = CAIRO_FORMAT_ARGB32;
        break;
    case poppler::image::format_rgb24:
        // This usually matches CAIRO_FORMAT_RGB24
        cairo_fmt = CAIRO_FORMAT_RGB24;
        break;
    // ...handle or reject other formats as needed
    default:
        // For other formats, you may need to do a manual conversion,
        // or handle grayscale, etc.
        return 1;
    }

    // ------------------------------------------------------------------
    // Step 2: Wrap the Poppler image data in a Cairo surface
    //
    // NOTE: The poppler::image data remains valid ONLY while 'img' lives.
    //       Keep 'img' around as long as you use 'surface'.
    // ------------------------------------------------------------------
    cairo_surface_t* surface = cairo_image_surface_create_for_data(
        (unsigned char*)image.data(), // raw pixel data
        cairo_fmt,
        image.width(),
        image.height(),
        image.bytes_per_row()         // stride
    );

    // You can now use this surface as a normal Cairo surface
    cairo_t* cr = cairo_create(surface);

    // Example: write the image to a PNG
    cairo_surface_write_to_png(surface, "output.png");

    // Clean up
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    
    
    return 0;
}
