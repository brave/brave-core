// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/https_upgrade_exceptions/browser/https_upgrade_exceptions_service.h"
#include "brave/ios/browser/application_context/brave_application_context_impl.h"
#import "ios/chrome/browser/shared/model/prefs/pref_names.h"
#import "ios/components/security_interstitials/https_only_mode/feature.h"
#import "ios/components/security_interstitials/https_only_mode/https_upgrade_service.h"
#include "net/base/features.h"
#include "net/base/url_util.h"

namespace {
bool CanUpgradeToHTTPS(const GURL& url) {
  // FIXME: Move this impl out of the chromium_src override
  BraveApplicationContextImpl* braveContext =
      static_cast<BraveApplicationContextImpl*>(GetApplicationContext());
  return braveContext->https_upgrade_exceptions_service()->CanUpgradeToHTTPS(
      url);
}
}  // namespace

// Add checks for Brave-by-default feature flag, standard HTTPS upgrades pref
// and the brave https upgrade exception list when determining if the navigation
// should be upgraded
#define kHttpsUpgrades kHttpsUpgrades) && \
  !base::FeatureList::IsEnabled(net::features::kBraveHttpsByDefault) && \
  !(prefs_ && prefs_->GetBoolean(prefs::kHttpsUpgradesEnabled)
#define IsLocalhost IsLocalhost(url) || !CanUpgradeToHTTPS
#include "src/ios/chrome/browser/https_upgrades/model/https_only_mode_upgrade_tab_helper.mm"
#undef IsLocalhost
#undef kHttpsUpgrades
