set(APPLICATION_NAME Colloid)
add_executable(${APPLICATION_NAME})
set_target_properties(${APPLICATION_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${APPLICATION_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(${APPLICATION_NAME} PROPERTIES CXX_EXTENSIONS OFF)

target_sources(
        ${APPLICATION_NAME}
        PRIVATE
        Main.cpp
        colloid/Renderer.cpp
        colloid/Application.cpp

        PRIVATE
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        colloid/Renderer.h
        colloid/parsers/ObjectParser.h
        colloid/Mesh.h
        colloid/Application.h
        colloid/Primitives.h
        colloid/Camera.h
        colloid/Gui.h
)

target_include_directories(
        ${APPLICATION_NAME}
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)


target_link_libraries(
        ${APPLICATION_NAME}
        PRIVATE
        Falcon::Falcon
        SDL3::SDL3
        fast_float
        fastgltf::fastgltf
        # ${APPLICATION_NAME}-StrictWarnings # TODO: Add back after fixing warnings
        # ${APPLICATION_NAME}-ProjectSanitizers # TODO: Add back after fixing warnings

        PUBLIC
        DearImGui::DearImGui
)
AddCompilerFlag(${APPLICATION_NAME} PRIVATE "AUTO")

# Copy models directory to the build directory
add_custom_command(
        TARGET ${APPLICATION_NAME}
        POST_BUILD
        # Create the directory
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${APPLICATION_NAME}>/models"
        # Copy files into the target directory
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/models"
        "$<TARGET_FILE_DIR:${APPLICATION_NAME}>/models"

        COMMENT "Copying models to build directory"
)