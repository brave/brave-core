
/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/tools/redirect_cc/os_utils.h"

#include <Windows.h>

#include "brave/tools/redirect_cc/logging.h"
#include "brave/tools/redirect_cc/utils.h"

namespace os_utils {

namespace {

// Quote a string as necessary for CommandLineToArgvW compatibility.
bool QuoteForCommandLineToArgvW(const FilePathString& arg,
                                FilePathString* out) {
  // We follow the quoting rules of CommandLineToArgvW.
  // http://msdn.microsoft.com/en-us/library/17w5ykft.aspx
  FilePathString quotable_chars(L" \\\"");
  if (arg.find_first_of(quotable_chars) == FilePathString::npos) {
    // No quoting necessary.
    return false;
  }

  out->clear();
  out->push_back('"');
  for (size_t i = 0; i < arg.size(); ++i) {
    if (arg[i] == '\\') {
      // Find the extent of this run of backslashes.
      size_t start = i, end = start + 1;
      for (; end < arg.size() && arg[end] == '\\'; ++end) {
      }
      size_t backslash_count = end - start;

      // Backslashes are escapes only if the run is followed by a double
      // quote. Since we also will end the string with a double quote, we
      // escape for either a double quote or the end of the string.
      if (end == arg.size() || arg[end] == '"') {
        // To quote, we need to output 2x as many backslashes.
        backslash_count *= 2;
      }
      for (size_t j = 0; j < backslash_count; ++j)
        out->push_back('\\');

      // Advance i to one before the end to balance i++ in loop.
      i = end - 1;
    } else if (arg[i] == '"') {
      out->push_back('\\');
      out->push_back('"');
    } else {
      out->push_back(arg[i]);
    }
  }
  out->push_back('"');
  return true;
}

FilePathString CreateCmdLine(const std::vector<FilePathString>& argv) {
  FilePathString cmd_line;
  FilePathString quotted_arg;
  quotted_arg.reserve(1024);
  for (const auto& arg : argv) {
    const FilePathString& arg_to_append =
        QuoteForCommandLineToArgvW(arg, &quotted_arg) ? quotted_arg : arg;
    if (!cmd_line.empty()) {
      cmd_line += L" ";
    }
    cmd_line += arg_to_append;
  }
  return cmd_line;
}

}  // namespace

int LaunchProcessAndWaitForExitCode(const std::vector<FilePathString>& argv) {
  FilePathString cmd_line = CreateCmdLine(argv);
  DWORD err = 0;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  if (!CreateProcessW(NULL,             // No module name (use command line)
                      cmd_line.data(),  // Command line
                      NULL,             // Process handle not inheritable
                      NULL,             // Thread handle not inheritable
                      TRUE,             // Set handle inheritance to TRUE
                      0,                // No creation flags
                      NULL,             // Use parent's environment block
                      NULL,             // Use parent's starting directory
                      &si,              // Pointer to STARTUPINFO structure
                      &pi)  // Pointer to PROCESS_INFORMATION structure
  ) {
    err = GetLastError();
    LOG("CreateProcess failed (" << err << ")");
    return err;
  }

  // Wait until child process exits.
  err = WaitForSingleObject(pi.hProcess, INFINITE);
  if (err != WAIT_OBJECT_0) {
    if (err == WAIT_FAILED) {
      err = GetLastError();
    }
    LOG("Waiting for process to exit failed (" << err << ")");
  } else {
    if (!GetExitCodeProcess(pi.hProcess, &err)) {
      err = GetLastError();
      LOG("Failed to get process exit code (" << err << "");
    }
  }

  // Close process and thread handles.
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return err;
}

bool PathExists(FilePathStringView path) {
  return (GetFileAttributes(path.data()) != INVALID_FILE_ATTRIBUTES);
}

bool GetEnvVar(FilePathStringView variable_name, FilePathString* result) {
  DWORD value_length =
      ::GetEnvironmentVariable(variable_name.data(), nullptr, 0);
  if (value_length == 0)
    return false;
  if (result) {
    std::unique_ptr<wchar_t[]> value(new wchar_t[value_length]);
    ::GetEnvironmentVariable(variable_name.data(), value.get(), value_length);
    *result = value.get();
  }
  return true;
}

}  // namespace os_utils
