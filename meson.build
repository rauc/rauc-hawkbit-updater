project(
  'rauc-hawkbit-updater',
  'c',
  version : '1.4',
  meson_version : '>=0.50',
  default_options: [
    'warning_level=2',
  ],
  license : 'LGPL-2.1-only',
)

conf = configuration_data()
conf.set_quoted('PROJECT_VERSION', meson.project_version())

libcurldep = dependency('libcurl', version : '>=7.47.0')
giodep = dependency('gio-2.0', version : '>=2.26.0')
giounixdep = dependency('gio-unix-2.0', version : '>=2.26.0')
jsonglibdep = dependency('json-glib-1.0')

incdir = include_directories('include')

sources_updater = [
  'src/rauc-hawkbit-updater.c',
  'src/rauc-installer.c',
  'src/config-file.c',
  'src/hawkbit-client.c',
  'src/json-helper.c',
  'src/log.c',
]

c_args = '''
  -Wbad-function-cast
  -Wcast-align
  -Wdeclaration-after-statement
  -Wformat=2
  -Wshadow
  -Wno-unused-parameter
  -Wno-missing-field-initializers
'''.split()
add_project_arguments(c_args, language : 'c')

systemddep = dependency('systemd', required : get_option('systemd'))
libsystemddep = dependency('libsystemd', required : get_option('systemd'))

if systemddep.found()
  conf.set('WITH_SYSTEMD', '1')
  sources_updater += 'src/sd-helper.c'
  systemdsystemunitdir = get_option('systemdsystemunitdir')
  if systemdsystemunitdir == ''
    systemdsystemunitdir = systemddep.get_pkgconfig_variable('systemdsystemunitdir')
  endif
  install_data('script/rauc-hawkbit-updater.service', install_dir : systemdsystemunitdir)
endif

gnome = import('gnome')
dbus = 'rauc-installer-gen'
dbus_ifaces = files('src/rauc-installer.xml')
dbus_sources = gnome.gdbus_codegen(
  dbus,
  sources : dbus_ifaces,
  interface_prefix : 'de.pengutronix.rauc.',
  namespace: 'R',
)

config_h = configure_file(
  output : 'config.h',
  configuration : conf
)
add_project_arguments('-include' + meson.current_build_dir() / 'config.h', language: 'c')

doxygen = find_program('doxygen', required : get_option('apidoc'))

if doxygen.found()
  doc_config = configuration_data()
  doc_config.set('DOXYGEN_OUTPUT', meson.current_build_dir() / 'doxygen')
  doc_config.set('DOXYGEN_INPUT', meson.current_source_dir() / 'src' + ' ' + meson.current_source_dir() / 'include')

  doxyfile = configure_file(input : 'Doxyfile.in',
    output : 'Doxyfile',
    configuration : doc_config,
    install : false)
  custom_target('doxygen',
    output : 'doxygen',
    input : doxyfile,
    command : [doxygen, '@INPUT@'],
    depend_files : sources_updater,
    build_by_default : get_option('apidoc').enabled(),
    )
endif

subdir('docs')

executable('rauc-hawkbit-updater',
  sources_updater,
  dbus_sources,
  config_h,
  dependencies : [libcurldep, giodep, giounixdep, jsonglibdep, libsystemddep],
  include_directories : incdir,
  install: true)
