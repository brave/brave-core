// Copyright (c) 2019 The Brave Authors
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#define BRAVE_REFERRAL                                                       \
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
    if (!cmd_line.append(L" --") ||                                          \
        !cmd_line.append(L"brave-referral-code") ||                          \
        !cmd_line.append(L"=\"") || !cmd_line.append(referral_code.get()) || \
        !cmd_line.append(L"\"")) {                                           \
          return ProcessExitResult(COMMAND_STRING_OVERFLOW);                 \
      }                                                                      \
    }                                                                        \
  }
#include "../../../../../chrome/installer/mini_installer/mini_installer.cc"
#undef BRAVE_REFERRAL

namespace mini_installer {

// Coverts a string in place to uppercase
void SafeStrASCIIUpper(wchar_t* str, size_t size) {
  if (!str || !size)
    return;

  for (size_t i = 0; i < size && str[i] != L'\0'; ++i) {
    wchar_t c = str[i];
    if (c >= L'a' && c <= L'z')
      str[i] += L'A' - L'a';
  }
}

bool ParseStandardReferralCode(const wchar_t* filename,
                ReferralCodeString* referral_code) {
  // Scan backwards for last dash in filename.
  const wchar_t* anchor = filename + lstrlen(filename) - 1;
  const wchar_t* scan = anchor;
  while (scan != filename && *scan != L'-')
    --scan;

  if (*scan++ != L'-')
    return false;

  if (anchor - scan != StandardReferralCodeLen)
    return false;

  const wchar_t* ref_code = scan;
  wchar_t ref_code_normalized[StandardReferralCodeLen + 1];

  // Ensure that first half of referral code is alphabetic.
  for (int i = 0; i < StandardReferralCodeLen / 2; ++i) {
    if ((ref_code[i] < L'a' || ref_code[i] > L'z') &&
        (ref_code[i] < L'A' || ref_code[i] > L'Z'))
      return false;
  }

  // Ensure that second half of referral code is numeric.
  for (int i = StandardReferralCodeLen / 2; i < StandardReferralCodeLen; ++i) {
    if (ref_code[i] < L'0' || ref_code[i] > L'9')
      return false;
  }

  if (!SafeStrCopy(ref_code_normalized, StandardReferralCodeLen, ref_code))
    return false;

  SafeStrASCIIUpper(ref_code_normalized, StandardReferralCodeLen);

  if (!referral_code->assign(ref_code_normalized))
    return false;

  return true;
}

bool ParseExtendedReferralCode(const wchar_t* filename,
                ReferralCodeString* referral_code) {
  // Scan backwards for second-to-last dash in filename, since this
  // type of referral code has an embedded dash.
  const wchar_t* scan = filename + lstrlen(filename) - 1;
  while (scan != filename && *scan != L'-')
    --scan;

  if (*scan-- != L'-')
    return false;

  while (scan != filename && *scan != L'-')
    --scan;

  if (*scan++ != L'-')
    return false;

  // Ensure that referral code is alphabetic.
  const wchar_t* ref_code = scan;
  int dashes = 0;
  for (int i = 0; i < lstrlen(ref_code); ++i) {
    if ((ref_code[i] < L'a' || ref_code[i] > L'z') &&
        (ref_code[i] < L'A' || ref_code[i] > L'Z') && (ref_code[i] != L'-'))
      return NULL;
    if (ref_code[i] == L'-')
      ++dashes;
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
  const wchar_t* scan = filename.get() + filename.length() - 1;
  while (scan != filename.get() && *scan != L'.')
    --scan;

  if (*scan == L'.')
    filename.truncate_at(scan - filename.get());

  // Strip any de-duplicating suffix from filename, e.g. "(1)".
  scan = filename.get() + filename.length() - 1;
  if (*scan == L')') {
    --scan;
    while (scan != filename.get() && *scan >= '0' && *scan <= '9')
      --scan;
    if (*scan == L'(')
      filename.truncate_at(scan - filename.get());
  }

  // Strip trailing spaces from filename.
  scan = filename.get() + filename.length() - 1;
  while (scan != filename.get() && *scan == L' ')
    --scan;

  if (scan != filename.get() && (scan != filename.get() + filename.length()))
    filename.truncate_at(scan - filename.get() + 1);

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
