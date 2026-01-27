// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEB_APPLICATIONS_PWA_INSTALL_PAGE_ACTION_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEB_APPLICATIONS_PWA_INSTALL_PAGE_ACTION_H_

#include "components/prefs/pref_member.h"

// Add a BooleanPrefMember and a method to check the pref in UpdateVisibility().
// * Note that we wrap the BooleanPrefMember with a unique_ptr because we need
//   WebContents to init BooleanPrefMember. The member will be initialized in
//   the first call of CheckPinPwaInstallButtonPref().
#define will_deactivate_subscription_                                     \
  will_deactivate_subscription_;                                          \
  std::unique_ptr<BooleanPrefMember> pin_pwa_install_button_pref_member_; \
  bool CheckPinPwaInstallButtonPref(content::WebContents* web_contents)

#include <chrome/browser/ui/web_applications/pwa_install_page_action.h>  // IWYU pragma: export

#undef will_deactivate_subscription_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEB_APPLICATIONS_PWA_INSTALL_PAGE_ACTION_H_
