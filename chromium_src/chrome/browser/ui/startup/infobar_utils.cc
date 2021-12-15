/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/startup/default_brave_browser_prompt.h"
#include "chrome/browser/ui/session_crashed_bubble.h"
#include "chrome/browser/ui/startup/google_api_keys_infobar_delegate.h"
#include "components/infobars/content/content_infobar_manager.h"

class BraveGoogleKeysInfoBarDelegate {
 public:
  static void Create(infobars::ContentInfoBarManager* infobar_manager) {
    // lulz
  }
};

#define ShowIfNotOffTheRecordProfile ShowIfNotOffTheRecordProfileBrave
#define ShowDefaultBrowserPrompt ShowDefaultBraveBrowserPrompt
#define GoogleApiKeysInfoBarDelegate BraveGoogleKeysInfoBarDelegate

#include "src/chrome/browser/ui/startup/infobar_utils.cc"
#undef GoogleApiKeysInfoBarDelegate
#undef ShowDefaultBrowserPrompt
#undef ShowIfNotOffTheRecordProfile
