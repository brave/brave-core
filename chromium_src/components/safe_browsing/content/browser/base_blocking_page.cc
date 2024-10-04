/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/security_interstitials/core/safe_browsing_loud_error_ui.h"

namespace {

constexpr char kSafeBrowsingHelpCenterURL[] =
    "https://support.brave.com/hc/en-us/articles/"
    "15222663599629-Safe-Browsing-in-Brave";

}  // namespace

namespace security_interstitials {

class BraveSafeBrowsingLoudErrorUI : public SafeBrowsingLoudErrorUI {
 public:
  using SafeBrowsingLoudErrorUI::SafeBrowsingLoudErrorUI;

  void HandleCommand(SecurityInterstitialCommand command) override {
    if (command == CMD_OPEN_HELP_CENTER) {
      controller()->OpenURL(should_open_links_in_new_tab(),
                            GURL(kSafeBrowsingHelpCenterURL));
    } else {
      SafeBrowsingLoudErrorUI::HandleCommand(command);
    }
  }
};

}  // namespace security_interstitials

#define SafeBrowsingLoudErrorUI BraveSafeBrowsingLoudErrorUI
#include "src/components/safe_browsing/content/browser/base_blocking_page.cc"
#undef SafeBrowsingLoudErrorUI
