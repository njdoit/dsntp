cmake_minimum_required(VERSION 3.16)
project(dsntp-agent VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(DSNTP_SOURCES
  src/protocol/codec.c
  src/protocol/payload.c
  src/clock/clock.c
  src/fsm/fsm.c
  src/measure/measure.c
  src/consensus/consensus.c
  src/recover/recover.c
  src/crypto/crypto.c
  src/net/udp.c
  src/config/config.c
  src/api/local_sock.c
  src/agent/agent.c
  src/main.c
)

add_executable(dsntp-agent ${DSNTP_SOURCES})

if(WIN32)
  target_link_libraries(dsntp-agent PRIVATE ws2_32)
elseif(UNIX AND NOT APPLE)
  target_link_libraries(dsntp-agent PRIVATE m pthread)
endif()

enable_testing()
add_executable(test_smoke tests/test_smoke.c
  src/protocol/codec.c
  src/fsm/fsm.c
  src/consensus/consensus.c
  src/measure/measure.c
)
if(UNIX)
  target_link_libraries(test_smoke PRIVATE m)
endif()
add_test(NAME smoke COMMAND test_smoke)

install(TARGETS dsntp-agent RUNTIME DESTINATION bin)
