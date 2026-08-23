if(NOT AR OR NOT MRI)
  message(FATAL_ERROR "usage: cmake -DAR=<archiver> -DMRI=<script> -P merge-archives.cmake")
endif()
execute_process(
  COMMAND "${AR}" -M
  INPUT_FILE "${MRI}"
  COMMAND_ERROR_IS_FATAL ANY
)
