# Generates src/assets_tor.qrc from the directory chosen by the Tor embedding
# block in the top-level CMakeLists.txt. TOR_EMBED_DIR is empty when this build
# ships no Tor, which produces an empty resource.
#
# The paths written into the .qrc are always relative to src/, because that is
# where the generated .qrc lives; TOR_EMBED_DIR only supplies the file names.

set(QRC_LIST)
set(TOR_BIN_FOUND 0)

if (TOR_EMBED_DIR)
    file(GLOB TOR_FILES LIST_DIRECTORIES false "${TOR_EMBED_DIR}/*")

    foreach(FILE ${TOR_FILES})
        get_filename_component(FILE_REL "${FILE}" NAME)

        # file(GLOB) matches dotfiles, and src/assets/tor carries a .gitkeep.
        if (FILE_REL MATCHES "^\\.")
            continue()
        endif()

        list(APPEND QRC_LIST "        <file>assets/tor/${FILE_REL}</file>")

        if (FILE_REL STREQUAL "tor" OR FILE_REL STREQUAL "tor.exe")
            set(TOR_BIN_FOUND 1)
        endif()
    endforeach()

    if (NOT TOR_BIN_FOUND)
        message(FATAL_ERROR "The Tor binary could not be found in ${TOR_EMBED_DIR}")
    endif()
endif()

list(JOIN QRC_LIST "\n" QRC_DATA)
configure_file("cmake/assets_tor.qrc" "${CMAKE_CURRENT_SOURCE_DIR}/src/assets_tor.qrc")
