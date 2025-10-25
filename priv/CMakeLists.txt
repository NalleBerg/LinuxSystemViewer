add_executable(lsv-elevate elevate.c)
set_target_properties(lsv-elevate PROPERTIES OUTPUT_NAME "lsv-elevate")
target_compile_options(lsv-elevate PRIVATE -Wall -Wextra -O2)

# Do not link Qt for this tiny helper

install(TARGETS lsv-elevate RUNTIME DESTINATION bin)
