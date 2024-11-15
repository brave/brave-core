/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#include "brave/components/l10n/common/localization_util.h"

#if !BUILDFLAG(IS_WIN)
#include "chrome/grit/generated_resources.h"

#define GetAppShortcutsSubdirName GetAppShortcutsSubdirName_UnUsed
#endif

#include "src/chrome/browser/shell_integration.cc"

#if !BUILDFLAG(IS_WIN)
#undef GetAppShortcutsSubdirName
#endif

#if !BUILDFLAG(IS_WIN)
namespace shell_integration {
std::u16string GetAppShortcutsSubdirName() {
  int id = IDS_APP_SHORTCUTS_SUBDIR_NAME_BRAVE_STABLE;
  switch (chrome::GetChannel()) {
    case version_info::Channel::STABLE:
      id = IDS_APP_SHORTCUTS_SUBDIR_NAME_BRAVE_STABLE;
      break;
    case version_info::Channel::BETA:
      id = IDS_APP_SHORTCUTS_SUBDIR_NAME_BRAVE_BETA;
      break;
    case version_info::Channel::DEV:
      id = IDS_APP_SHORTCUTS_SUBDIR_NAME_BRAVE_DEV;
      break;
    case version_info::Channel::CANARY:
      id = IDS_APP_SHORTCUTS_SUBDIR_NAME_BRAVE_NIGHTLY;
      break;
    case version_info::Channel::UNKNOWN:
      id = IDS_APP_SHORTCUTS_SUBDIR_NAME_BRAVE_DEVELOPMENT;
      break;
    default:
      NOTREACHED() << "All possible channels are handled above.";
  }

  return brave_l10n::GetLocalizedResourceUTF16String(id);
}
}  // namespace shell_integration
#endif  // !BUILDFLAG(IS_WIN)
