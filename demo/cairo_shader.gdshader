shader_type canvas_item;
render_mode unshaded;

void vertex()
{
	
}
void fragment() {
	// Sample the texture normally.
	vec4 col = texture(TEXTURE, UV);
	// Swap red and blue channels.
	COLOR = vec4(col.b, col.g, col.r, col.a);
}
