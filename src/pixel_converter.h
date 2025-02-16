#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref.hpp>

#include <godot_cpp/classes/texture2d.hpp>

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/rendering_device.hpp>

#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/rd_shader_source.hpp>
#include <godot_cpp/variant/rid.hpp>

using namespace godot;

class PixelConverter : public RefCounted
{

    GDCLASS(PixelConverter, RefCounted)


public:
    PixelConverter();
    
    void compute();

private:

    String shader_src_string = R"(
abc;
// Invocations in the (x, y, z) dimension
layout(local_size_x = 2, local_size_y = 1, local_size_z = 1) in;

// A binding to the buffer we create in our script
layout(set = 0, binding = 0, std430) restrict buffer MyDataBuffer {
    float data[];
}
my_data_buffer;

// The code we want to execute in each invocation
void main() {
    // gl_GlobalInvocationID.x uniquely identifies this invocation across all work groups
    my_data_buffer.data[gl_GlobalInvocationID.x] *= 2.0;
}
)";


    RenderingDevice* render_device;
    Ref<RDShaderSource> shader_src;
    Ref<RDShaderSPIRV> spirv;
    
    RID shader_rid;

};