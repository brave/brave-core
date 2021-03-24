/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// clang-format off
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include <memory>
#include <string>
// clang-format on

// Determines verbosity of output based on the value of the environmental
// variable BRAVE_BUILD_SHOW_REDIRECT_CC_CMD (set to 1 for verbose).
BOOL IsVerbose() {
  wchar_t buffer[2];
  DWORD dwRet =
      GetEnvironmentVariableW(L"BRAVE_BUILD_SHOW_REDIRECT_CC_CMD", buffer, 2);
  if (dwRet > 2) {
    wprintf(
        L"REDIRECT-CC: BRAVE_BUILD_SHOW_REDIRECT_CC_CMD env. var. is expected "
        L"to be either 0 or 1.\n");
    return FALSE;
  } else if (dwRet == 0) {
    dwRet = GetLastError();
    if (dwRet != ERROR_ENVVAR_NOT_FOUND) {
      wprintf(
          L"REDIRECT-CC: Error getting env. var. "
          L"BRAVE_BUILD_SHOW_REDIRECT_CC_CMD (%d).\n",
          dwRet);
    }
    return FALSE;
  } else if (wcsncmp(buffer, L"0", 1) != 0) {
    return TRUE;
  }
  return FALSE;
}

// Escapes double quotes in the arg and wraps the entire arg in double quotes.
const std::wstring EscapeArg(const wchar_t* arg) {
  std::wstring str(L"\"");
  str.append(arg);
  size_t pos = 1;
  while ((pos = str.find(L"\"", pos)) != std::string::npos) {
    str.replace(pos, 1, L"\\\"");
    pos += 2;
  }
  str.append(L"\"");
  return str;
}

// Calls python executable from search path and passes it redirect-cc.py script
// and all args passed to this exe.
int wmain(int argc, wchar_t* argv[]) {
  if (argc < 2) {
    wprintf(L"Usage: %s [cmdline_for_redirect_cc]\n", argv[0]);
    return 1;
  }

  // Build the new command line that calls python with our script.
  std::wstring cmd_line_builder =
      L"python ..\\..\\brave\\script\\redirect-cc.py";
  for (int i = 1; i < argc; i++) {
    cmd_line_builder.append(_T(" "));
    // Double-quote escape args as they may have spaces.
    cmd_line_builder.append(EscapeArg(argv[i]));
  }

  // CreateProcess requires a mutable command line string.
  const auto len = cmd_line_builder.length() + 1;
  auto cmd_line = std::make_unique<TCHAR[]>(len);
  wcscpy_s(cmd_line.get(), len, cmd_line_builder.c_str());

  if (IsVerbose()) {
    wprintf(
        L"----------------------------------------------\n"
        L"%s\n"
        L"----------------------------------------------\n",
        cmd_line.get());
  }

  DWORD err = 0;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  if (!CreateProcessW(NULL,            // No module name (use command line)
                      cmd_line.get(),  // Command line
                      NULL,            // Process handle not inheritable
                      NULL,            // Thread handle not inheritable
                      TRUE,            // Set handle inheritance to TRUE
                      0,               // No creation flags
                      NULL,            // Use parent's environment block
                      NULL,            // Use parent's starting directory
                      &si,             // Pointer to STARTUPINFO structure
                      &pi)  // Pointer to PROCESS_INFORMATION structure
  ) {
    err = GetLastError();
    wprintf(L"REDIRECT-CC: CreateProcess failed (%d).\n", err);
    return err;
  }

  // Wait until child process exits.
  err = WaitForSingleObject(pi.hProcess, INFINITE);
  if (err != WAIT_OBJECT_0) {
    if (err == WAIT_FAILED) {
      err = GetLastError();
    }
    wprintf(L"REDIRECT-CC: Waiting for process to exit failed (%d).\n", err);
  } else {
    if (!GetExitCodeProcess(pi.hProcess, &err)) {
      err = GetLastError();
      wprintf(L"REDIRECT-CC: Failed to get process exit code (%d).\n", err);
    }
  }

  // Close process and thread handles.
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return err;
}
