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

	# The type of an untyped variable is only inferred: at runtime, it can hold another class.
	var inferred = TextEdit.new()
	for i in 2:
		inferred.set_selection_mode(TextEdit.SELECTION_MODE_LINE, 1, 2, 0)
		if i == 0:
			print(inferred.get_selection_mode() == TextEdit.SELECTION_MODE_LINE)
			inferred.free()
			inferred = NotEdit.new()
	print(inferred.called)

	# Explicit `self` is an untyped base too, to the compiler.
	var explicit := ExplicitSelf.new()
	explicit.explicit_self()
	print(explicit.get_selection_mode() == TextEdit.SELECTION_MODE_WORD)
	explicit.free()

	# Awaited, on a typed base and on an inferred one.
	var awaited := TextEdit.new()
	await awaited.set_selection_mode(TextEdit.SELECTION_MODE_WORD, 1, 2, 0)
	print(awaited.get_selection_mode() == TextEdit.SELECTION_MODE_WORD)
	awaited.free()
	var awaited_inferred = TextEdit.new()
	for i in 2:
		await awaited_inferred.set_selection_mode(TextEdit.SELECTION_MODE_WORD, 1, 2, 0)
		if i == 0:
			print(awaited_inferred.get_selection_mode() == TextEdit.SELECTION_MODE_WORD)
			awaited_inferred.free()
			awaited_inferred = NotEdit.new()
	print(awaited_inferred.called)


class ExplicitSelf extends TextEdit:
	func explicit_self():
		self.set_selection_mode(SELECTION_MODE_WORD, 1, 2, 0)


class NotEdit:
	var called := false

	func set_selection_mode(_mode, _line, _column, _caret):
		called = true
