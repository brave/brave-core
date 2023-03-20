/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_SANDBOX_WIN_SRC_MODULE_FILE_NAME_INTERCEPTION_H_
#define BRAVE_SANDBOX_WIN_SRC_MODULE_FILE_NAME_INTERCEPTION_H_

#include <Windows.h>

#include <Psapi.h>

#include "sandbox/win/src/sandbox_types.h"

#if (PSAPI_VERSION > 1)
#define TargetGetModuleFileNameExA TargetK32GetModuleFileNameExA
#define TargetGetModuleFileNameExW TargetK32GetModuleFileNameExW
#endif

namespace sandbox {

using GetModuleFileNameAFunction = decltype(&::GetModuleFileNameA);
using GetModuleFileNameWFunction = decltype(&::GetModuleFileNameW);
using GetModuleFileNameExAFunction = decltype(&::GetModuleFileNameExA);
using GetModuleFileNameExWFunction = decltype(&::GetModuleFileNameExW);

SANDBOX_INTERCEPT DWORD WINAPI
TargetGetModuleFileNameA(GetModuleFileNameAFunction orig,
                         HMODULE hModule,
                         LPSTR lpFilename,
                         DWORD nSize);

SANDBOX_INTERCEPT DWORD WINAPI
TargetGetModuleFileNameW(GetModuleFileNameWFunction orig,
                         HMODULE hModule,
                         LPWSTR lpFilename,
                         DWORD nSize);

SANDBOX_INTERCEPT DWORD WINAPI
TargetGetModuleFileNameExA(GetModuleFileNameExAFunction orig,
                           HANDLE hProcess,
                           HMODULE hModule,
                           LPSTR lpFilename,
                           DWORD nSize);

SANDBOX_INTERCEPT DWORD WINAPI
TargetGetModuleFileNameExW(GetModuleFileNameExWFunction orig,
                           HANDLE hProcess,
                           HMODULE hModule,
                           LPWSTR lpFilename,
                           DWORD nSize);

}  // namespace sandbox

#endif  // BRAVE_SANDBOX_WIN_SRC_MODULE_FILE_NAME_INTERCEPTION_H_
