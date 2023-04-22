find_package(Catch2 3 REQUIRED)

function(add_catch2_test name)
    add_executable(${name} ${name}.cpp)
    target_include_directories(${name} PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/thirdparty/rapidjson/include
    )
    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain)
    add_test(NAME ${name} COMMAND ${name})
endfunction(add_catch2_test)

add_catch2_test(Deserialization)
add_catch2_test(Serialization)