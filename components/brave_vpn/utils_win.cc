/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/utils_win.h"

#include <ras.h>
#include <raserror.h>
#include <stdio.h>

#include "base/logging.h"
#include "base/strings/stringprintf.h"

namespace brave_vpn {

namespace {

// https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage
void PrintSystemError(DWORD error) {
  DWORD c_buf_size = 512;
  TCHAR lpsz_error_string[512];

  DWORD buf_len =
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    lpsz_error_string, c_buf_size, NULL);
  if (buf_len) {
    LOG(ERROR) << lpsz_error_string;
  }
}

}  // namespace

// https://docs.microsoft.com/en-us/windows/win32/api/ras/nf-ras-rasgeterrorstringa
void PrintRasError(DWORD error) {
  DWORD c_buf_size = 512;
  TCHAR lpsz_error_string[512];

  if (error > RASBASE && error < RASBASEEND) {
    if (RasGetErrorString(error, lpsz_error_string, c_buf_size) ==
        ERROR_SUCCESS) {
      LOG(ERROR) << lpsz_error_string;
      return;
    }
  }

  PrintSystemError(error);
}

std::wstring GetPhonebookPath() {
  wchar_t app_data_path[1025] = {0};
  std::wstring phone_book_path;

  // https://docs.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-expandenvironmentstringsa
  DWORD dw_ret =
      ExpandEnvironmentStrings(TEXT("%APPDATA%"), app_data_path, 1024);
  if (dw_ret == 0) {
    LOG(ERROR) << "failed to get APPDATA path";
    PrintRasError(GetLastError());
    return phone_book_path;
  }

  phone_book_path = base::StringPrintf(
      L"%ls\\Microsoft\\Network\\Connections\\Pbk\\rasphone.pbk",
      app_data_path);

  return phone_book_path;
}

}  // namespace brave_vpn
