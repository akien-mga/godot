# Only signatures the engine kept are valid overrides, not any other.
class Override extends TextServerExtension:
	func _font_draw_glyph(_font: RID, _canvas: RID, _size: int):
		pass


func test():
	pass
