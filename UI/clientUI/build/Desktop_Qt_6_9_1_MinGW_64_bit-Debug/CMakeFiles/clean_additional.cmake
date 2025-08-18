# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\EmailRemoteControl_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\EmailRemoteControl_autogen.dir\\ParseCache.txt"
  "EmailRemoteControl_autogen"
  )
endif()
