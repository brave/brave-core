/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/compiler_specific.h"
#include "build/branding_buildflags.h"
#include "chrome/installer/mini_installer/configuration.h"
#include "chrome/installer/mini_installer/mini_installer_constants.h"
#include "chrome/installer/mini_installer/mini_string.h"
#include "chrome/installer/mini_installer/regkey.h"


#define BRAVE_RUN_SETUP                                                      \
  PathString installer_filename;                                             \
  wchar_t value[MAX_PATH] = {0, };                                           \
  const bool result =                                                        \
      RegKey::ReadSZValue(HKEY_CURRENT_USER,                                 \
                          L"Software\\BraveSoftware\\Promo",                 \
                          L"StubInstallerPath", value, _countof(value)) ;    \
  if (result &&                                                              \
       installer_filename.assign(value) &&                                   \
       installer_filename.length() != 0) {                                   \
    ReferralCodeString referral_code;                                        \
    if (ParseReferralCode(installer_filename.get(), &referral_code)) {       \
      cmd_line.append(L" --brave-referral-code");                            \
      cmd_line.append(L"=\"");                                               \
      cmd_line.append(referral_code.get());                                  \
      cmd_line.append(L"\"");                                                \
    }                                                                        \
  }

#if defined(OFFICIAL_BUILD)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (1)
#endif  // defined(OFFICIAL_BUILD)

namespace mini_installer {

#if BUILDFLAG(GOOGLE_CHROME_BRANDING)

bool OpenInstallStateKey(const Configuration& configuration, RegKey* key);

// This function sets the flag in registry to indicate that Google Update
// should try full installer next time. If the current installer works, this
// flag is cleared by setup.exe at the end of install.
void SetInstallerFlags(const Configuration& configuration) {
  StackString<128> value;

  RegKey key;
  if (!OpenInstallStateKey(configuration, &key)) {
    return;
  }

  // TODO(grt): Trim legacy modifiers (chrome,chromeframe,apphost,applauncher,
  // multi,readymode,stage,migrating,multifail) from the ap value.

  LONG ret = key.ReadSZValue(kApRegistryValue, value.get(), value.capacity());

  // The conditions below are handling two cases:
  // 1. When ap value is present, we want to add the required tag only if it
  //    is not present.
  // 2. When ap value is missing, we are going to create it with the required
  //    tag.
  if ((ret == ERROR_SUCCESS) || (ret == ERROR_FILE_NOT_FOUND)) {
    if (ret == ERROR_FILE_NOT_FOUND) {
      value.clear();
    }

    if (!StrEndsWith(value.get(), kFullInstallerSuffix) &&
        value.append(kFullInstallerSuffix)) {
      key.WriteSZValue(kApRegistryValue, value.get());
    }
  }
}
#endif

}  // namespace mini_installer

#include <chrome/installer/mini_installer/mini_installer.cc>
#if defined(OFFICIAL_BUILD)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#endif  // defined(OFFICIAL_BUILD)
#undef BRAVE_RUN_SETUP

namespace mini_installer {

namespace {
constexpr size_t kStandardReferralCodeLen = 6;
}  // namespace

// Coverts a string in place to uppercase
void SafeStrASCIIUpper(wchar_t* str, size_t size) {
  if (!str || !size)
    return;

  for (size_t i = 0; i < size && UNSAFE_TODO(str[i]) != L'\0'; ++i) {
    wchar_t c = UNSAFE_TODO(str[i]);
    if (c >= L'a' && c <= L'z')
      UNSAFE_TODO(str[i]) += L'A' - L'a';
  }
}

bool ParseStandardReferralCode(const wchar_t* filename,
                ReferralCodeString* referral_code) {
  // Scan backwards for last dash in filename.
  const wchar_t* anchor = UNSAFE_TODO(filename + lstrlen(filename) - 1);
  const wchar_t* scan = anchor;
  while (scan != filename && *scan != L'-')
    UNSAFE_TODO(--scan);

  if (*UNSAFE_TODO(scan++) != L'-') {
    return false;
  }

  if (anchor - scan + 1 != kStandardReferralCodeLen)
    return false;

  const wchar_t* ref_code = scan;
  wchar_t ref_code_normalized[kStandardReferralCodeLen + 1];

  // Ensure that first half of referral code is alphabetic.
  for (size_t i = 0; i < kStandardReferralCodeLen / 2; ++i) {
    if ((UNSAFE_TODO(ref_code[i]) < L'a' || UNSAFE_TODO(ref_code[i]) > L'z') &&
        (UNSAFE_TODO(ref_code[i]) < L'A' || UNSAFE_TODO(ref_code[i]) > L'Z')) {
      return false;
    }
  }

  // Ensure that second half of referral code is numeric.
  for (size_t i = kStandardReferralCodeLen / 2; i < kStandardReferralCodeLen;
       ++i) {
    if (UNSAFE_TODO(ref_code[i]) < L'0' || UNSAFE_TODO(ref_code[i]) > L'9') {
      return false;
    }
  }

  if (!SafeStrCopy(ref_code_normalized, kStandardReferralCodeLen + 1, ref_code))
    return false;

  SafeStrASCIIUpper(ref_code_normalized, kStandardReferralCodeLen);

  if (!referral_code->assign(ref_code_normalized))
    return false;

  return true;
}

bool ParseExtendedReferralCode(const wchar_t* filename,
                ReferralCodeString* referral_code) {
  // Scan backwards for second-to-last dash in filename, since this
  // type of referral code has an embedded dash.
  const wchar_t* scan = UNSAFE_TODO(filename + lstrlen(filename) - 1);
  while (scan != filename && *scan != L'-')
    UNSAFE_TODO(--scan);

  if (*UNSAFE_TODO(scan--) != L'-') {
    return false;
  }

  while (scan != filename && *scan != L'-')
    UNSAFE_TODO(--scan);

  if (*UNSAFE_TODO(scan++) != L'-') {
    return false;
  }

  // Ensure that referral code is alphabetic.
  const wchar_t* ref_code = scan;
  int dashes = 0;
  for (int i = 0; i < lstrlen(ref_code); ++i) {
    if ((UNSAFE_TODO(ref_code[i]) < L'a' || UNSAFE_TODO(ref_code[i]) > L'z') &&
        (UNSAFE_TODO(ref_code[i]) < L'A' || UNSAFE_TODO(ref_code[i]) > L'Z') &&
        (UNSAFE_TODO(ref_code[i]) != L'-')) {
      return false;
    }
    if (UNSAFE_TODO(ref_code[i]) == L'-') {
      ++dashes;
    }
  }

  // Ensure that referral code contains exactly one dash.
  if (dashes != 1)
    return false;

  if (!referral_code->assign(ref_code))
    return false;

  return true;
}

bool ParseReferralCode(const wchar_t* installer_filename,
                       ReferralCodeString* referral_code) {
  PathString filename;
  if (!filename.assign(
          GetNameFromPathExt(installer_filename, lstrlen(installer_filename))))
    return false;

  // Strip extension from filename.
  const wchar_t* scan = UNSAFE_TODO(filename.get() + filename.length() - 1);
  while (scan != filename.get() && *scan != L'.')
    UNSAFE_TODO(--scan);

  if (*scan == L'.')
    filename.truncate_at(scan - filename.get());

  // Strip any de-duplicating suffix from filename, e.g. "(1)".
  scan = UNSAFE_TODO(filename.get() + filename.length() - 1);
  if (*scan == L')') {
    UNSAFE_TODO(--scan);
    while (scan != filename.get() && *scan >= '0' && *scan <= '9')
      UNSAFE_TODO(--scan);
    if (*scan == L'(')
      filename.truncate_at(scan - filename.get());
  }

  // Strip trailing spaces from filename.
  scan = UNSAFE_TODO(filename.get() + filename.length() - 1);
  while (scan != filename.get() && *scan == L' ')
    UNSAFE_TODO(--scan);

  if (scan != filename.get() &&
      (scan != UNSAFE_TODO(filename.get() + filename.length()))) {
    filename.truncate_at(scan - filename.get() + 1);
  }

  // First check for 6-character standard referral code XXXDDD, where
  // X is an alphabetic character and D is a numeric character. If not
  // found, check for an alphabetic referral code of any length in the
  // form XXX-XXX.
  if (!ParseStandardReferralCode(filename.get(), referral_code) &&
      !ParseExtendedReferralCode(filename.get(), referral_code)) {
    return false;
  }

  return true;
}

}  // namespace mini_installer
