hvn_man = find_program('hvn-man', native : true, required : false)
doxygen = find_program('doxygen', native : true, required : false)

if hvn_man.found()
	man_1_hny = custom_target('hny(1)',
		output : 'hny.1', input : 'hny.1.md',
		command : [ hvn_man, '-i', '@INPUT@', '-o', '@OUTPUT@' ],
		install : true, install_dir : get_option('mandir') / 'man1',
		install_tag : 'man'
	)
endif

if doxygen.found()
	doxyfile = configure_file(configuration : {
		'PROJECT_NAME' : meson.project_name(),
		'INPUT' : '../include/hny.h', # configure_file's dict doesn't take file objects, and we're as of meson 0.61.2 unable to get a full_path() from them
	}, input : 'Doxyfile.in', output : 'Doxyfile')

	doc = custom_target('doc',
		output : 'html', input : interface_header,
		depend_files : [ doxyfile, interface_header ],
		command : [ doxygen, doxyfile ],
		install : true, install_dir : get_option('datadir') / 'html',
		install_tag : 'doc'
	)
endif
