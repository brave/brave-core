/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/sandbox/win/src/module_file_name_interception.h"

#include <string.h>
#include <algorithm>
#include <string>
#include <type_traits>

#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/win/windows_types.h"
#include "sandbox/win/src/sandbox_types.h"

namespace {

template <typename CharT>
struct BraveToChrome;

template <>
struct BraveToChrome<char> {
  static constexpr const base::StringPiece kBrave = "brave.exe";
  static constexpr const base::StringPiece kChrome = "chrome.exe";

  static void ReplaceAt(char* dest, size_t dest_size) {
    ::strncpy_s(dest, dest_size, kChrome.data(), kChrome.length());
  }
};

template <>
struct BraveToChrome<wchar_t> {
  static constexpr const base::WStringPiece kBrave = L"brave.exe";
  static constexpr const base::WStringPiece kChrome = L"chrome.exe";

  static void ReplaceAt(wchar_t* dest, size_t dest_size) {
    ::wcsncpy_s(dest, dest_size, kChrome.data(), kChrome.length());
  }
};

template <typename StringType,
          typename CharT = std::remove_pointer_t<StringType>>
DWORD PatchFilename(StringType filename, DWORD length, DWORD size) {
  if (!base::EndsWith(filename, BraveToChrome<CharT>::kBrave,
                      base::CompareCase::INSENSITIVE_ASCII)) {
    return length;
  }

  constexpr DWORD kBraveLen = BraveToChrome<CharT>::kBrave.length();
  constexpr DWORD kChromeLen = BraveToChrome<CharT>::kChrome.length();
  static_assert(kBraveLen <= kChromeLen);
  constexpr DWORD kLenDiff = kChromeLen - kBraveLen;

  const size_t brave_pos = length - kBraveLen;
  BraveToChrome<CharT>::ReplaceAt(filename + brave_pos, size - brave_pos);
  if (size < length + kLenDiff) {
    ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
  }
  const auto result = std::min(size, length + kLenDiff);
  filename[result] = 0;
  return result;
}

}  // namespace

namespace sandbox {

BASE_FEATURE(kModuleFileNamePatch,
             "ModuleFileNamePatch",
             base::FEATURE_DISABLED_BY_DEFAULT);

SANDBOX_INTERCEPT DWORD WINAPI
TargetGetModuleFileNameA(GetModuleFileNameAFunction orig,
                         HMODULE hModule,
                         LPSTR lpFilename,
                         DWORD nSize) {
  const auto result = orig(hModule, lpFilename, nSize);
  if (result != 0) {
    return PatchFilename(lpFilename, result, nSize);
  }
  return result;
}

SANDBOX_INTERCEPT DWORD WINAPI
TargetGetModuleFileNameW(GetModuleFileNameWFunction orig,
                         HMODULE hModule,
                         LPWSTR lpFilename,
                         DWORD nSize) {
  const auto result = orig(hModule, lpFilename, nSize);
  if (result != 0) {
    return PatchFilename(lpFilename, result, nSize);
  }
  return result;
}

SANDBOX_INTERCEPT DWORD WINAPI
TargetGetModuleFileNameExA(GetModuleFileNameExAFunction orig,
                           HANDLE hProcess,
                           HMODULE hModule,
                           LPSTR lpFilename,
                           DWORD nSize) {
  const auto result = orig(hProcess, hModule, lpFilename, nSize);
  if (result != 0) {
    return PatchFilename(lpFilename, result, nSize);
  }
  return result;
}

SANDBOX_INTERCEPT DWORD WINAPI
TargetGetModuleFileNameExW(GetModuleFileNameExWFunction orig,
                           HANDLE hProcess,
                           HMODULE hModule,
                           LPWSTR lpFilename,
                           DWORD nSize) {
  const auto result = orig(hProcess, hModule, lpFilename, nSize);
  if (result != 0) {
    return PatchFilename(lpFilename, result, nSize);
  }
  return result;
}

}  // namespace sandbox
