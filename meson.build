project(
	'libusbgx',
	[ 'c', 'cpp' ],
	version: '0.2.0',
	license: 'LGPL2.1',
	default_options: [
		'warning_level=1',
		'werror=true',
	]
)

add_project_arguments([ '-D_GNU_SOURCE' ], language: 'c')

cc = meson.get_compiler('c')

config = configuration_data()

version = meson.project_version()
version_hex = run_command('sh', '-c', 'printf "0x%02x%02x%04x" $(printf "%s" "@0@" | sed -e "s/\./ /g")'.format(version), check: true).stdout().strip()
config.set('USBG_VERSION_HEX', version_hex)

dependencies = []
sources = []
c_flags = []

libconfig = dependency('libconfig', required: get_option('gadget-schemes'))
libdl = cc.find_library('dl', required: get_option('tests'))
cmocka = dependency('cmocka', required: get_option('tests'))
doxygen = find_program('doxygen', required: get_option('doxygen'))

if libconfig.found()
	c_flags =  ['-DHAS_GADGET_SCHEMES']
	dependencies += libconfig
endif

inc = include_directories('include', '.')

subdir('src')

configure_file(
	input: 'include/usbg/usbg_version.h.in',
	output: 'usbg_version.h',
	configuration: config
)
meson.add_install_script('install-headers.sh')

libusbgx = library(
	'usbgx',
	sources,
	version: '2.0.0',
	dependencies: dependencies,
	include_directories: inc,
	c_args: c_flags,
	install: true,
)

# For subproject:
libusbgx_dep = declare_dependency(
	include_directories: inc,
	link_with: libusbgx,
)

if get_option('examples')
	subdir('examples')
endif

if cmocka.found()
	subdir('tests')
endif

if doxygen.found()
	cfg = configuration_data()
	if cmocka.found()
		cfg.set('BUILD_TESTS_TRUE', '')
	else
		cfg.set('BUILD_TESTS_TRUE', 'X')
	endif
	configure_file(
		input: 'doxygen.cfg.in',
		output: 'doxygen.cfg',
		configuration: cfg
	)

	env = environment()
	env.set('PROJECT', meson.project_name())
	env.set('VERSION', version)
	env.set('DOCDIR', meson.current_build_dir() + '/doc')
	env.set('SRCDIR', meson.current_source_dir())
	run_command(doxygen,
		meson.current_build_dir() + '/doxygen.cfg',
		env: env,
		check: true,
	)
endif

pkgconfig = import('pkgconfig')
pkgconfig.generate(
	libraries: libusbgx,
	version: meson.project_version(),
	filebase: meson.project_name(),
	name: meson.project_name(),
	description: 'USB gadget-configfs library',
)

# TODO: install doxygen?
# TODO: install cmake files
