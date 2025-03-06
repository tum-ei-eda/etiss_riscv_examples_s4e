add_executable_etiss_bylib(chacha20 chacha20_lib)
add_subdirectory(impl)
target_link_libraries(chacha20 chacha20_lib)
