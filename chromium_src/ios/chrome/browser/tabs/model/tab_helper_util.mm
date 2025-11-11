/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "ios/chrome/browser/tabs/model/tab_helper_util.h"

#import "components/omnibox/common/omnibox_features.h"
#include "ios/chrome/browser/complex_tasks/model/ios_task_tab_helper.h"
#import "ios/chrome/browser/https_upgrades/model/https_only_mode_upgrade_tab_helper.h"
#import "ios/chrome/browser/https_upgrades/model/https_upgrade_service_factory.h"
#import "ios/chrome/browser/https_upgrades/model/typed_navigation_upgrade_tab_helper.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/tabs/model/ios_chrome_synced_tab_delegate.h"
#import "ios/components/security_interstitials/https_only_mode/https_only_mode_container.h"
#import "ios/components/security_interstitials/ios_blocking_page_tab_helper.h"

void AttachTabHelpers(web::WebState* web_state, TabHelperFilter filter_flags) {
  IOSTaskTabHelper::CreateForWebState(web_state);

  security_interstitials::IOSBlockingPageTabHelper::CreateForWebState(
      web_state);
}
