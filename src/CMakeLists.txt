set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(SOURCE_DIR "${ROOT_DIR}/src")
set(SOURCE_FILES
	"${SOURCE_DIR}/AttackCollision.cpp"
	"${SOURCE_DIR}/AttackCollision.h"
	"${SOURCE_DIR}/AttackTrail.cpp"
	"${SOURCE_DIR}/AttackTrail.h"
	"${SOURCE_DIR}/CachedAttackData.h"
	"${SOURCE_DIR}/Events.cpp"
	"${SOURCE_DIR}/Events.h"
	"${SOURCE_DIR}/HitRefs.cpp"
	"${SOURCE_DIR}/HitRefs.h"
	"${SOURCE_DIR}/Hooks.cpp"
	"${SOURCE_DIR}/Hooks.h"
	"${SOURCE_DIR}/main.cpp"
	"${SOURCE_DIR}/ModAPI.cpp"
	"${SOURCE_DIR}/ModAPI.h"
	"${SOURCE_DIR}/Offsets.h"
	"${SOURCE_DIR}/Papyrus.cpp"
	"${SOURCE_DIR}/Papyrus.h"
	"${SOURCE_DIR}/PCH.h"
	"${SOURCE_DIR}/PendingHit.cpp"
	"${SOURCE_DIR}/PendingHit.h"
	"${SOURCE_DIR}/PrecisionAPI.h"
	"${SOURCE_DIR}/PrecisionHandler.cpp"
	"${SOURCE_DIR}/PrecisionHandler.h"
	"${SOURCE_DIR}/Settings.cpp"
	"${SOURCE_DIR}/Settings.h"
	"${SOURCE_DIR}/Utils.cpp"
	"${SOURCE_DIR}/Utils.h"
	"${SOURCE_DIR}/Havok/ActiveRagdoll.h"
	"${SOURCE_DIR}/Havok/Blender.cpp"
	"${SOURCE_DIR}/Havok/Blender.h"
	"${SOURCE_DIR}/Havok/ContactListener.cpp"
	"${SOURCE_DIR}/Havok/ContactListener.h"
	"${SOURCE_DIR}/Havok/Havok.cpp"
	"${SOURCE_DIR}/Havok/Havok.h"
	"${SOURCE_DIR}/render/cbuffer.cpp"
	"${SOURCE_DIR}/render/cbuffer.h"
	"${SOURCE_DIR}/render/common.cpp"
	"${SOURCE_DIR}/render/common.h"
	"${SOURCE_DIR}/render/d3d_context.cpp"
	"${SOURCE_DIR}/render/d3d_context.h"
	"${SOURCE_DIR}/render/DrawHandler.cpp"
	"${SOURCE_DIR}/render/DrawHandler.h"
	"${SOURCE_DIR}/render/line_drawer.cpp"
	"${SOURCE_DIR}/render/line_drawer.h"
	"${SOURCE_DIR}/render/shader.cpp"
	"${SOURCE_DIR}/render/shader.h"
	"${SOURCE_DIR}/render/timer.cpp"
	"${SOURCE_DIR}/render/timer.h"
	"${SOURCE_DIR}/render/vertex_buffer.cpp"
	"${SOURCE_DIR}/render/vertex_buffer.h"
	"${SOURCE_DIR}/render/shaders/shader_vertex_color.h"
	"${SOURCE_DIR}/utilities/ITimer.cpp"
	"${SOURCE_DIR}/utilities/ITimer.h"
)

source_group(TREE "${ROOT_DIR}" FILES ${SOURCE_FILES})

set(VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/Plugin.h")
configure_file(
	"${ROOT_DIR}/cmake/Plugin.h.in"
	"${VERSION_HEADER}"
	@ONLY
)

source_group("src" FILES "${VERSION_HEADER}")

configure_file(
	"${ROOT_DIR}/cmake/version.rc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/version.rc"
	@ONLY
)

add_library(
	"${PROJECT_NAME}"
	SHARED
	${SOURCE_FILES}
	"${VERSION_HEADER}"
	"${CMAKE_CURRENT_BINARY_DIR}/version.rc"
	"${ROOT_DIR}/.clang-format"
	"${ROOT_DIR}/.editorconfig"
)

target_compile_features(
	"${PROJECT_NAME}"
	PRIVATE
		cxx_std_20
)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	target_compile_options(
		"${PROJECT_NAME}"
		PRIVATE
			"/sdl"	# Enable Additional Security Checks
			"/utf-8"	# Set Source and Executable character sets to UTF-8
			"/Zi"	# Debug Information Format

			"/await"

			"/permissive-"	# Standards conformance
			"/Zc:preprocessor"	# Enable preprocessor conformance mode

			"/wd4200" # nonstandard extension used : zero-sized array in struct/union

			"$<$<CONFIG:DEBUG>:>"
			"$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob3>"
	)

	target_link_options(
		"${PROJECT_NAME}"
		PRIVATE
			"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
			"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
	)
endif()

target_include_directories(
	"${PROJECT_NAME}"
	PRIVATE
		"${CMAKE_CURRENT_BINARY_DIR}/src"
		"${SOURCE_DIR}"
)

set(SKSE_SUPPORT_XBYAK ON)

add_subdirectory("$ENV{CommonLibSSEPath_NG}" CommonLibSSE EXCLUDE_FROM_ALL)

add_subdirectory("${ROOT_DIR}/extern/glm" glm)

find_package(tomlplusplus REQUIRED CONFIG)
find_package(xbyak REQUIRED CONFIG)

target_link_libraries(
	"${PROJECT_NAME}"
	PRIVATE
		CommonLibSSE::CommonLibSSE
		glm::glm
		tomlplusplus::tomlplusplus
		xbyak::xbyak
)

target_precompile_headers(
	"${PROJECT_NAME}"
	PRIVATE
		"${SOURCE_DIR}/PCH.h"
)

install(
	FILES
		"$<TARGET_FILE:${PROJECT_NAME}>"
	DESTINATION "SKSE/Plugins"
	COMPONENT "main"
)

install(
	FILES
		"$<TARGET_PDB_FILE:${PROJECT_NAME}>"
	DESTINATION "/"
	COMPONENT "pdbs"
)

if("${COPY_OUTPUT}")
	add_custom_command(
		TARGET "${PROJECT_NAME}"
		POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${CompiledPluginsPath}/SKSE/Plugins/"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${CompiledPluginsPath}/SKSE/Plugins/"
		VERBATIM
	)
endif()
