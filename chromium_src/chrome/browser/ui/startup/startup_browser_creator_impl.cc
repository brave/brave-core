/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/startup/default_brave_browser_prompt.h"
#include "chrome/browser/ui/startup/google_api_keys_infobar_delegate.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

#define GoogleApiKeysInfoBarDelegate BraveGoogleKeysInfoBarDelegate

class BraveGoogleKeysInfoBarDelegate {
 public:
  static void Create(infobars::ContentInfoBarManager* infobar_manager) {
    // lulz
  }
};

#define BRAVE_STARTUPBROWSERCREATORIMPL_DETERMINEURLSANDLAUNCH \
  welcome_enabled = true;

#define ShowDefaultBrowserPrompt ShowDefaultBraveBrowserPrompt

#include "../../../../../../chrome/browser/ui/startup/startup_browser_creator_impl.cc"  // NOLINT

#undef BRAVE_STARTUPBROWSERCREATORIMPL_DETERMINEURLSANDLAUNCH
#undef GoogleApiKeysInfoBarDelegate
#undef ShowDefaultBrowserPrompt
