test_exe = executable('test_exe',
	[
		'test.c',
		'usbg-test.c',
		'usbg-io-wrappers.c',
	],
	dependencies: [
		libusbgx_dep,
		cmocka,
	],
)

header_checker = executable('header_checker',
	[
		'header_checker.cpp',
	],
	dependencies: [
		libusbgx_dep,
		cmocka,
	],
)

# TODO: also run with --generate-config or --use-config
test('test_exe', test_exe)

test('header_checker', header_checker)
