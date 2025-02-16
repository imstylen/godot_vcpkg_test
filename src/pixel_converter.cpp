#include "pixel_converter.h"
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>

PixelConverter::PixelConverter() {

}

void PixelConverter::compute() {
	render_device = RenderingServer::get_singleton()->create_local_rendering_device();

    shader_src.instantiate();
    shader_src->set_language(RenderingDevice::ShaderLanguage::SHADER_LANGUAGE_GLSL);
    shader_src->set_stage_source(RenderingDevice::ShaderStage::SHADER_STAGE_COMPUTE,shader_src_string);
    
    spirv = render_device->shader_compile_spirv_from_source(shader_src);
    UtilityFunctions::print(spirv->get_stage_compile_error(RenderingDevice::ShaderStage::SHADER_STAGE_COMPUTE));
    shader_rid = render_device->shader_create_from_spirv(spirv);

    UtilityFunctions::print("Pixel Converter Created!");

    // 1. Prepare input data: an array of 10 floats [1, 2, ..., 10]
    PackedFloat32Array input;
    for (int i = 0; i < 10; i++) {
        input.push_back(static_cast<float>(i + 1));
    }

    // Convert the float array to a byte array for GPU transfer.
    PackedByteArray input_bytes = input.to_byte_array();

    // 2. Create a storage buffer using the input data.
    // Each float is 4 bytes; input_bytes.size() already reflects the total byte count.
    uint32_t buffer_size = input_bytes.size();
    RID buffer_rid = render_device->storage_buffer_create(buffer_size, input_bytes);

    // 3. Set up the uniform that points to our storage buffer.
    RDUniform* uniform = memnew(RDUniform());
    uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
    uniform->set_binding(0);  // Must match the "binding" in the shader.
    uniform->add_id(buffer_rid);

    // Place the uniform in a vector (or array) for creating a uniform set.
    TypedArray<RDUniform> uniforms;
    uniforms.push_back(Variant(uniform));// not correct. expects a variant.

    // Create a uniform set with the uniforms, linking it to our shader.
    // The last parameter '0' should match the "set" in our shader file.
    RID uniform_set = render_device->uniform_set_create(uniforms, shader_rid, 0);

    // 4. Create a compute pipeline from the precompiled shader.
    RID pipeline = render_device->compute_pipeline_create(shader_rid);

    // Begin recording commands for the compute shader.
    int64_t compute_list = render_device->compute_list_begin();

    // Bind the compute pipeline and uniform set.
    render_device->compute_list_bind_compute_pipeline(compute_list, pipeline);
    render_device->compute_list_bind_uniform_set(compute_list, uniform_set, 0);

    // Dispatch the compute shader.
    // With 5 work groups in X and 2 local invocations per workgroup (as specified in the shader),
    // this gives a total of 10 invocations.
    render_device->compute_list_dispatch(compute_list, 5, 1, 1);

    // End the command list.
    render_device->compute_list_end();

    // 5. Submit the commands to the GPU and synchronize (wait for execution).
    render_device->submit();
    render_device->sync();

    // 6. Retrieve the data back from the GPU.
    PackedByteArray output_bytes = render_device->buffer_get_data(buffer_rid);
    PackedFloat32Array output = output_bytes.to_float32_array();

    // Print the input and output arrays.
    UtilityFunctions::print("Input:");
    for (int i = 0; i < input.size(); i++) {
        UtilityFunctions::print(String::num(input[i]));
    }

    UtilityFunctions::print("Output:");
    for (int i = 0; i < output.size(); i++) {
        UtilityFunctions::print(String::num(output[i]));
    }
}
