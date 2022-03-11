libhny = library('hny',
	include_directories : headers,
	install : true,
	sources : [
		'cpio_decoder.c',
		'hny_extraction.c',
		'hny_prefix.c',
		'hny_remove.c',
		'hny_shift.c',
		'hny_spawn.c',
		'hny_type.c',
		'lzma2_decoder.c',
		'xz_decoder.c',
	]
)

pkgconfig.generate(libhny, description : 'Honey package manager core library.')
