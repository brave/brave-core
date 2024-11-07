 /* Copyright (c) 2019 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at https://mozilla.org/MPL/2.0/. */

namespace base {
class FilePath;
}  // namespace base

namespace {
base::FilePath GetLocalizableBraveAppShortcutsSubdirName();
}

#define BRAVE_GET_CHROME_APPS_FOLDER_IMPL \
  return path.Append(GetLocalizableBraveAppShortcutsSubdirName());

#include "src/chrome/browser/web_applications/os_integration/mac/apps_folder_support.mm"
#undef BRAVE_GET_CHROME_APPS_FOLDER_IMPL

namespace {
base::FilePath GetLocalizableBraveAppShortcutsSubdirName() {
  // clang-format off
  static const char kBraveBrowserDevelopmentAppDirName[] =
      "Brave Browser Development Apps.localized";
  static const char kBraveBrowserAppDirName[] =
      "Brave Browser Apps.localized";
  static const char kBraveBrowserBetaAppDirName[] =
      "Brave Browser Beta Apps.localized";
  static const char kBraveBrowserDevAppDirName[] =
      "Brave Browser Dev Apps.localized";
  static const char kBraveBrowserNightlyAppDirName[] =
      "Brave Browser Nightly Apps.localized";
  // clang-format on

  switch (chrome::GetChannel()) {
    case version_info::Channel::STABLE:
      return base::FilePath(kBraveBrowserAppDirName);
    case version_info::Channel::BETA:
      return base::FilePath(kBraveBrowserBetaAppDirName);
    case version_info::Channel::DEV:
      return base::FilePath(kBraveBrowserDevAppDirName);
    case version_info::Channel::CANARY:
      return base::FilePath(kBraveBrowserNightlyAppDirName);
    case version_info::Channel::UNKNOWN:
      return base::FilePath(kBraveBrowserDevelopmentAppDirName);
    default:
      break;
  }

  NOTREACHED() << "All possible channels are handled above.";
}
}  // namespace
