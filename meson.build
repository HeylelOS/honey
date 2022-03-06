project('Honey', 'c', license : 'BSD-3-Clause')

##########################
# Dependencies and tools #
##########################

pkgconfig = import('pkgconfig')

#################
# Configuration #
#################

configuration = configuration_data()
configuration.set('CONFIG_HNY_EXTRACTION_BUFFERSIZE_DEFAULT', 4096, description : 'Extraction default internal buffer size')
configuration.set('CONFIG_HNY_EXTRACTION_BUFFERSIZE_MIN', 512, description : 'Extraction minimal internal buffer size')
configuration.set('CONFIG_HNY_EXTRACTION_DICTIONARYMAX_DEFAULT', 'UINT32_MAX', description : 'LZMA2 dictionary max size default')
configuration.set('CONFIG_HNY_EXTRACTION_CPIO_FILENAME_DEFAULT_CAPACITY', 32, description : 'Default filename buffer capacity when extracting cpio')
configuration.set('CONFIG_HNY_REMOVE_DIRSTACK_DEFAULT_CAPACITY', 10, description : 'Remove directory stack default capacity')
configuration.set('CONFIG_HNY_STATUS_BUFFER_DEFAULT_CAPACITY', 120, description : 'Status readlink buffer default capacity')

###########
# Targets #
###########

subdir('include')
subdir('src/libhny')
subdir('src/hny')

#########
# Tests #
#########

subdir('test')

########
# Docs #
########

subdir('docs')
