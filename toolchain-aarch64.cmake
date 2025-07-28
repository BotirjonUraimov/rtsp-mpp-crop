# Use environment variable from enable_toolkit.sh
set(COMPILER_PREFIX $ENV{GCC_COMPILER})

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER ${COMPILER_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${COMPILER_PREFIX}-g++)

