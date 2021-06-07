/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_STARTUP_BROWSER_CREATOR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_STARTUP_BROWSER_CREATOR_H_

#define GetPrivateProfileIfRequested     \
  GetPrivateProfileIfRequested_Unused(); \
  virtual Profile* GetPrivateProfileIfRequested

class BraveStartupBrowserCreator;

#include "../../../../../../chrome/browser/ui/startup/startup_browser_creator.h"

#undef GetPrivateProfileIfRequested

// Declare our subclass with the overridden method.
class BraveStartupBrowserCreator final : public StartupBrowserCreator {
 public:
  Profile* GetPrivateProfileIfRequested(const base::CommandLine& command_line,
                                        Profile* profile) override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_STARTUP_BROWSER_CREATOR_H_
