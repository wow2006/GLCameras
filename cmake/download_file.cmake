# ${}/cmake/download_file.cmake

function(download_file FILE_NAME URL MD5)
  if(EXISTS "${FILE_NAME}")
    message("Using ${FILE_NAME} cached.")
    return()
  endif()

  file(
    DOWNLOAD
    ${URL}
    ${FILE_NAME}
    EXPECTED_MD5 ${MD5}
  )
endfunction()
