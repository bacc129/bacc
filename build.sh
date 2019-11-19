#!/bin/bash

   SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

   function usage()
   {
      printf "\\tUsage: %s \\n\\t[Build Option -o <Debug|Release|RelWithDebInfo|MinSizeRel>] \\n\\t[CodeCoverage -c] \\n\\t[Doxygen -d] \\n\\t[CoreSymbolName -s <1-7 characters>] \\n\\t[ResourceModel -r <Unlimit|Fee|Delegate>] \\n\\t[Avoid Compiling -a]\\n\\n" "$0" 1>&2
      exit 1
   }

   is_noninteractive() {
      [[ -n "${CHAIN_BUILD_NONINTERACTIVE+1}" ]]
   }

   ARCH=$( uname )
   if [ "${SOURCE_DIR}" == "${PWD}" ]; then
      BUILD_DIR="${PWD}/build"
   else
      BUILD_DIR="${PWD}"
   fi
   CMAKE_BUILD_TYPE=Release
   DISK_MIN=10
   DOXYGEN=false
   ENABLE_COVERAGE_TESTING=false
   CORE_SYMBOL_NAME="BACC"
   ROOT_ACCOUNT="bacc"
   USE_PUB_KEY_LEGACY_PREFIX=1
   MAX_PRODUCERS=17
   BLOCK_INTERVAL_MS=500
   PRODUCER_REPETITIONS=6
   RESOURCE_MODEL=2

   # if chain use bonus to vote
   USE_BONUS_TO_VOTE=1

   # Use current directory's tmp directory if noexec is enabled for /tmp
   if (mount | grep "/tmp " | grep --quiet noexec); then
        mkdir -p $SOURCE_DIR/tmp
        TEMP_DIR="${SOURCE_DIR}/tmp"
        rm -rf $SOURCE_DIR/tmp/*
   else # noexec wasn't found
        TEMP_DIR="/tmp"
   fi
   START_MAKE=true
   TIME_BEGIN=$( date -u +%s )
   VERSION=1.2

   txtbld=$(tput bold)
   bldred=${txtbld}$(tput setaf 1)
   txtrst=$(tput sgr0)

   if [ $# -ne 0 ]; then
      while getopts ":cdo:s:r:ah" opt; do
         case "${opt}" in
            o )
               options=( "Debug" "Release" "RelWithDebInfo" "MinSizeRel" )
               if [[ "${options[*]}" =~ "${OPTARG}" ]]; then
                  CMAKE_BUILD_TYPE="${OPTARG}"
               else
                  printf "\\n\\tInvalid argument: %s\\n" "${OPTARG}" 1>&2
                  usage
                  exit 1
               fi
            ;;
            c )
               ENABLE_COVERAGE_TESTING=true
            ;;
            d )
               DOXYGEN=true
            ;;
            s)
               if [ "${#OPTARG}" -gt 7 ] || [ -z "${#OPTARG}" ]; then
                  printf "\\n\\tInvalid argument: %s\\n" "${OPTARG}" 1>&2
                  usage
                  exit 1
               else
                  CORE_SYMBOL_NAME="${OPTARG}"
               fi
            ;;
            r )
               options=( "Unlimit" "Fee" "Delegate" )
               if [[ "${options[*]}" =~ "${OPTARG}" ]]; then
                  if [[ "${OPTARG}" == "Unlimit" ]]; then
                    RESOURCE_MODEL=0
                  fi
                  if [[ "${OPTARG}" == "Fee" ]]; then
                    RESOURCE_MODEL=1
                  fi
                  if [[ "${OPTARG}" == "Delegate" ]]; then
                    RESOURCE_MODEL=2
                  fi
               else
                  printf "\\n\\tInvalid argument: %s\\n" "${OPTARG}" 1>&2
                  usage
                  exit 1
               fi
            ;;
            a)
               START_MAKE=false
            ;;
            h)
               usage
               exit 1
            ;;
            y)
               CHAIN_BUILD_NONINTERACTIVE=1
            ;;
            \? )
               printf "\\n\\tInvalid Option: %s\\n" "-${OPTARG}" 1>&2
               usage
               exit 1
            ;;
            : )
               printf "\\n\\tInvalid Option: %s requires an argument.\\n" "-${OPTARG}" 1>&2
               usage
               exit 1
            ;;
            * )
               usage
               exit 1
            ;;
         esac
      done
   fi

   pushd "${SOURCE_DIR}" &> /dev/null

   printf "\\n\\tBeginning build version: %s\\n" "${VERSION}"
   printf "\\t%s\\n" "$( date -u )"
   printf "\\tUser: %s\\n" "$( whoami )"
   #printf "\\tgit head id: %s\\n" "$( cat .git/refs/heads/master )"
#   printf "\\tCurrent branch: %s\\n" "$( git rev-parse --abbrev-ref HEAD )"
   printf "\\n\\tARCHITECTURE: %s\\n" "${ARCH}"

   popd &> /dev/null

   if [ "$ARCH" == "Linux" ]; then

      if [ ! -e /etc/os-release ]; then
         printf "\\n\\tcurrently supports Centos, Ubuntu Linux only.\\n"
         printf "\\tPlease install on the latest version of one of these Linux distributions.\\n"
         printf "\\tExiting now.\\n"
         exit 1
      fi

      OS_NAME=$( cat /etc/os-release | grep ^NAME | cut -d'=' -f2 | sed 's/\"//gI' )

      case "$OS_NAME" in
         "Ubuntu")
            FILE="${SOURCE_DIR}/scripts/build_ubuntu.sh"
            CXX_COMPILER=clang++-4.0
            C_COMPILER=clang-4.0
            MONGOD_CONF=${HOME}/bacc/opt/mongodb/mongod.conf
            export PATH=${HOME}/bacc/opt/mongodb/bin:$PATH
         ;;
         *)
            printf "\\n\\tUnsupported Linux Distribution. Exiting now.\\n\\n"
            exit 1
      esac

      export BOOST_ROOT="${HOME}/bacc/opt/boost"
      WASM_ROOT="${HOME}/bacc/opt/wasm"
      OPENSSL_ROOT_DIR=/usr/include/openssl
   fi

   if [ "$ARCH" == "Darwin" ]; then
      FILE="${SOURCE_DIR}/scripts/build_darwin.sh"
      CXX_COMPILER=clang++
      C_COMPILER=clang
      MONGOD_CONF=/usr/local/etc/mongod.conf
      OPENSSL_ROOT_DIR=/usr/local/opt/openssl
   fi

   . "$FILE"

   printf "\\n\\n>>>>>>>> ALL dependencies sucessfully found or installed .\\n\\n"
   printf ">>>>>>>> CMAKE_BUILD_TYPE=%s\\n" "${CMAKE_BUILD_TYPE}"
   printf ">>>>>>>> ENABLE_COVERAGE_TESTING=%s\\n" "${ENABLE_COVERAGE_TESTING}"
   printf ">>>>>>>> DOXYGEN=%s\\n" "${DOXYGEN}"
   printf ">>>>>>>> RESOURCE_MODEL=%s\\n\\n" "${RESOURCE_MODEL}"

   if [ ! -d "${BUILD_DIR}" ]; then
      if ! mkdir -p "${BUILD_DIR}"
      then
         printf "Unable to create build directory %s.\\n Exiting now.\\n" "${BUILD_DIR}"
         exit 1;
      fi
   fi

   if ! cd "${BUILD_DIR}"
   then
      printf "Unable to enter build directory %s.\\n Exiting now.\\n" "${BUILD_DIR}"
      exit 1;
   fi

   if [ -z "$CMAKE" ]; then
      CMAKE=$( command -v cmake )
   fi

echo "-DCMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}' "
echo "-DCMAKE_CXX_COMPILER='${CXX_COMPILER}'"
echo "-DCMAKE_C_COMPILER='${C_COMPILER}' "
echo "-DWASM_ROOT='${WASM_ROOT}' "
echo "-DCORE_SYMBOL_NAME='${CORE_SYMBOL_NAME}'"
echo "-DUSE_PUB_KEY_LEGACY_PREFIX=${USE_PUB_KEY_LEGACY_PREFIX}"
echo "-DUSE_MULTIPLE_VOTE=${USE_MULTIPLE_VOTE}"
echo "-DROOT_ACCOUNT='${ROOT_ACCOUNT}'"
echo "-DMAX_PRODUCERS='${MAX_PRODUCERS}' "
echo "-DBLOCK_INTERVAL_MS='${BLOCK_INTERVAL_MS}' "
echo "-DPRODUCER_REPETITIONS='${PRODUCER_REPETITIONS}'"
echo "-DRESOURCE_MODEL=${RESOURCE_MODEL}"
echo "-DOPENSSL_ROOT_DIR='${OPENSSL_ROOT_DIR}' "
echo "-DBUILD_MONGO_DB_PLUGIN=true"
echo "-DENABLE_COVERAGE_TESTING='${ENABLE_COVERAGE_TESTING}' "
echo "-DBUILD_DOXYGEN='${DOXYGEN}'"
echo "-DCMAKE_INSTALL_PREFIX='/usr/local/baccchain'"
echo ${LOCAL_CMAKE_FLAGS}

   if ! "${CMAKE}" -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" -DCMAKE_CXX_COMPILER="${CXX_COMPILER}" \
      -DCMAKE_C_COMPILER="${C_COMPILER}" -DWASM_ROOT="${WASM_ROOT}" -DCORE_SYMBOL_NAME="${CORE_SYMBOL_NAME}" \
      -DUSE_PUB_KEY_LEGACY_PREFIX=${USE_PUB_KEY_LEGACY_PREFIX} \
      -DUSE_MULTIPLE_VOTE=${USE_MULTIPLE_VOTE} \
      -DROOT_ACCOUNT="${ROOT_ACCOUNT}" \
      -DMAX_PRODUCERS="${MAX_PRODUCERS}" -DBLOCK_INTERVAL_MS="${BLOCK_INTERVAL_MS}" -DPRODUCER_REPETITIONS="${PRODUCER_REPETITIONS}" \
      -DRESOURCE_MODEL=${RESOURCE_MODEL} \
      -DOPENSSL_ROOT_DIR="${OPENSSL_ROOT_DIR}" -DBUILD_MONGO_DB_PLUGIN=true \
      -DENABLE_COVERAGE_TESTING="${ENABLE_COVERAGE_TESTING}" -DBUILD_DOXYGEN="${DOXYGEN}" \
      -DCMAKE_INSTALL_PREFIX="/usr/local/baccchain" ${LOCAL_CMAKE_FLAGS} "${SOURCE_DIR}"
   then
      printf "\\n\\t>>>>>>>>>>>>>>>>>>>> CMAKE building BACC-Chain has exited with the above error.\\n\\n"
      exit -1
   fi

   if [ "${START_MAKE}" == "false" ]; then
      printf "\\n\\t>>>>>>>>>>>>>>>>>>>> BACC-Chain has been successfully configured but not yet built.\\n\\n"
      exit 0
   fi

   if [ -z ${JOBS} ]; then JOBS=$CPU_CORE; fi # Future proofing: Ensure $JOBS is set (usually set in scripts/build_*.sh scripts)
   if ! make -j"${JOBS}"
   then
      printf "\\n\\t>>>>>>>>>>>>>>>>>>>> MAKE building BACC-Chain has exited with the above error.\\n\\n"
      exit -1
   fi
