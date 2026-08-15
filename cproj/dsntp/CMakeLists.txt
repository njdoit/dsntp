cmake_minimum_required(VERSION 3.16)
project(dsntp VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(MSVC)
  add_compile_options(/W3)
else()
  add_compile_options(-Wall -Wextra)
endif()

option(DSNTP_WITH_OPENSSL "Link OpenSSL for ECDSA (M2)" OFF)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(DSNTP_CORE_SOURCES
  lib/src/protocol/codec.c
  lib/src/protocol/payload.c
  lib/src/clock/clock.c
  lib/src/fsm/fsm.c
  lib/src/measure/measure.c
  lib/src/consensus/consensus.c
  lib/src/recover/recover.c
  lib/src/crypto/crypto.c
  lib/src/net/udp.c
  lib/src/config/config.c
)

add_library(dsntp_core STATIC ${DSNTP_CORE_SOURCES})

if(WIN32)
  target_link_libraries(dsntp_core PUBLIC ws2_32)
elseif(UNIX AND NOT APPLE)
  target_link_libraries(dsntp_core PUBLIC m pthread)
endif()

if(DSNTP_WITH_OPENSSL)
  find_package(OpenSSL REQUIRED)
  target_compile_definitions(dsntp_core PUBLIC DSNTP_WITH_OPENSSL=1)
  target_link_libraries(dsntp_core PUBLIC OpenSSL::Crypto)
endif()

# --- apps ---
add_executable(dsntp-agent
  apps/agent/main.c
  apps/agent/agent.c
  apps/agent/local_sock.c
  apps/agent/reporter.c
)
target_link_libraries(dsntp-agent PRIVATE dsntp_core)

add_executable(dsntp-time-shim apps/time-shim/main.c)
target_link_libraries(dsntp-time-shim PRIVATE dsntp_core)

add_executable(dsntp-ctl apps/ctl/main.c)
if(WIN32)
  target_link_libraries(dsntp-ctl PRIVATE ws2_32)
endif()

# --- tests ---
enable_testing()
add_executable(test_smoke tests/test_smoke.c)
target_link_libraries(test_smoke PRIVATE dsntp_core)
add_test(NAME smoke COMMAND test_smoke)

install(TARGETS dsntp-agent dsntp-time-shim dsntp-ctl RUNTIME DESTINATION bin)
install(DIRECTORY include/dsntp DESTINATION include)
