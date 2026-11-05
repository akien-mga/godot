# The method found is the script's one: older signatures of the native method it shadows don't apply.
class Edit extends TextEdit:
	@warning_ignore("native_method_override")
	func set_selection_mode(_mode):
		pass

	func call_method():
		set_selection_mode(SELECTION_MODE_SHIFT, 1, 2, 0)


func test():
	pass
