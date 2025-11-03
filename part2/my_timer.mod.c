#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb9e81daf, "proc_remove" },
	{ 0xf46d5bf3, "mutex_lock" },
	{ 0x680628e7, "ktime_get_real_ts64" },
	{ 0x40a621c5, "scnprintf" },
	{ 0xf46d5bf3, "mutex_unlock" },
	{ 0xa61fd7aa, "__check_object_size" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0xf64ac983, "__copy_overflow" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0xe4de56b4, "__ubsan_handle_load_invalid_value" },
	{ 0xd272d446, "__fentry__" },
	{ 0xf8d7ac5e, "proc_create" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0x70eca2ca, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0xb9e81daf,
	0xf46d5bf3,
	0x680628e7,
	0x40a621c5,
	0xf46d5bf3,
	0xa61fd7aa,
	0x092a35a2,
	0xf64ac983,
	0xd272d446,
	0xe4de56b4,
	0xd272d446,
	0xf8d7ac5e,
	0xd272d446,
	0x70eca2ca,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"proc_remove\0"
	"mutex_lock\0"
	"ktime_get_real_ts64\0"
	"scnprintf\0"
	"mutex_unlock\0"
	"__check_object_size\0"
	"_copy_to_user\0"
	"__copy_overflow\0"
	"__stack_chk_fail\0"
	"__ubsan_handle_load_invalid_value\0"
	"__fentry__\0"
	"proc_create\0"
	"__x86_return_thunk\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "9C12C7F07306A75298708E5");
