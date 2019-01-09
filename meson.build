project('rauc-hawkbit-updater', 'c',
	 license : 'LGPLv2.1')

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

systemddep = dependency('libsystemd', required : get_option('systemd'))
if systemddep.found()
  sources_updater += 'src/sd-helper.c'
endif

gnome = import('gnome')
dbus = 'rauc-installer-gen'
dbus_ifaces = files('src/rauc-installer.xml')
dbus_sources = gnome.gdbus_codegen(
  dbus,
  sources: dbus_ifaces,
  interface_prefix: 'de.pengutronix.rauc.',
  namespace: 'R',
)

executable('rauc-hawkbit-updater',
	   sources_updater,
	   dbus_sources,
	   dependencies : [libcurldep, giodep, giounixdep, jsonglibdep, systemddep],
	   include_directories : incdir)
