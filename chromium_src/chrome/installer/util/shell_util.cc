/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/notreached.h"
#include "brave/installer/util/brave_shell_util.h"
#include "chrome/common/chrome_icon_resources_win.h"
#include "chrome/install_static/install_util.h"
#include "components/version_info/channel.h"

namespace {

#if defined(OFFICIAL_BUILD)

int GetIconIndexForFileType() {
  switch (install_static::GetChromeChannel()) {
    case version_info::Channel::STABLE:
      return icon_resources::kFileTypeIndex;
    case version_info::Channel::BETA:
      return icon_resources::kBetaFileTypeIndex;
    case version_info::Channel::DEV:
      return icon_resources::kDevFileTypeIndex;
    case version_info::Channel::CANARY:
      return icon_resources::kSxSFileTypeIndex;
    default:
      NOTREACHED();
      return icon_resources::kFileTypeIndex;
  }
}

#endif  // OFFICIAL_BUILD

}  // namespace

#define BRAVE_IPFS L"ipfs"
#define BRAVE_IPNS L"ipns"

#define BRAVE_GET_TARGET_FOR_DEFAULT_APP_SETTINGS                         \
  if (base::EqualsCaseInsensitiveASCII(protocol, BRAVE_IPFS))             \
    return base::StringPrintf(kSystemSettingsDefaultAppsFormat, L"IPFS"); \
  if (base::EqualsCaseInsensitiveASCII(protocol, BRAVE_IPNS))             \
    return base::StringPrintf(kSystemSettingsDefaultAppsFormat, L"IPNS");

#if defined(OFFICIAL_BUILD)
// Add BraveFile prog id in registry with proper icon.
// This prog id will be referenced from serveral file association reg entry.
#define BRAVE_GET_CHROME_PROG_ID_ENTRIES                     \
  app_info.prog_id = installer::GetProgIdForFileType();      \
  app_info.file_type_icon_index = GetIconIndexForFileType(); \
  GetProgIdEntries(app_info, entries);

// Give BraveXXFile prog id for some file type.(ex, .pdf or .svg) instead of
// BraveHTML.
#define BRAVE_GET_APP_EXT_REGISTRATION_ENTRIES                         \
  if (installer::ShouldUseFileTypeProgId(ext)) {                       \
    entries->push_back(std::make_unique<RegistryEntry>(                \
        key_name, installer::GetProgIdForFileType(), std::wstring())); \
    return;                                                            \
  }

// Give BraveXXFile prog id for some file type.(ex, .pdf or .svg) instead of
// BraveHTML.
#define BRAVE_GET_SHELL_INTEGRATION_ENTRIES                               \
  const std::wstring file_ext = ShellUtil::kPotentialFileAssociations[i]; \
  if (installer::ShouldUseFileTypeProgId(file_ext)) {                     \
    entries->push_back(std::make_unique<RegistryEntry>(                   \
        capabilities + L"\\FileAssociations", file_ext,                   \
        installer::GetProgIdForFileType()));                              \
    continue;                                                             \
  }
#else
#define BRAVE_GET_CHROME_PROG_ID_ENTRIES
#define BRAVE_GET_APP_EXT_REGISTRATION_ENTRIES
#define BRAVE_GET_SHELL_INTEGRATION_ENTRIES
#endif

#include "../../../../../chrome/installer/util/shell_util.cc"
#undef BRAVE_GET_SHELL_INTEGRATION_ENTRIES
#undef BRAVE_GET_APP_EXT_REGISTRATION_ENTRIES
#undef BRAVE_GET_CHROME_PROG_ID_ENTRIES
#undef BRAVE_GET_TARGET_FOR_DEFAULT_APP_SETTINGS
#undef BRAVE_IPFS
#undef BRAVE_IPNS
