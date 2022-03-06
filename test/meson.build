cover = dependency('cover', required : false)

if cover.found()
	xz = find_program('xz', required : false)

	if xz.found()
		test('hny-test', executable('hny-test', dependencies : cover, sources : 'hny.c'),
			args : [ '-tap', '-' ],
			depends : hny,
			env : {
				'HNY_EXE' : hny.full_path(),
				'XZ_EXE' : xz.full_path(),
			},
			protocol : 'tap'
		)
	endif
endif
