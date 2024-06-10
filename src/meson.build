sources += files(
	'usbg.c',
	'usbg_error.c',
	'usbg_common.c',
	'function/ether.c',
	'function/ffs.c',
	'function/midi.c',
	'function/ms.c',
	'function/phonet.c',
	'function/serial.c',
	'function/loopback.c',
	'function/hid.c',
	'function/uac2.c',
	'function/uvc.c',
	'function/printer.c',
)

if libconfig.found()
	sources += files(
		'usbg_schemes_libconfig.c',
		'usbg_common_libconfig.c'
	)
else
	sources += files('usbg_schemes_none.c')
endif
