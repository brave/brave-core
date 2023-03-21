/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SANDBOX_WIN_SRC_INTERCEPTORS_64_H_
#define BRAVE_CHROMIUM_SRC_SANDBOX_WIN_SRC_INTERCEPTORS_64_H_

#include "src/sandbox/win/src/interceptors_64.h"  // IWYU pragma: export

#if !defined(PSAPI_VERSION)
#error <Psapi.h> should be included.
#endif

namespace sandbox {

SANDBOX_INTERCEPT DWORD WINAPI TargetGetModuleFileNameA64(HMODULE hModule,
                                                          LPSTR lpFilename,
                                                          DWORD nSize);

SANDBOX_INTERCEPT DWORD WINAPI TargetGetModuleFileNameW64(HMODULE hModule,
                                                          LPWSTR lpFilename,
                                                          DWORD nSize);

#if (PSAPI_VERSION > 1)
#define TargetGetModuleFileNameExA64 TargetK32GetModuleFileNameExA64
#define TargetGetModuleFileNameExW64 TargetK32GetModuleFileNameExW64
#endif

SANDBOX_INTERCEPT DWORD WINAPI TargetGetModuleFileNameExA64(HANDLE hProcess,
                                                            HMODULE hModule,
                                                            LPSTR lpFilename,
                                                            DWORD nSize);

SANDBOX_INTERCEPT DWORD WINAPI TargetGetModuleFileNameExW64(HANDLE hProcess,
                                                            HMODULE hModule,
                                                            LPWSTR lpFilename,
                                                            DWORD nSize);

}  // namespace sandbox

#endif  // BRAVE_CHROMIUM_SRC_SANDBOX_WIN_SRC_INTERCEPTORS_64_H_
