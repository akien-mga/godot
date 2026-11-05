# Calls that only fit an older signature of a native method, kept for compatibility, use that one.

class Edit extends TextEdit:
	func implicit_self():
		set_selection_mode(SELECTION_MODE_SHIFT, 1, 2, 0)

	func in_lambda():
		var call_method := func(): set_selection_mode(SELECTION_MODE_LINE, 1, 2, 0)
		call_method.call()


func test():
	var edit := Edit.new()
	edit.implicit_self()
	print(edit.get_selection_mode() == TextEdit.SELECTION_MODE_SHIFT)
	edit.in_lambda()
	print(edit.get_selection_mode() == TextEdit.SELECTION_MODE_LINE)

	# Typed base, leaving out defaulted arguments of the older signature.
	edit.set_selection_mode(TextEdit.SELECTION_MODE_SHIFT, 1)
	print(edit.get_selection_mode() == TextEdit.SELECTION_MODE_SHIFT)
	edit.free()

	# The current signature is still preferred when the arguments fit it.
	var plain := TextEdit.new()
	plain.set_selection_mode(TextEdit.SELECTION_MODE_WORD)
	print(plain.get_selection_mode() == TextEdit.SELECTION_MODE_WORD)
	plain.free()
