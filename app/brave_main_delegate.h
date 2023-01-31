/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_APP_BRAVE_MAIN_DELEGATE_H_
#define BRAVE_APP_BRAVE_MAIN_DELEGATE_H_

#include "build/build_config.h"
#include "chrome/app/chrome_main_delegate.h"

// Chrome implementation of ContentMainDelegate.
class BraveMainDelegate : public ChromeMainDelegate {
 public:
  BraveMainDelegate(const BraveMainDelegate&) = delete;
  BraveMainDelegate& operator=(const BraveMainDelegate&) = delete;
  BraveMainDelegate();

  // |exe_entry_point_ticks| is the time at which the main function of the
  // executable was entered, or null if not available.
  explicit BraveMainDelegate(base::TimeTicks exe_entry_point_ticks);
  ~BraveMainDelegate() override;

 protected:
  // content::ContentMainDelegate implementation:
  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;
  content::ContentUtilityClient* CreateContentUtilityClient() override;
  void PreSandboxStartup() override;
  absl::optional<int> PostEarlyInitialization(
      ChromeMainDelegate::InvokedIn invoked_in) override;
};

#endif  // BRAVE_APP_BRAVE_MAIN_DELEGATE_H_
