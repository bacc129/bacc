cmake_minimum_required( VERSION 3.5 )

project( BACCChain )

enable_testing()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
   message(WARNING "CMAKE_INSTALL_PREFIX is set to default path of ${CMAKE_INSTALL_PREFIX}, resetting to ${CMAKE_INSTALL_PREFIX}/baccchain")
   set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/baccchain")
elseif ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr/local")
   message(WARNING "CMAKE_INSTALL_PREFIX is explicitly set to /usr/local. This is not recommended.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libraries/fc/CMakeModules")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

if (UNIX)
   if (APPLE)
      if (LLVM_DIR STREQUAL "" OR NOT LLVM_DIR)
         set(LLVM_DIR "/usr/local/opt/llvm@4/lib/cmake/llvm/")
      endif()
   endif()
endif()

include( GNUInstallDirs )
include( VersionMacros )
include( SetupTargetMacros )
include( InstallDirectoryPermissions )
include( MASSigning )

set( BLOCKCHAIN_NAME "BACC" )
set( CMAKE_CXX_STANDARD 14 )
set( CMAKE_CXX_EXTENSIONS ON )
set( CXX_STANDARD_REQUIRED ON)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 1)

if(VERSION_SUFFIX)
    set(VERSION_FULL "bacc.${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_SUFFIX}")
else()
    set(VERSION_FULL "bacc.${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
endif()

set( CLI_CLIENT_EXECUTABLE_NAME baccli )
set( NODE_EXECUTABLE_NAME bacc )
set( KEY_STORE_EXECUTABLE_NAME baccwallet )
set( GENESIS_GEN_EXECUTABLE_NAME baccgenesis )
set( CUSTOM_URL_SCHEME "gcs" )
set( INSTALLER_APP_ID "68ad7005-8eee-49c9-95ce-9eed97e5b347" )

# http://stackoverflow.com/a/18369825
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
        message(FATAL_ERROR "GCC version must be at least 6.0!")
    endif()
    if ("${CMAKE_GENERATOR}" STREQUAL "Ninja")
       add_compile_options(-fdiagnostics-color=always)
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.0)
        message(FATAL_ERROR "Clang version must be at least 4.0!")
    endif()
    if ("${CMAKE_GENERATOR}" STREQUAL "Ninja")
       add_compile_options(-fcolor-diagnostics)
    endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
set(BUILD_DOXYGEN FALSE CACHE BOOL "Build doxygen documentation on every make")
set(BUILD_MONGO_DB_PLUGIN FALSE CACHE BOOL "Build mongo database plugin")

#set (USE_PCH 1)

if (USE_PCH)
  include (cotire)
endif(USE_PCH)

# add defaults for openssl
if ("${OPENSSL_ROOT_DIR}" STREQUAL "")
   if (NOT "$ENV{OPENSSL_ROOT_DIR}" STREQUAL "")
      set(OPENSSL_ROOT_DIR $ENV{OPENSSL_ROOT_DIR})
      set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/include)
   elseif (APPLE)
      set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
      set(OPENSSL_INCLUDE_DIR "/usr/local/opt/openssl/include")
   elseif(UNIX AND NOT APPLE)
      set(OPENSSL_ROOT_DIR "/usr/include/openssl")
      set(OPENSSL_INCLUDE_DIR "/usr/include/openssl/include")
   else()
      message(FATAL_ERROR "openssl not found and don't know where to look, please specify OPENSSL_ROOT_DIR")
   endif()
endif()

if(UNIX)
  if(APPLE)
    set(whole_archive_flag "-force_load")
    set(no_whole_archive_flag "")
  else()
    set(whole_archive_flag "--whole-archive")
    set(no_whole_archive_flag "--no-whole-archive")
  endif()
else()
  set(whole_archive_flag "--whole-archive")
  set(no_whole_archive_flag "--no-whole-archive")
endif()

SET( Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF" )
IF( WIN32 )
  SET(BOOST_ROOT $ENV{BOOST_ROOT})
  set(Boost_USE_MULTITHREADED ON)
  set(BOOST_ALL_DYN_LINK OFF) # force dynamic linking for all libraries
ENDIF(WIN32)
FIND_PACKAGE(Boost 1.67 REQUIRED COMPONENTS
    thread
    date_time
    filesystem
    system
    program_options
    signals
    serialization
    chrono
    unit_test_framework
    context
    locale
    iostreams)

if( WIN32 )

    message( STATUS "Configuring baccchain on WIN32")
    set( DB_VERSION 60 )
    set( BDB_STATIC_LIBS 1 )

    set( ZLIB_LIBRARIES "" )
    SET( DEFAULT_EXECUTABLE_INSTALL_DIR bin/ )

    set(CRYPTO_LIB)

    #looks like this flag can have different default on some machines.
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")

    # Probably cmake has a bug and vcxproj generated for executable in Debug conf. has disabled debug info
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")

    # On windows tcl should be installed to the directory pointed by setenv.bat script
    SET(TCL_INCLUDE_PATH $ENV{TCL_ROOT}/include)
    MESSAGE(STATUS "tcl INCLUDE PATH: ${TCL_INCLUDE_PATH}")

    FIND_PACKAGE(TCL)
    MESSAGE(STATUS "tcl_library: ${TCL_LIBRARY}")

    SET(TCL_LIBS "optimized;${TCL_LIBRARY};debug;")
    get_filename_component(TCL_LIB_PATH "${TCL_LIBRARY}" PATH)
    get_filename_component(TCL_LIB_NAME "${TCL_LIBRARY}" NAME_WE)
    get_filename_component(TCL_LIB_EXT "${TCL_LIBRARY}" EXT)

    SET(TCL_LIBS "${TCL_LIBS}${TCL_LIB_PATH}/${TCL_LIB_NAME}g${TCL_LIB_EXT}")
    SET(TCL_LIBRARY ${TCL_LIBS})

else( WIN32 ) # Apple AND Linux

    if( APPLE )
        # Apple Specific Options Here
        message( STATUS "Configuring baccchain on OS X" )
        set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-deprecated-declarations" )
    else( APPLE )
        # Linux Specific Options Here
        message( STATUS "Configuring baccchain on Linux" )
        set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wall" )
        if ( FULL_STATIC_BUILD )
          set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
        endif ( FULL_STATIC_BUILD )

        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")


        if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
            if( CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.0.0 OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.0.0 )
                set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-partial-specialization" )
            endif()
        endif()
    endif( APPLE )

    if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp" )
    endif()

    if( "${CMAKE_GENERATOR}" STREQUAL "Ninja" )
        if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics" )
        endif()
    endif()

    # based on http://www.delorie.com/gnu/docs/gdb/gdb_70.html
    # uncomment this line to tell GDB about macros (slows compile times)
    # set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-2 -g3" )

endif( WIN32 )

set(ENABLE_COVERAGE_TESTING FALSE CACHE BOOL "Build baccchain for code coverage analysis")

if(ENABLE_COVERAGE_TESTING)
  SET(CMAKE_CXX_FLAGS "--coverage ${CMAKE_CXX_FLAGS}")
  find_program( LCOV_PATH  lcov )
  find_program( LLVMCOV_PATH  llvm-cov )
  find_program( GENHTML_PATH NAMES genhtml)
endif()

include(utils)
add_subdirectory( externals )

if ("${CORE_SYMBOL_NAME}" STREQUAL "")
  set( CORE_SYMBOL_NAME "BACC" )
endif()
string(TOUPPER ${CORE_SYMBOL_NAME} CORE_SYMBOL_NAME)

# set(USE_PUB_KEY_LEGACY_PREFIX 1)

if ("${PUB_KEY_LEGACY_PREFIX}" STREQUAL "")
    set(PUB_KEY_LEGACY_PREFIX "BACC")
endif()
string(TOUPPER ${PUB_KEY_LEGACY_PREFIX} PUB_KEY_LEGACY_PREFIX)
message( STATUS "Using '${PUB_KEY_LEGACY_PREFIX}' as pubkey legacy prefix" )

if ("${PUB_KEY_BASE_PREFIX}" STREQUAL "")
    set(PUB_KEY_BASE_PREFIX "PUB")
endif()
string(TOUPPER ${PUB_KEY_BASE_PREFIX} PUB_KEY_BASE_PREFIX)
message( STATUS "Using '${PUB_KEY_BASE_PREFIX}' as pubkey common prefix" )

if (USE_PUB_KEY_LEGACY_PREFIX)
    set(USE_PUB_KEY_LEGACY_PREFIX 1)
    message( STATUS "Using legacy prefix for public or private key" )
else()
    set(USE_PUB_KEY_LEGACY_PREFIX 0)
    message( STATUS "Using common prefix for public or private key" )
endif(USE_PUB_KEY_LEGACY_PREFIX)

string(LENGTH ${CORE_SYMBOL_NAME} CORE_SYMBOL_NAME_LENGTH)
if (CORE_SYMBOL_NAME_LENGTH GREATER 7)
  message(FATAL_ERROR "CORE_SYMBOL_NAME length must be between 1 and 7 characters")
endif()

message( STATUS "Using '${CORE_SYMBOL_NAME}' as CORE symbol name" )

if ("${SYSTEM_ACCOUNT_ROOT_KEY}" STREQUAL "")
   set(SYSTEM_ACCOUNT_ROOT_KEY "${PUB_KEY_LEGACY_PREFIX}5muUziYrETi5b6G2Ev91dCBrEm3qir7PK4S2qSFqfqcmouyzCr")
endif()

message( STATUS "Using '${SYSTEM_ACCOUNT_ROOT_KEY}' as public key for system root account" )

if ("${CHAIN_INIT_TIMESTAMP}" STREQUAL "")
    set(CHAIN_INIT_TIMESTAMP "2019-05-04T12:00:00")
endif()

message( STATUS "Using '${CHAIN_INIT_TIMESTAMP}' as chain init timestamp" )

if ("${ROOT_ACCOUNT}" STREQUAL "")
    set(ROOT_ACCOUNT "force")
endif()

message( STATUS "Using '${ROOT_ACCOUNT}' as root account" )

#add contract_config
if ("${FROZEN_DELAY}" STREQUAL "")
  set( FROZEN_DELAY "7 * 24 * 60 * 20" )
endif()
if ("${MAX_PRODUCERS}" STREQUAL "")
  set( MAX_PRODUCERS "23" )
endif()
if ("${BLOCK_INTERVAL_MS}" STREQUAL "")
  set( BLOCK_INTERVAL_MS "3000" )
endif()
if ("${PRODUCER_REPETITIONS}" STREQUAL "")
  set( PRODUCER_REPETITIONS "1" )
endif()
if ("${BLOCK_REWARDS_BP}" STREQUAL "")
  set( BLOCK_REWARDS_BP "5 * 1000" )
endif()
if ("${UPDATE_CYCLE}" STREQUAL "")
  set( UPDATE_CYCLE "100" )
endif()
if ("${RESOURCE_MODEL}" STREQUAL "")
  set( RESOURCE_MODEL "1" )
endif()

# is chain active multiple vote
if(USE_MULTIPLE_VOTE)
    message( STATUS "Using multiple vote for chain" )
    set(IS_ACTIVE_MULTIPLE_VOTE 1)
else()
    set(IS_ACTIVE_MULTIPLE_VOTE 0)
endif(USE_MULTIPLE_VOTE)

# is chain active bonus to vote
if(USE_BONUS_TO_VOTE)
    message( STATUS "Using bonus to vote for chain" )
    set(IS_ACTIVE_BONUS_TO_VOTE 1)
else()
    set(IS_ACTIVE_BONUS_TO_VOTE 0)
endif(USE_BONUS_TO_VOTE)

include(wasm)
add_subdirectory( libraries )
add_subdirectory( contracts )
add_subdirectory( plugins )
add_subdirectory( programs )

install_directory_permissions(DIRECTORY ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio)

install(FILES testnet.template DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio/launcher)
install_directory_permissions(DIRECTORY ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio)
install_directory_permissions(DIRECTORY ${CMAKE_INSTALL_FULL_SYSCONFDIR}/eosio/launcher)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/chain.version.in ${CMAKE_CURRENT_BINARY_DIR}/eosio.version.hpp)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/eosio.version.hpp DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR})

set(EOS_ROOT_DIR ${CMAKE_BINARY_DIR})
configure_file(${CMAKE_SOURCE_DIR}/CMakeModules/eosio-config.cmake.in ${CMAKE_BINARY_DIR}/lib/cmake/eosio/eosio-config.cmake @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/CMakeModules/EosioTesterBuild.cmake.in ${CMAKE_BINARY_DIR}/lib/cmake/eosio/EosioTester.cmake @ONLY)

set(EOS_ROOT_DIR ${CMAKE_INSTALL_PREFIX})
configure_file(${CMAKE_SOURCE_DIR}/CMakeModules/eosio-config.cmake.in ${CMAKE_BINARY_DIR}/modules/eosio-config.cmake @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/modules/eosio-config.cmake DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/cmake/eosio)
configure_file(${CMAKE_SOURCE_DIR}/CMakeModules/EosioTester.cmake.in ${CMAKE_BINARY_DIR}/modules/EosioTester.cmake @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/modules/EosioTester.cmake DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/cmake/eosio)
