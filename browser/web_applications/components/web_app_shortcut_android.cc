// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/web_applications/components/web_app_shortcut_android.h"

#include <fcntl.h>

#include "base/base_paths.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/i18n/file_util_icu.h"
#include "base/nix/xdg_util.h"
#include "base/path_service.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/scoped_blocking_call.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/web_applications/components/web_app_helpers.h"
#include "chrome/browser/web_applications/components/web_app_shortcut.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/chrome_constants.h"

namespace web_app {

base::FilePath GetAppShortcutFilename(const base::FilePath& profile_path,
                                      const std::string& app_id) {
  DCHECK(!app_id.empty());

  // Use a prefix, because xdg-desktop-menu requires it.
  std::string filename(chrome::kBrowserProcessExecutableName);
  filename.append("-").append(app_id).append("-").append(
      profile_path.BaseName().value());
  base::i18n::ReplaceIllegalCharactersInPath(&filename, '_');
  // Spaces in filenames break xdg-desktop-menu
  // (see https://bugs.freedesktop.org/show_bug.cgi?id=66605).
  base::ReplaceChars(filename, " ", "_", &filename);
  return base::FilePath(filename.append(".desktop"));
}

void DeleteShortcutOnDesktop(const base::FilePath& shortcut_filename) {
}

void DeleteShortcutInApplicationsMenu(
    const base::FilePath& shortcut_filename,
    const base::FilePath& directory_filename) {
}

bool CreateDesktopShortcut(
    const web_app::ShortcutInfo& shortcut_info,
    const web_app::ShortcutLocations& creation_locations) {
  return false;
}

web_app::ShortcutLocations GetExistingShortcutLocations(
    base::Environment* env,
    const base::FilePath& profile_path,
    const std::string& extension_id) {
  base::FilePath desktop_path;
  // If Get returns false, just leave desktop_path empty.
  base::PathService::Get(base::DIR_USER_DESKTOP, &desktop_path);
  return GetExistingShortcutLocations(env, profile_path, extension_id,
                                      desktop_path);
}

web_app::ShortcutLocations GetExistingShortcutLocations(
    base::Environment* env,
    const base::FilePath& profile_path,
    const std::string& extension_id,
    const base::FilePath& desktop_path) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);

  web_app::ShortcutLocations locations;
  return locations;
}

void DeleteDesktopShortcuts(const base::FilePath& profile_path,
                            const std::string& extension_id) {
}

void DeleteAllDesktopShortcuts(const base::FilePath& profile_path) {
}

namespace internals {

bool CreatePlatformShortcuts(const base::FilePath& web_app_path,
                             const ShortcutLocations& creation_locations,
                             ShortcutCreationReason /*creation_reason*/,
                             const ShortcutInfo& shortcut_info) {
  return false;
}

void DeletePlatformShortcuts(const base::FilePath& web_app_path,
                             const ShortcutInfo& shortcut_info) {
}

void UpdatePlatformShortcuts(const base::FilePath& web_app_path,
                             const base::string16& /*old_app_title*/,
                             const ShortcutInfo& shortcut_info) {
}

void DeleteAllShortcutsForProfile(const base::FilePath& profile_path) {
}

}  // namespace internals

}  // namespace web_app
