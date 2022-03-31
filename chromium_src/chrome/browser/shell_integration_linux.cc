/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define GetDefaultBrowser GetDefaultBrowser_ChromiumImpl
#include "src/chrome/browser/shell_integration_linux.cc"
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
  return browser.find("brave-browser") != std::string::npos;
}

DefaultWebClientState GetDefaultBrowser() {
  // Check whether current install is default.
  auto state = GetDefaultBrowser_ChromiumImpl();
  if (state == IS_DEFAULT)
    return state;

  // Check Other channel installs are default.
  return IsAnyBraveBrowserDefaultBrowser() ? OTHER_MODE_IS_DEFAULT
                                           : NOT_DEFAULT;
}

}  // namespace shell_integration
