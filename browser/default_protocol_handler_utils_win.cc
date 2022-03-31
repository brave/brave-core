/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/default_protocol_handler_utils_win.h"

#include <shobjidl.h>
#include <winternl.h>
#include <wrl/client.h>

#include <memory>
#include <vector>

#include "base/base64.h"
#include "base/files/file_path.h"
#include "base/hash/md5.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util_win.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/win/atl.h"
#include "base/win/registry.h"
#include "base/win/scoped_co_mem.h"
#include "base/win/scoped_com_initializer.h"
#include "base/win/win_util.h"
#include "base/win/windows_version.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/util/shell_util.h"

// Most of source code in this file comes from firefox's SetDefaultBrowser -
// https://github.com/mozilla/gecko-dev/blob/master/toolkit/mozapps/defaultagent/SetDefaultBrowser.cpp

namespace protocol_handler_utils {

namespace {

constexpr wchar_t kUserChoiceKey[] = L"UserChoice";
constexpr wchar_t kProgIDKey[] = L"ProgID";
constexpr wchar_t kHashKey[] = L"Hash";

inline DWORD WordSwap(DWORD v) {
  return (v >> 16) | (v << 16);
}

std::wstring HashString(base::WStringPiece input_string) {
  auto* input_bytes =
      reinterpret_cast<const unsigned char*>(input_string.data());
  const int input_byte_count = (input_string.length() + 1) * sizeof(wchar_t);

  constexpr size_t kDWordsPerBlock = 2;
  constexpr size_t kBlockSize = sizeof(DWORD) * kDWordsPerBlock;
  // Incomplete blocks are ignored.
  const int block_count = input_byte_count / kBlockSize;

  if (block_count == 0) {
    return std::wstring();
  }

  // Compute an MD5 hash. md5[0] and md5[1] will be used as constant multipliers
  // in the scramble below.
  base::MD5Digest digest;
  base::MD5Sum(input_bytes, input_byte_count, &digest);
  auto* md5 = reinterpret_cast<DWORD*>(digest.a);
  // The following loop effectively computes two checksums, scrambled like a
  // hash after every DWORD is added.

  // Constant multipliers for the scramble, one set for each DWORD in a block.
  const DWORD c0s[kDWordsPerBlock][5] = {
      {md5[0] | 1, 0xCF98B111uL, 0x87085B9FuL, 0x12CEB96DuL, 0x257E1D83uL},
      {md5[1] | 1, 0xA27416F5uL, 0xD38396FFuL, 0x7C932B89uL, 0xBFA49F69uL}};
  const DWORD c1s[kDWordsPerBlock][5] = {
      {md5[0] | 1, 0xEF0569FBuL, 0x689B6B9FuL, 0x79F8A395uL, 0xC3EFEA97uL},
      {md5[1] | 1, 0xC31713DBuL, 0xDDCD1F0FuL, 0x59C3AF2DuL, 0x35BD1EC9uL}};

  // The checksums.
  DWORD h0 = 0;
  DWORD h1 = 0;
  // Accumulated total of the checksum after each DWORD.
  DWORD h0_acc = 0;
  DWORD h1_acc = 0;

  for (int i = 0; i < block_count; ++i) {
    for (size_t j = 0; j < kDWordsPerBlock; ++j) {
      const DWORD* c0 = c0s[j];
      const DWORD* c1 = c1s[j];

      DWORD input;
      memcpy(&input, &input_bytes[(i * kDWordsPerBlock + j) * sizeof(DWORD)],
             sizeof(DWORD));

      h0 += input;
      // Scramble 0
      h0 *= c0[0];
      h0 = WordSwap(h0) * c0[1];
      h0 = WordSwap(h0) * c0[2];
      h0 = WordSwap(h0) * c0[3];
      h0 = WordSwap(h0) * c0[4];
      h0_acc += h0;

      h1 += input;
      // Scramble 1
      h1 = WordSwap(h1) * c1[1] + h1 * c1[0];
      h1 = (h1 >> 16) * c1[2] + h1 * c1[3];
      h1 = WordSwap(h1) * c1[4] + h1;
      h1_acc += h1;
    }
  }

  DWORD hash[2] = {h0 ^ h1, h0_acc ^ h1_acc};
  std::string base64_text;
  base::Base64Encode(
      base::StringPiece(reinterpret_cast<const char*>(hash), sizeof(hash)),
      &base64_text);
  return base::UTF8ToWide(base64_text);
}

std::wstring FormatUserChoiceString(base::WStringPiece ext,
                                    base::WStringPiece sid,
                                    base::WStringPiece prog_id,
                                    SYSTEMTIME timestamp) {
  timestamp.wSecond = 0;
  timestamp.wMilliseconds = 0;

  FILETIME file_time = {0};
  if (!::SystemTimeToFileTime(&timestamp, &file_time))
    return std::wstring();

  // This string is built into Windows as part of the UserChoice hash algorithm.
  // It might vary across Windows SKUs (e.g. Windows 10 vs. Windows Server), or
  // across builds of the same SKU, but this is the only currently known
  // version. There isn't any known way of deriving it, so we assume this
  // constant value. If we are wrong, we will not be able to generate correct
  // UserChoice hashes.
  const wchar_t* user_experience =
      L"User Choice set via Windows User Experience "
      L"{D18B6DD5-6124-4341-9318-804003BAFA0B}";

  const wchar_t* user_choice_fmt =
      L"%ls%ls%ls"
      L"%08lx"
      L"%08lx"
      L"%ls";

  const std::wstring user_choice = base::StringPrintf(
      user_choice_fmt, ext.data(), sid.data(), prog_id.data(),
      file_time.dwHighDateTime, file_time.dwLowDateTime, user_experience);
  // For using CharLowerW instead of base::ToLowerASCII().
  // Otherwise, hash test with non-ascii inputs are failed.
  std::vector<wchar_t> buf(user_choice.begin(), user_choice.end());
  buf.push_back(0);
  ::CharLowerW(buf.data());
  return std::wstring(buf.data());
}

bool AddMillisecondsToSystemTime(SYSTEMTIME* system_time,
                                 ULONGLONG increment_ms) {
  FILETIME file_time;
  ULARGE_INTEGER file_time_int;
  if (!::SystemTimeToFileTime(system_time, &file_time))
    return false;

  file_time_int.LowPart = file_time.dwLowDateTime;
  file_time_int.HighPart = file_time.dwHighDateTime;

  // FILETIME is in units of 100ns.
  file_time_int.QuadPart += increment_ms * 1000 * 10;

  file_time.dwLowDateTime = file_time_int.LowPart;
  file_time.dwHighDateTime = file_time_int.HighPart;
  SYSTEMTIME tmp_system_time;
  if (!::FileTimeToSystemTime(&file_time, &tmp_system_time))
    return false;

  *system_time = tmp_system_time;
  return true;
}

// Compare two SYSTEMTIMEs as FILETIME after clearing everything
// below minutes.
bool CheckEqualMinutes(SYSTEMTIME system_time1, SYSTEMTIME system_time2) {
  system_time1.wSecond = 0;
  system_time1.wMilliseconds = 0;

  system_time2.wSecond = 0;
  system_time2.wMilliseconds = 0;

  FILETIME file_time1;
  FILETIME file_time2;
  if (!::SystemTimeToFileTime(&system_time1, &file_time1) ||
      !::SystemTimeToFileTime(&system_time2, &file_time2)) {
    return false;
  }

  return (file_time1.dwLowDateTime == file_time2.dwLowDateTime) &&
         (file_time1.dwHighDateTime == file_time2.dwHighDateTime);
}

std::wstring GetAssociationKeyPath(base::WStringPiece protocol) {
  const wchar_t* key_path_fmt;
  if (protocol[0] == L'.') {
    key_path_fmt =
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%"
        L"ls";
  } else {
    key_path_fmt =
        L"SOFTWARE\\Microsoft\\Windows\\Shell\\Associations\\"
        L"UrlAssociations\\%ls";
  }

  return base::StringPrintf(key_path_fmt, protocol.data());
}

bool SetUserChoice(base::WStringPiece ext,
                   base::WStringPiece sid,
                   base::WStringPiece prog_id) {
  SYSTEMTIME hash_timestamp;
  ::GetSystemTime(&hash_timestamp);
  auto hash = GenerateUserChoiceHash(ext, sid, prog_id, hash_timestamp);
  if (hash.empty())
    return false;

  // The hash changes at the end of each minute, so check that the hash should
  // be the same by the time we're done writing.
  constexpr ULONGLONG kWriteTimingThresholdMilliseconds = 100;
  // Generating the hash could have taken some time, so start from now.
  SYSTEMTIME write_end_timestamp;
  ::GetSystemTime(&write_end_timestamp);
  if (!AddMillisecondsToSystemTime(&write_end_timestamp,
                                   kWriteTimingThresholdMilliseconds)) {
    return false;
  }
  if (!CheckEqualMinutes(hash_timestamp, write_end_timestamp)) {
    LOG(ERROR) << "Hash is too close to expiration, sleeping until next hash.";
    ::Sleep(kWriteTimingThresholdMilliseconds * 2);

    // For consistency, use the current time.
    ::GetSystemTime(&hash_timestamp);
    hash = GenerateUserChoiceHash(ext, sid, prog_id, hash_timestamp);
    if (hash.empty())
      return false;
  }

  auto assoc_key_path = GetAssociationKeyPath(ext);
  if (assoc_key_path.empty())
    return false;

  base::win::RegKey assoc_key(HKEY_CURRENT_USER);
  if (assoc_key.OpenKey(assoc_key_path.c_str(), KEY_READ | KEY_WRITE) !=
      ERROR_SUCCESS) {
    LOG(ERROR) << "Can't open reg key: " << assoc_key_path;
    return false;
  }

  // When Windows creates this key, it is read-only (Deny Set Value), so we need
  // to delete it first.
  LSTATUS ls = assoc_key.DeleteKey(kUserChoiceKey);
  if (ls != ERROR_FILE_NOT_FOUND && ls != ERROR_SUCCESS) {
    LOG(ERROR) << "Failed to delete UserChoice key"
               << " " << ls;
    return false;
  }

  if (assoc_key.CreateKey(kUserChoiceKey, KEY_READ | KEY_WRITE) !=
      ERROR_SUCCESS) {
    LOG(ERROR) << "Failed to create UserChoice key";
    return false;
  }

  if (assoc_key.WriteValue(kProgIDKey, prog_id.data()) != ERROR_SUCCESS) {
    LOG(ERROR) << "Failed to write ProgID value";
    return false;
  }

  if (assoc_key.WriteValue(kHashKey, hash.c_str()) != ERROR_SUCCESS) {
    LOG(ERROR) << "Failed to write Hash value";
    return false;
  }

  assoc_key.Close();

  return true;
}

bool CheckProgIDExists(base::WStringPiece prog_id) {
  base::win::RegKey root(HKEY_CLASSES_ROOT);
  return root.OpenKey(prog_id.data(), KEY_READ) == ERROR_SUCCESS;
}

std::wstring GetBrowserProgId() {
  base::FilePath brave_exe;
  if (!base::PathService::Get(base::FILE_EXE, &brave_exe)) {
    LOG(ERROR) << "Error getting app exe path";
    return std::wstring();
  }

  const auto suffix = ShellUtil::GetCurrentInstallationSuffix(brave_exe);
  std::wstring brave_html =
      base::StrCat({install_static::GetProgIdPrefix(), suffix});

  // ProgIds cannot be longer than 39 characters.
  // Ref: http://msdn.microsoft.com/en-us/library/aa911706.aspx.
  // Make all new registrations comply with this requirement (existing
  // registrations must be preserved).
  std::wstring new_style_suffix;
  if (ShellUtil::GetUserSpecificRegistrySuffix(&new_style_suffix) &&
      suffix == new_style_suffix && brave_html.length() > 39) {
    NOTREACHED();
    brave_html.erase(39);
  }
  return brave_html;
}

std::wstring GetProgIdForProtocol(base::WStringPiece protocol) {
  Microsoft::WRL::ComPtr<IApplicationAssociationRegistration> registration;
  HRESULT hr =
      ::CoCreateInstance(CLSID_ApplicationAssociationRegistration, nullptr,
                         CLSCTX_INPROC, IID_PPV_ARGS(&registration));
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to create IApplicationAssociationRegistration";
    return std::wstring();
  }
  base::win::ScopedCoMem<wchar_t> current_app;
  const bool is_protocol = protocol[0] != L'.';
  hr = registration->QueryCurrentDefault(
      protocol.data(), is_protocol ? AT_URLPROTOCOL : AT_FILEEXTENSION,
      AL_EFFECTIVE, &current_app);
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to query default app for protocol " << protocol;
    return std::wstring();
  }
  return current_app.get();
}

// NOTE: The passed-in current user SID is used here, instead of getting the SID
// for the owner of the key. We are assuming that this key in HKCU is owned by
// the current user, since we want to replace that key ourselves. If the key is
// owned by someone else, then this check will fail; this is ok because we would
// likely not want to replace that other user's key anyway.
bool CheckUserChoiceHash(base::WStringPiece protocol,
                         base::WStringPiece user_sid) {
  auto key_path = GetAssociationKeyPath(protocol);
  if (key_path.empty())
    return false;

  base::win::RegKey user_choice_key(HKEY_CURRENT_USER, key_path.c_str(),
                                    KEY_READ);
  if (!user_choice_key.Valid())
    return false;

  if (user_choice_key.OpenKey(kUserChoiceKey, KEY_READ) != ERROR_SUCCESS)
    return false;

  FILETIME last_write_file_time;

  // RegKey doesn't support for fetching last write time.
  if (::RegQueryInfoKeyW(user_choice_key.Handle(), nullptr, nullptr, nullptr,
                         nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                         nullptr, &last_write_file_time) != ERROR_SUCCESS) {
    return false;
  }

  SYSTEMTIME last_write_system_time;
  if (!::FileTimeToSystemTime(&last_write_file_time, &last_write_system_time))
    return false;

  // Read ProgId
  std::wstring prog_id;
  if (user_choice_key.ReadValue(kProgIDKey, &prog_id) != ERROR_SUCCESS)
    return false;

  // Read Hash
  std::wstring stored_hash;
  if (user_choice_key.ReadValue(kHashKey, &stored_hash) != ERROR_SUCCESS)
    return false;

  auto computed_hash = GenerateUserChoiceHash(protocol, user_sid, prog_id,
                                              last_write_system_time);
  if (computed_hash.empty())
    return false;

  if (::CompareStringOrdinal(computed_hash.c_str(), -1, stored_hash.c_str(), -1,
                             FALSE) != CSTR_EQUAL) {
    return false;
  }

  return true;
}

}  // namespace

std::wstring GenerateUserChoiceHash(base::WStringPiece ext,
                                    base::WStringPiece sid,
                                    base::WStringPiece prog_id,
                                    SYSTEMTIME timestamp) {
  auto user_choice = FormatUserChoiceString(ext, sid, prog_id, timestamp);
  if (user_choice.empty()) {
    LOG(ERROR) << "Didn't get user choice string for generating hash.";
    return std::wstring();
  }

  return HashString(user_choice);
}

bool SetDefaultProtocolHandlerFor(base::WStringPiece protocol) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);

  if (IsDefaultProtocolHandlerFor(protocol)) {
    VLOG(2) << "Already default handler for " << protocol;
    return true;
  }

  const auto prog_id = GetBrowserProgId();
  if (!CheckProgIDExists(prog_id)) {
    LOG(ERROR) << "ProgId is not found - " << prog_id;
    return false;
  }

  std::wstring user_sid;
  if (!base::win::GetUserSidString(&user_sid)) {
    LOG(ERROR) << "Can't get user sid";
    return false;
  }

  if (!CheckUserChoiceHash(protocol, user_sid)) {
    LOG(ERROR) << "UserChoice Hash mismatch";
    return false;
  }

  // Applied FF's policy: unsupported below Windows 10 RS2
  if (base::win::GetVersion() < base::win::Version::WIN10_RS2) {
    VLOG(2) << "UserChoice hash matched, but Windows build is too old";
    return false;
  }

  if (!SetUserChoice(protocol, user_sid, prog_id))
    return false;

  // Verify after set.
  return GetProgIdForProtocol(protocol) == prog_id;
}

bool IsDefaultProtocolHandlerFor(base::WStringPiece protocol) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);

  const auto prog_id = GetBrowserProgId();
  if (!CheckProgIDExists(prog_id)) {
    LOG(ERROR) << "ProgId is not found - " << prog_id;
    return false;
  }

  return prog_id == GetProgIdForProtocol(protocol);
}

}  // namespace protocol_handler_utils
