# App implementation files
set(APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/SIDBlasterApp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/BatchConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/CommandProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/MusicBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/TraceLogger.cpp
)

set(APP_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/SIDBlasterApp.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/BatchConverter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/CommandProcessor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/MusicBuilder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/TraceLogger.h
)