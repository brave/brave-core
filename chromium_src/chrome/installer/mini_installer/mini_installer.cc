/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/40285824): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "chrome/installer/mini_installer/mini_installer.h"

#include "base/compiler_specific.h"
#include "build/branding_buildflags.h"
#include "chrome/installer/mini_installer/configuration.h"
#include "chrome/installer/mini_installer/mini_installer_constants.h"
#include "chrome/installer/mini_installer/mini_string.h"
#include "chrome/installer/mini_installer/regkey.h"


#if defined(OFFICIAL_BUILD) && !BUILDFLAG(GOOGLE_CHROME_BRANDING)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (1)
#define NEED_TO_RESET_BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#endif  // defined(OFFICIAL_BUILD)

#define BRAVE_STUFF_PATCH_FLAG_INTO_WINDOWS_ERROR                            \
  if (exit_code.IsSuccess() && setup_type.compare(kLZMAResourceType) == 0) { \
    exit_code.windows_error = kIsLZMAResourceType;                           \
  }

#define BRAVE_GET_PREVIOUS_SETUP_EXE_PATH                        \
  if (*setup_path == L'\0') {                                    \
    ProcessExitResult exit_code = GetPreviousSetupExePath(       \
        configuration, setup_exe.get(), setup_exe.capacity());   \
    if (!exit_code.IsSuccess()) {                                \
      return exit_code;                                          \
    }                                                            \
  }

#define BRAVE_RUN_SETUP                                                      \
  if (configuration.previous_version() &&                                    \
      (!cmd_line.append(L" --") || !cmd_line.append(kCmdPreviousVersion) ||  \
       !cmd_line.append(L"=\"") ||                                           \
       !cmd_line.append(configuration.previous_version()) ||                 \
       !cmd_line.append(L"\""))) {                                           \
    return ProcessExitResult(COMMAND_STRING_OVERFLOW);                       \
  }                                                                          \
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

#define BRAVE_SET_INSTALLER_FLAGS SetInstallerFlags(configuration);

// If a compressed setup patch was found, run the previous setup.exe to
// patch and generate the new setup.exe.
#define BRAVE_RUN_PREVIOUS_SETUP_EXE \
  if (exit_code.IsSuccess() && exit_code.windows_error == kIsLZMAResourceType) { \
    PathString setup_dest_path;                                              \
    if (!setup_dest_path.assign(base_path.get()) ||                          \
        !setup_dest_path.append(kSetupExe)) {                                \
      return ProcessExitResult(PATH_STRING_OVERFLOW);                        \
    }                                                                        \
    exit_code = PatchSetup(configuration, setup_path, setup_dest_path,       \
                           max_delete_attempts);                             \
    if (exit_code.IsSuccess()) {                                             \
      setup_path.assign(setup_dest_path);                                    \
    } else {                                                                 \
      setup_path.clear();                                                    \
    }                                                                        \
  }

namespace mini_installer {

namespace {
ProcessExitResult PatchSetup(const Configuration& configuration,
                             const PathString& patch_path,
                             const PathString& dest_path,
                             int& max_delete_attempts);
}  // namespace

void SetInstallerFlags(const Configuration& configuration);

}  // namespace mini_installer

#define Initialize() Initialize(module)

#include <chrome/installer/mini_installer/mini_installer.cc>

#undef Initialize
#undef BRAVE_RUN_PREVIOUS_SETUP_EXE
#undef BRAVE_SET_INSTALLER_FLAGS
#undef BRAVE_RUN_SETUP
#undef BRAVE_GET_PREVIOUS_SETUP_EXE_PATH
#undef BRAVE_STUFF_PATCH_FLAG_INTO_WINDOWS_ERROR

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

void SetInstallerFlags(const Configuration& configuration) {
#if BUILDFLAG(GOOGLE_CHROME_BRANDING)
  // Set the magic suffix in registry to try full installer next time. We ignore
  // any errors here and we try to set the suffix for user level unless
  // GoogleUpdateIsMachine=1 is present in the environment or --system-level is
  // on the command line in which case we set it for system level instead. If
  // the current installer works, the flag to try full installer is cleared by
  // setup.exe at the end of install.

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
#endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING)
}


// Gets the setup.exe path from Registry by looking at the value of Uninstall
// string.  |size| is measured in wchar_t units.
ProcessExitResult GetSetupExePathForAppGuid(bool system_level,
                                            const wchar_t* app_guid,
                                            const wchar_t* previous_version,
                                            wchar_t* path,
                                            size_t size) {
  const HKEY root_key = system_level ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  RegKey key;
  LONG result = OpenClientStateKey(root_key, app_guid, KEY_QUERY_VALUE, &key);
  if (result == ERROR_SUCCESS) {
    result = key.ReadSZValue(kUninstallRegistryValue, path, size);
  }
  if (result != ERROR_SUCCESS) {
    return ProcessExitResult(UNABLE_TO_FIND_REGISTRY_KEY, result);
  }

  // Check that the path to the existing installer includes the expected
  // version number.  It's not necessary for accuracy to verify before/after
  // delimiters.
  if (!SearchStringI(path, previous_version)) {
    return ProcessExitResult(PATCH_NOT_FOR_INSTALLED_VERSION);
  }

  // Strip double-quotes surrounding the string, if present.
  if (size >= 1 && path[0] == '\"') {
    size_t path_length = SafeStrLen(path, size);
    if (path_length >= 2 && path[path_length - 1] == '\"') {
      if (!SafeStrCopy(path, size, path + 1)) {
        return ProcessExitResult(PATH_STRING_OVERFLOW);
      }
      path[path_length - 2] = '\0';
    }
  }

  return ProcessExitResult(SUCCESS_EXIT_CODE);
}

// Gets the path to setup.exe of the previous version. The overall path is found
// in the Uninstall string in the registry. A previous version number specified
// in |configuration| is used if available. |size| is measured in wchar_t units.
ProcessExitResult GetPreviousSetupExePath(const Configuration& configuration,
                                          wchar_t* path,
                                          size_t size) {
  // Check Chrome's ClientState key for the path to setup.exe. This will have
  // the correct path for all well-functioning installs.
  return GetSetupExePathForAppGuid(
      configuration.is_system_level(), configuration.chrome_app_guid(),
      configuration.previous_version(), path, size);
}

namespace {

// Applies an differential update to the previous setup.exe provided by
// `patch_path` and produces a new setup.exe at the path `target_path`.
ProcessExitResult PatchSetup(const Configuration& configuration,
                             const PathString& patch_path,
                             const PathString& dest_path,
                             int& max_delete_attempts) {
  CommandString cmd_line;
  PathString exe_path;
  ProcessExitResult exit_code = GetPreviousSetupExePath(
      configuration, exe_path.get(), exe_path.capacity());
  if (!exit_code.IsSuccess()) {
    return exit_code;
  }

  if (!cmd_line.append(L"\"") || !cmd_line.append(exe_path.get()) ||
      !cmd_line.append(L"\" --") || !cmd_line.append(kCmdUpdateSetupExe) ||
      !cmd_line.append(L"=\"") || !cmd_line.append(patch_path.get()) ||
      !cmd_line.append(L"\" --") || !cmd_line.append(kCmdNewSetupExe) ||
      !cmd_line.append(L"=\"") || !cmd_line.append(dest_path.get()) ||
      !cmd_line.append(L"\"")) {
    exit_code = ProcessExitResult(COMMAND_STRING_OVERFLOW);
  }

  if (!exit_code.IsSuccess()) {
    return exit_code;
  }

  // Get any command line option specified for mini_installer and pass them
  // on to setup.exe.
  AppendCommandLineFlags(configuration.command_line(), &cmd_line);

  exit_code = RunProcessAndWait(exe_path.get(), cmd_line.get(),
                                SETUP_PATCH_FAILED_FILE_NOT_FOUND,
                                SETUP_PATCH_FAILED_PATH_NOT_FOUND,
                                SETUP_PATCH_FAILED_COULD_NOT_CREATE_PROCESS);
  DeleteWithRetryAndMetrics(patch_path.get(), max_delete_attempts);

  return exit_code;
}

}  // namespace

}  // namespace mini_installer

#if defined(NEED_TO_RESET_BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (0)
#undef NEED_TO_RESET_BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#endif
