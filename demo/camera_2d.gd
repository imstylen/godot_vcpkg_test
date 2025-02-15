extends Camera2D

# Adjustable parameters
var zoom_step: float = 0.1
var min_zoom: float = 0.25
var max_zoom: float = 4
var pan_speed: float = 0.5

func _ready() -> void:
	# Makes this Camera2D the current (active) camera
	make_current()

func _input(event: InputEvent) -> void:
	# Handle mouse wheel for zoom
	if event is InputEventMouseButton:
		if event.pressed:
			if event.ctrl_pressed:
				if event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
					_zoom_camera(-zoom_step)
				elif event.button_index == MOUSE_BUTTON_WHEEL_UP:
					_zoom_camera(zoom_step)
			if event.button_index == MOUSE_BUTTON_RIGHT:
				Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED)
		else:
			if event.button_index == MOUSE_BUTTON_RIGHT:
				Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE)
				
	# Handle middle mouse drag for panning
	if event is InputEventMouseMotion:				
		if Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
			# Move opposite to the mouse drag
			position -= event.relative * (1/zoom.x) * pan_speed

func _zoom_camera(delta_zoom: float) -> void:
	# Calculate new zoom
	var new_zoom = zoom.x + delta_zoom
	new_zoom = clamp(new_zoom, min_zoom, max_zoom)
	zoom = Vector2(new_zoom, new_zoom)
