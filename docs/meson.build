sphinx = find_program('sphinx-build', required: get_option('doc'))

if not sphinx.found()
  subdir_done()
endif

doc_src = [
  'conf.py',
  'changes.rst',
  'contributing.rst',
  'index.rst',
  'reference.rst',
  'using.rst',
]

custom_target('doc',
  output: 'html',
  input: doc_src,
  command: [sphinx, '-b', 'html', meson.current_source_dir(), meson.current_build_dir() / 'html'],
  build_by_default: get_option('doc').enabled(),
  install: get_option('doc').enabled(),
  install_dir: get_option('datadir') / 'doc' / meson.project_name(),
  )
