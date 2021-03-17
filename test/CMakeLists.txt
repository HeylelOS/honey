
#########################
# Main executable tests #
#########################

function(test_hny name script dependencies)
	add_test(NAME "${name}"
		COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/support/hny/run-test" 
			-D "${CMAKE_CURRENT_SOURCE_DIR}/unit/hny"
			-E "${CMAKE_BINARY_DIR}/hny"
			-- "${script}"
	)
	set_tests_properties("${name}" PROPERTIES DEPENDS "${dependencies}")
endfunction()

test_hny(TestHoneyExtract extract "")
test_hny(TestHoneyShift   shift   "TestHoneyExtract")
test_hny(TestHoneyStatus  status  "TestHoneyShift")
test_hny(TestHoneySetup   setup   "TestHoneyStatus")
test_hny(TestHoneyList    list    "TestHoneySetup")
test_hny(TestHoneyClean   clean   "TestHoneyList")
test_hny(TestHoneyRemove  remove  "TestHoneyClean")

