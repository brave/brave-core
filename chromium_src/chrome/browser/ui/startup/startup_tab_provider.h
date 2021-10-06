/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_STARTUP_TAB_PROVIDER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_STARTUP_TAB_PROVIDER_H_

#define StartupTabProviderImpl ChromiumStartupTabProviderImpl
#include "../../../../../../chrome/browser/ui/startup/startup_tab_provider.h"
#undef StartupTabProviderImpl

class StartupTabProviderImpl : public ChromiumStartupTabProviderImpl {
 public:
  StartupTabProviderImpl() = default;
  StartupTabProviderImpl(const StartupTabProviderImpl&) = delete;
  StartupTabProviderImpl& operator=(const StartupTabProviderImpl&) = delete;

  StartupTabs GetCommandLineTabs(const base::CommandLine& command_line,
                                 const base::FilePath& cur_dir,
                                 Profile* profile) const override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_STARTUP_TAB_PROVIDER_H_
