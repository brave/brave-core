/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/buildflags/buildflags.h"

#define GetDefaultBrowser GetDefaultBrowser_ChromiumImpl
#include <chrome/browser/shell_integration_linux.cc>
#undef GetDefaultBrowser

namespace shell_integration {

bool IsAnyBraveBrowserDefaultBrowser() {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::MAY_BLOCK);

  std::vector<std::string> argv;
  argv.push_back(shell_integration_linux::kXdgSettings);
  argv.push_back("get");
  argv.push_back(shell_integration_linux::kXdgSettingsDefaultBrowser);

  std::string browser;
  // We don't care about the return value here.
  base::GetAppOutput(base::CommandLine(argv), &browser);

  // Only match the current brand's desktop file prefix so that Brave Origin
  // doesn't consider regular Brave as "another channel" and vice versa.
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  return browser.find("brave-origin") != std::string::npos;
#else
  return browser.find("brave-browser") != std::string::npos;
#endif
}

DefaultWebClientState GetDefaultBrowser() {
  // Check whether current install is default.
  auto state = GetDefaultBrowser_ChromiumImpl();
  if (state == IS_DEFAULT)
    return state;

  // Check other channel installs of the same brand are default.
  return IsAnyBraveBrowserDefaultBrowser() ? OTHER_MODE_IS_DEFAULT
                                           : NOT_DEFAULT;
}

}  // namespace shell_integration
