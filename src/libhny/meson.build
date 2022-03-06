libhny = library('hny',
	include_directories : headers,
	install : true,
	sources : [
		'hny_extraction.c',
		'hny_extraction_cpio.c',
		'hny_extraction_lzma2.c',
		'hny_extraction_xz.c',
		'hny_prefix.c',
		'hny_remove.c',
		'hny_shift.c',
		'hny_spawn.c',
		'hny_type.c',
	]
)

pkgconfig.generate(libhny, description : 'Honey package manager core library.')
