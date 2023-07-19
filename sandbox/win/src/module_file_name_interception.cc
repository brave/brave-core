/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/sandbox/win/src/module_file_name_interception.h"

#include <string.h>
#include <algorithm>
#include <string>

#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/win/windows_types.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace {

void ReplaceAt(char* dest, size_t dest_size, base::StringPiece src) {
  ::strncpy_s(dest, dest_size, src.data(),
              std::min(dest_size - 1, src.length()));
}

void ReplaceAt(wchar_t* dest, size_t dest_size, base::WStringPiece src) {
  ::wcsncpy_s(dest, dest_size, src.data(),
              std::min(dest_size - 1, src.length()));
}

template <typename CharT>
struct BraveToChrome;

template <>
struct BraveToChrome<char> {
  static constexpr const base::StringPiece kBrave = "brave.exe";
  static constexpr const base::StringPiece kChrome = "chrome.exe";
};

template <>
struct BraveToChrome<wchar_t> {
  static constexpr const base::WStringPiece kBrave = L"brave.exe";
  static constexpr const base::WStringPiece kChrome = L"chrome.exe";
};

template <typename CharT>
struct TestBraveToChrome;

template <>
struct TestBraveToChrome<char> {
  static constexpr const base::StringPiece kBrave = "brave_browser_tests.exe";
  static constexpr const base::StringPiece kChrome = "chrome_browser_tests.exe";
};

template <>
struct TestBraveToChrome<wchar_t> {
  static constexpr const base::WStringPiece kBrave = L"brave_browser_tests.exe";
  static constexpr const base::WStringPiece kChrome =
      L"chrome_browser_tests.exe";
};

template <template <class T> class FromTo, typename CharT>
absl::optional<DWORD> PatchFilenameImpl(CharT* filename,
                                        DWORD length,
                                        DWORD size) {
  if (!base::EndsWith(base::BasicStringPiece<CharT>(filename, length),
                      FromTo<CharT>::kBrave,
                      base::CompareCase::INSENSITIVE_ASCII)) {
    return absl::nullopt;
  }

  constexpr DWORD kBraveLen = FromTo<CharT>::kBrave.length();
  constexpr DWORD kChromeLen = FromTo<CharT>::kChrome.length();
  static_assert(kBraveLen <= kChromeLen);
  constexpr DWORD kLenDiff = kChromeLen - kBraveLen;

  --size;  // space for null-terminator

  const size_t brave_pos = length - kBraveLen;
  ReplaceAt(filename + brave_pos, size - brave_pos, FromTo<CharT>::kChrome);
  if (size < length + kLenDiff) {
    ::SetLastError(ERROR_INSUFFICIENT_BUFFER);
  }
  length = std::min(size, length + kLenDiff);
  filename[length] = 0;
  return length;
}

template <typename CharT>
DWORD PatchFilename(CharT* filename, DWORD length, DWORD size) {
  if (auto r = PatchFilenameImpl<BraveToChrome>(filename, length, size)) {
    return *r;
  }
  if (auto r = PatchFilenameImpl<TestBraveToChrome>(filename, length, size)) {
    return *r;
  }
  return length;
}

}  // namespace

namespace sandbox {

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
