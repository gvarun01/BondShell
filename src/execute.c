#include "../include/execute.h"
#include "../utils/shell_utils.h"  // For exitCheck
#include "../utils/string_utils.h"  // For trimstr, getcmd
#include "../utils/error_handler.h" // For handleError (though not directly used, good for consistency)
#include <string.h> // For strcpy, strlen, strtok_r, strdup
#include <stdlib.h> // For malloc, free
#include <sys/time.h> // For gettimeofday (already in global.h but good practice for direct use)


void executeCommand(char *command_to_parse) // Renamed command_got for clarity
{
    if (command_to_parse == NULL) {
        handleError("NULL command passed to executeCommand");
        return;
    }

    // Make a mutable copy as strtok_r will modify it.
    char *cmd_buffer = strdup(command_to_parse);
    if (cmd_buffer == NULL) {
        handleError("strdup failed in executeCommand for cmd_buffer");
        return;
    }

    // Check if the command is intended to be a background process
    // This check is simple; a more robust parser might handle '&' more flexibly (e.g., "cmd & arg")
    bool is_bg_process_suffix = false; // True if the command_to_parse ends with '&'
    if (cmd_buffer[strlen(cmd_buffer) - 1] == '&')
    {
        is_bg_process_suffix = true;
        cmd_buffer[strlen(cmd_buffer) - 1] = '\0'; // Remove the trailing '&'
        trimstr(cmd_buffer); // Trim again in case of "cmd & "
    }

    // cmd_original_content preserves the command after '&' suffix removal, before strtok_r modifies cmd_buffer
    char *cmd_original_content = strdup(cmd_buffer);
    if (cmd_original_content == NULL) {
        handleError("strdup failed for cmd_original_content");
        free(cmd_buffer);
        return;
    }

    char *tokenizer_context; // Renamed x1
    char *token = strtok_r(cmd_buffer, "&", &tokenizer_context); // Use cmd_buffer here

    int num_sequential_bg_commands = 0; // Renamed num_mpercent
    // Count how many commands are separated by '&'
    // This loop consumes tokens from cmd_buffer
    while (token != NULL)
    {
        num_sequential_bg_commands++;
        token = strtok_r(NULL, "&", &tokenizer_context);
    }

    // If there was a trailing '&', it implies one more command segment than counted by strtok_r if it was like "cmd1 & cmd2 &"
    // However, the current strtok_r loop will count "cmd1" and "cmd2" from "cmd1&cmd2&" (after suffix removal).
    // The logic seems to be: if "cmd1 & cmd2", then num_sequential_bg_commands = 2. Last one is FG.
    // If "cmd1 & cmd2 &", then is_bg_process_suffix = true. num_sequential_bg_commands = 2. All are BG.
    // Let's clarify the intent:
    // "cmd" -> FG
    // "cmd &" -> BG (num_sequential_bg_commands = 1, is_bg_process_suffix = true)
    // "cmd1 & cmd2" -> cmd1 BG, cmd2 FG (num_sequential_bg_commands = 2, is_bg_process_suffix = false)
    // "cmd1 & cmd2 &" -> cmd1 BG, cmd2 BG (num_sequential_bg_commands = 2, is_bg_process_suffix = true)

    int actual_num_commands = num_sequential_bg_commands; // How many distinct commands were found

    if (actual_num_commands == 0 && strlen(cmd_original_content) > 0) { // e.g. "   &   " might become empty
        handleError("Invalid command structure with '&'");
        free(cmd_buffer);
        free(cmd_original_content);
        return;
    }
    if (actual_num_commands == 0 && strlen(cmd_original_content) == 0) { // Empty input after processing
        free(cmd_buffer);
        free(cmd_original_content);
        return;
    }


    if (actual_num_commands == 1 && !is_bg_process_suffix) // Single command to run in foreground
    {
        trimstr(cmd_original_content); // cmd_original_content still holds the full command here
        exitCheck(cmd_original_content); // Check if it's "exit"

        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);

        executeInFG(cmd_original_content); // Execute in foreground

        gettimeofday(&end_time, NULL);
        executionTime = (int)((end_time.tv_sec - start_time.tv_sec));

        char *base_command_name = getcmd(cmd_original_content); // Get base command for prompt
        if (base_command_name != NULL) {
            strcpy(executedCommand, base_command_name);
            free(base_command_name); // getcmd allocates memory
        } else {
            strcpy(executedCommand, "Unknown");
        }


        if (executionTime > 2) // Append execution time to prompt if > 2s
        {
            if (strlen(promtExec) > 0) strcat(promtExec, " | ");
            strcat(promtExec, executedCommand); // Use the potentially shortened command name
            char time_buf[32];
            sprintf(time_buf, " : %ds", executionTime); // Corrected sprintf
            strcat(promtExec, time_buf);
        }
    }
    else // Multiple commands separated by '&', or a single command ending with '&'
    {
        // Need to re-tokenize cmd_original_content as cmd_buffer was consumed
        char *current_cmd_token = strtok_r(cmd_original_content, "&", &tokenizer_context);
        int cmd_index = 0;
        while(current_cmd_token != NULL) {
            trimstr(current_cmd_token);
            if (strlen(current_cmd_token) == 0) { // Skip empty commands from "&&" or leading/trailing &
                current_cmd_token = strtok_r(NULL, "&", &tokenizer_context);
                cmd_index++;
                continue;
            }
            exitCheck(current_cmd_token); // Check each part for "exit" - though bg "exit" is weird

            bool run_this_one_in_bg = true;
            if (cmd_index == actual_num_commands - 1 && !is_bg_process_suffix) {
                // This is the last command in a sequence like "cmd1 & cmd2" (not "cmd1 & cmd2 &")
                run_this_one_in_bg = false;
            }

            if (run_this_one_in_bg) {
                bg(current_cmd_token); // Run in background
            } else { // Last command in sequence, run in foreground
                struct timeval start_time, end_time;
                gettimeofday(&start_time, NULL);

                executeInFG(current_cmd_token);

                gettimeofday(&end_time, NULL);
                executionTime = (int)((end_time.tv_sec - start_time.tv_sec));

                char *base_command_name = getcmd(current_cmd_token);
                if (base_command_name != NULL) {
                    strcpy(executedCommand, base_command_name);
                    free(base_command_name);
                } else {
                    strcpy(executedCommand, "Unknown");
                }

                if (executionTime > 2)
                {
                    if (strlen(promtExec) > 0) strcat(promtExec, " | ");
                    strcat(promtExec, executedCommand);
                    char time_buf[32];
                    sprintf(time_buf, " : %ds", executionTime);
                    strcat(promtExec, time_buf);
                }
            }
            current_cmd_token = strtok_r(NULL, "&", &tokenizer_context);
            cmd_index++;
        }
    }
    free(cmd_buffer);
    free(cmd_original_content);
}