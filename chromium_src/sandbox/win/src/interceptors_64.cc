/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/sandbox/win/src/interceptors_64.cc"

#include "brave/sandbox/win/src/module_file_name_interception.h"

namespace sandbox {

SANDBOX_INTERCEPT DWORD WINAPI TargetGetModuleFileNameA64(HMODULE hModule,
                                                          LPSTR lpFilename,
                                                          DWORD nSize) {
  auto orig_fn = reinterpret_cast<GetModuleFileNameAFunction>(
      g_originals[GET_MODULE_FILENAME_A_ID]);
  return TargetGetModuleFileNameA(orig_fn, hModule, lpFilename, nSize);
}

SANDBOX_INTERCEPT DWORD WINAPI TargetGetModuleFileNameW64(HMODULE hModule,
                                                          LPWSTR lpFilename,
                                                          DWORD nSize) {
  auto orig_fn = reinterpret_cast<GetModuleFileNameWFunction>(
      g_originals[GET_MODULE_FILENAME_W_ID]);
  return TargetGetModuleFileNameW(orig_fn, hModule, lpFilename, nSize);
}

SANDBOX_INTERCEPT DWORD WINAPI TargetGetModuleFileNameExA64(HANDLE hProcess,
                                                            HMODULE hModule,
                                                            LPSTR lpFilename,
                                                            DWORD nSize) {
  auto orig_fn = reinterpret_cast<GetModuleFileNameExAFunction>(
      g_originals[GET_MODULE_FILENAME_EX_A_ID]);
  return TargetGetModuleFileNameExA(orig_fn, hProcess, hModule, lpFilename,
                                    nSize);
}

SANDBOX_INTERCEPT DWORD WINAPI TargetGetModuleFileNameExW64(HANDLE hProcess,
                                                            HMODULE hModule,
                                                            LPWSTR lpFilename,
                                                            DWORD nSize) {
  auto orig_fn = reinterpret_cast<GetModuleFileNameExWFunction>(
      g_originals[GET_MODULE_FILENAME_EX_W_ID]);
  return TargetGetModuleFileNameExW(orig_fn, hProcess, hModule, lpFilename,
                                    nSize);
}

}  // namespace sandbox
