project(QtUtil)

include_directories(.)
include_directories(./lib)

if (APPLE)
    ADD_DEFINITIONS(-D_MACOS)
elseif (UNIX)
    ADD_DEFINITIONS(-D_LINUX)
    link_libraries(uuid)
else ()
    ADD_DEFINITIONS(-D_WINDOWS)
    link_libraries(uuid)

endif ()

set(SOURCE_FILES
        Entity/JID.cpp
        Entity/JID.h
        lib/Base64/base64.h
        lib/Base64/base64.cpp
#        lib/cjson/cJSON.h
#        lib/cjson/cJSON.c
#        lib/cjson/cJSON_Utils.c
#        lib/cjson/cJSON_Utils.h
        lib/http/QtHttpRequest.h
        lib/http/QtHttpRequest.cpp
        lib/Md5/Md5.h
        lib/Md5/Md5.cpp
        lib/ini/ini.c
        lib/ini/ini.h
        lib/ini/ConfigLoader.cpp
        lib/ini/ConfigLoader.h
        Utils/utils.cpp
        Utils/utils.h
        qtutil_global.h
        lib/ini/ConfigLoader.cpp
        lib/ini/ConfigLoader.h
        Utils/Log.h
        Utils/Log.cpp
        lib/spdlog/async.h
        lib/spdlog/async_logger.h
        lib/spdlog/common.h
        lib/spdlog/formatter.h
        lib/spdlog/logger.h
        lib/spdlog/spdlog.h
        lib/spdlog/tweakme.h
        lib/spdlog/version.h
        lib/spdlog/details/async_logger_impl.h
        lib/spdlog/details/circular_q.h
        lib/spdlog/details/console_globals.h
        lib/spdlog/details/file_helper.h
        lib/spdlog/details/fmt_helper.h
        lib/spdlog/details/log_msg.h
        lib/spdlog/details/logger_impl.h
        lib/spdlog/details/mpmc_blocking_q.h
        lib/spdlog/details/null_mutex.h
        lib/spdlog/details/os.h
        lib/spdlog/details/pattern_formatter.h
        lib/spdlog/details/periodic_worker.h
        lib/spdlog/details/registry.h
        lib/spdlog/details/thread_pool.h
        lib/spdlog/fmt/bin_to_hex.h
        lib/spdlog/fmt/fmt.h
        lib/spdlog/fmt/ostr.h
        lib/spdlog/fmt/bundled/chrono.h
        lib/spdlog/fmt/bundled/color.h
        lib/spdlog/fmt/bundled/core.h
        lib/spdlog/fmt/bundled/format-inl.h
        lib/spdlog/fmt/bundled/format.h
        lib/spdlog/fmt/bundled/locale.h
        lib/spdlog/fmt/bundled/ostream.h
        lib/spdlog/fmt/bundled/posix.h
        lib/spdlog/fmt/bundled/printf.h
        lib/spdlog/fmt/bundled/ranges.h
        lib/spdlog/fmt/bundled/time.h
        lib/spdlog/sinks/android_sink.h
        lib/spdlog/sinks/ansicolor_sink.h
        lib/spdlog/sinks/base_sink.h
        lib/spdlog/sinks/basic_file_sink.h
        lib/spdlog/sinks/daily_file_sink.h
        lib/spdlog/sinks/dist_sink.h
        lib/spdlog/sinks/msvc_sink.h
        lib/spdlog/sinks/null_sink.h
        lib/spdlog/sinks/ostream_sink.h
        lib/spdlog/sinks/rotating_file_sink.h
        lib/spdlog/sinks/sink.h
        lib/spdlog/sinks/stdout_color_sinks.h
        lib/spdlog/sinks/stdout_sinks.h
        lib/spdlog/sinks/wincolor_sink.h
#        lib/cjson/cJSON_inc.h
#        lib/cjson/cJSON_externsion.cpp
        lib/http/HttpRequest.h)

if (APPLE)

endif ()

set(LIBRARY_OUTPUT_PATH ../bin)
set(CMAKE_DEBUG_POSTFIX "d")
IF (CMAKE_BUILD_TYPE MATCHES "Debug")
    ADD_DEFINITIONS(-D_DEBUG)
endif ()

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SOURCE_FILES} )

if (WIN32)
        target_link_libraries(${PROJECT_NAME}
                libcurl libcrypto libssl zlibstat)
else ()

    target_link_libraries(${PROJECT_NAME}
            curl
            z
            ssl
            crypto)
endif ()