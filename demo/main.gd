extends Sprite2D

var test_node: TestNode
var current_page: int = 0

var page_cache = {}

var worker_thread: Thread
var render_queue = []
var is_worker_running: bool = true

var fast_dpi:float = 25
var slow_dpi:float = 450


func render_worker():
	print("Thread started!!")
	while is_worker_running:
		if render_queue.size() > 0:
			var render_page_index:int = render_queue.pop_front()
			var new_texture: Texture2D = test_node.get_page_image(render_page_index,350)
			page_cache["high_%d" % render_page_index] = new_texture
			call_deferred("_finalize_texture",render_page_index)
			

func _ready() -> void:
		
	test_node = TestNode.new()
	
	worker_thread = Thread.new()
	worker_thread.start(render_worker.bind())
	
	get_viewport().files_dropped.connect(_on_files_dropped)
	
func _on_files_dropped(files: PackedStringArray):
	for file in files:
		if file.ends_with(".pdf"):
			print("PDF dropped: ", file)
			_on_file_selected(file)
		else:
			print("Ignored non-PDF file: ", file)
			
func _on_file_selected(path: String) -> void:
	# Do something with the selected file path
	print("Selected file: ", path)
		# Create (or otherwise obtain) your TestNode instance
	
	texture = null
	current_page = 0
	page_cache = {}
	test_node.set_pdf_path(path)
	
	go_to_next_page(0)
	
	
func _input(event: InputEvent) -> void:
	# Only handle mouse button events that are pressed
	if event is InputEventKey:
		if event.keycode == KEY_ESCAPE:
			scale= Vector2(1,1)
	if event is InputEventMouseButton and event.pressed:
		# Check if the Control key is held down
		if not event.ctrl_pressed:
			if event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
				go_to_next_page(1)
			elif event.button_index == MOUSE_BUTTON_WHEEL_UP:
				go_to_next_page(-1)

func go_to_next_page(delta) -> void:
	current_page += delta
	
	if page_cache.has("high_%d" % current_page):
		texture = page_cache["high_%d" % current_page]
		_update_stroke_page()
		return
		
	var new_texture: Texture2D = test_node.get_page_image(current_page,25)

	# If out of bounds, loop back to page 0
	if new_texture == null:
		current_page-=delta
		return
	
	page_cache["low_%d" % current_page] = new_texture
	
	texture = new_texture
	scale = Vector2(slow_dpi/fast_dpi, slow_dpi/fast_dpi)
	_update_stroke_page()
	
	render_queue.push_front(current_page)
	
func _finalize_texture(page_index: int):
	if current_page == page_index:
		texture = page_cache["high_%d" % current_page]
		scale = Vector2(1,1)
		_update_stroke_page()
		
func _update_stroke_page():
	$StrokeRenderer.go_to_page(current_page,texture.get_width(),texture.get_height())
