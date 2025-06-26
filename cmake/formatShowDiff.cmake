# formatShowDiff.cmake - Show formatting diff in console AND save to file

file(GLOB_RECURSE ALL_SOURCE_FILES 
    ${SOURCE_DIR}/*.cpp
    ${SOURCE_DIR}/*.h
    ${SOURCE_DIR}/*.hpp
    ${SOURCE_DIR}/*.inl
)

# Exclude build directories and external dependencies
list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX "${SOURCE_DIR}/build/.*")
list(FILTER ALL_SOURCE_FILES EXCLUDE REGEX "${SOURCE_DIR}/_deps/.*")

set(DIFF_FOUND FALSE)
set(FILES_WITH_ISSUES 0)
set(DIFF_FILE "${CMAKE_BINARY_DIR}/formatting.diff")

# Clear the diff file
file(WRITE ${DIFF_FILE} "")

message("🔍 Checking formatting differences...")
message("=====================================")

foreach(SOURCE_FILE ${ALL_SOURCE_FILES})
    # Get original content
    file(READ ${SOURCE_FILE} ORIGINAL_CONTENT)
    
    # Get formatted content
    execute_process(
        COMMAND ${CLANG_FORMAT_EXE} ${SOURCE_FILE}
        OUTPUT_VARIABLE FORMATTED_CONTENT
        RESULT_VARIABLE FORMAT_RESULT
        WORKING_DIRECTORY ${SOURCE_DIR}
    )
    
    if(FORMAT_RESULT EQUAL 0)
        # Compare content
        if(NOT "${ORIGINAL_CONTENT}" STREQUAL "${FORMATTED_CONTENT}")
            set(DIFF_FOUND TRUE)
            math(EXPR FILES_WITH_ISSUES "${FILES_WITH_ISSUES} + 1")
            
            message("")
            message("📝 File: ${SOURCE_FILE}")
            message("   Formatting differences detected")
            
            # Generate detailed diff for file
            file(WRITE ${CMAKE_BINARY_DIR}/temp_original.tmp "${ORIGINAL_CONTENT}")
            file(WRITE ${CMAKE_BINARY_DIR}/temp_formatted.tmp "${FORMATTED_CONTENT}")
            
            # Try git diff first, then fallback
            find_program(GIT_TOOL NAMES "git")
            if(GIT_TOOL)
                execute_process(
                    COMMAND git diff --no-index --no-prefix 
                        ${CMAKE_BINARY_DIR}/temp_original.tmp 
                        ${CMAKE_BINARY_DIR}/temp_formatted.tmp
                    OUTPUT_VARIABLE DETAILED_DIFF
                    ERROR_QUIET
                )
                
                # Show first few lines of diff in console
                string(REPLACE "\n" ";" DIFF_LINES "${DETAILED_DIFF}")
                list(LENGTH DIFF_LINES TOTAL_LINES)
                
                if(TOTAL_LINES GREATER 0)
                    message("   📋 Preview (first 10 lines):")
                    set(LINE_COUNT 0)
                    foreach(DIFF_LINE ${DIFF_LINES})
                        if(LINE_COUNT LESS 10 AND NOT "${DIFF_LINE}" STREQUAL "")
                            # Color coding for console
                            if("${DIFF_LINE}" MATCHES "^\\+.*")
                                message("   ✅ ${DIFF_LINE}")
                            elseif("${DIFF_LINE}" MATCHES "^-.*")
                                message("   ❌ ${DIFF_LINE}")
                            elseif("${DIFF_LINE}" MATCHES "^@@.*")
                                message("   🔹 ${DIFF_LINE}")
                            else()
                                message("      ${DIFF_LINE}")
                            endif()
                            math(EXPR LINE_COUNT "${LINE_COUNT} + 1")
                        endif()
                    endforeach()
                    
                    if(TOTAL_LINES GREATER 10)
                        math(EXPR REMAINING "${TOTAL_LINES} - 10")
                        message("   ... and ${REMAINING} more lines (see diff file)")
                    endif()
                endif()
            else()
                set(DETAILED_DIFF "=== Diff not available (git not found) ===\n")
                message("   ⚠️  Install git for detailed diff preview")
            endif()
            
            # Save to diff file
            file(APPEND ${DIFF_FILE} "=== ${SOURCE_FILE} ===\n")
            file(APPEND ${DIFF_FILE} "${DETAILED_DIFF}\n\n")
            
            # Show size info in console
            string(LENGTH "${ORIGINAL_CONTENT}" ORIG_LEN)
            string(LENGTH "${FORMATTED_CONTENT}" FORM_LEN)
            message("   📊 Original size: ${ORIG_LEN} chars")
            message("   📊 Formatted size: ${FORM_LEN} chars")
            
            # Clean temp files
            file(REMOVE ${CMAKE_BINARY_DIR}/temp_original.tmp)
            file(REMOVE ${CMAKE_BINARY_DIR}/temp_formatted.tmp)
        endif()
    else()
        message("❌ Error formatting: ${SOURCE_FILE}")
    endif()
endforeach()

message("")
message("=====================================")
if(DIFF_FOUND)
    message("📊 Summary: ${FILES_WITH_ISSUES} file(s) need formatting")
    message("💡 Run 'cmake --build . --target FORMAT' to apply changes")
    message("📄 Detailed diff saved to: ${DIFF_FILE}")
    
    # Show diff file size
    get_filename_component(DIFF_SIZE ${DIFF_FILE} SIZE)
    message("📊 Diff file size: ${DIFF_SIZE} bytes")
    
    # Quick stats about changes
    file(READ ${DIFF_FILE} FULL_DIFF)
    string(REGEX MATCHALL "\\+[^\n]*" ADDITIONS "${FULL_DIFF}")
    string(REGEX MATCHALL "-[^\n]*" DELETIONS "${FULL_DIFF}")
    list(LENGTH ADDITIONS ADD_COUNT)
    list(LENGTH DELETIONS DEL_COUNT)
    message("📈 Changes: +${ADD_COUNT} additions, -${DEL_COUNT} deletions")
else()
    file(WRITE ${DIFF_FILE} "✅ All files are properly formatted!\n")
    message("✅ All files are properly formatted!")
    message("📄 Status saved to: ${DIFF_FILE}")
endif()
message("=====================================")

# Exit with error code if differences found (for CI/CD)
if(DIFF_FOUND)
    message(FATAL_ERROR "Formatting check failed")
endif()