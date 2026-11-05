# Functions can override an older signature of a native virtual method, kept for compatibility.

class OlderOverride extends TextServerExtension:
	var called := []

	func _font_draw_glyph(_font: RID, _canvas: RID, size: int, _pos: Vector2, index: int, _color: Color) -> void:
		called.push_back([size, index])


class UntypedOlderOverride extends TextServerExtension:
	var called := []

	func _font_draw_glyph(_font, _canvas, size, _pos, index, _color):
		called.push_back([size, index])


class CurrentOverride extends TextServerExtension:
	var called := []

	func _font_draw_glyph(_font: RID, _canvas: RID, size: int, _pos: Vector2, index: int, _color: Color, oversampling: float) -> void:
		called.push_back([size, index, oversampling])


# The older signature fits this one too: it must still be called only once.
class CurrentOverrideWithDefault extends TextServerExtension:
	var called := []

	func _font_draw_glyph(_font: RID, _canvas: RID, size: int, _pos: Vector2, index: int, _color: Color, oversampling: float = -1.0) -> void:
		called.push_back([size, index, oversampling])


func draw_with(server: TextServerExtension) -> Array:
	server.font_draw_glyph(RID(), RID(), 16, Vector2(), 3, Color.WHITE, 2.0)
	return server.get("called")


func test():
	print(draw_with(OlderOverride.new()))
	print(draw_with(UntypedOlderOverride.new()))
	print(draw_with(CurrentOverride.new()))
	print(draw_with(CurrentOverrideWithDefault.new()))
