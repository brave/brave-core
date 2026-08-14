/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "ios/chrome/browser/https_upgrades/model/https_only_mode_upgrade_tab_helper.h"

#import "base/feature_list.h"
#import "base/functional/bind.h"
#import "base/functional/callback.h"
#import "base/logging.h"
#import "base/metrics/histogram_functions.h"
#import "base/strings/string_number_conversions.h"
#import "base/task/sequenced_task_runner.h"
#import "brave/components/https_upgrade_exceptions/browser/https_upgrade_exceptions_service.h"
#import "brave/ios/browser/https_upgrade_exceptions/https_upgrade_exceptions_service_accessor.h"
#import "brave/ios/browser/shared/prefs/pref_names.h"
#import "components/prefs/pref_service.h"
#import "components/security_interstitials/core/https_only_mode_metrics.h"
#import "ios/chrome/browser/prerender/model/prerender_tab_helper.h"
#import "ios/chrome/browser/shared/model/prefs/pref_names.h"
#import "ios/components/security_interstitials/https_only_mode/feature.h"
#import "ios/components/security_interstitials/https_only_mode/https_only_mode_blocking_page.h"
#import "ios/components/security_interstitials/https_only_mode/https_only_mode_container.h"
#import "ios/components/security_interstitials/https_only_mode/https_only_mode_controller_client.h"
#import "ios/components/security_interstitials/https_only_mode/https_only_mode_error.h"
#import "ios/components/security_interstitials/https_only_mode/https_upgrade_service.h"
#import "ios/web/public/navigation/navigation_context.h"
#import "ios/web/public/navigation/navigation_item.h"
#import "ios/web/public/navigation/navigation_manager.h"
#import "net/base/apple/url_conversions.h"
#import "net/base/features.h"
#import "net/base/url_util.h"
#import "url/gurl.h"
#import "url/url_constants.h"

namespace {

brave_shields::HttpsUpgradeLevel GetHttpsUpgradeLevel(PrefService* prefs) {
  if (!prefs) {
    return brave_shields::HttpsUpgradeLevel::kDisabled;
  }
  return static_cast<brave_shields::HttpsUpgradeLevel>(
      prefs->GetInteger(prefs::kHttpsUpgradeLevel));
}

// Returns whether `url` should be upgraded for the level selected in Shields.
// Standard only upgrades hosts that aren't known to be broken over HTTPS,
// strict upgrades every host.
bool BraveShouldUpgradeToHttps(PrefService* prefs, const GURL& url) {
  if (!base::FeatureList::IsEnabled(net::features::kBraveHttpsByDefault)) {
    return false;
  }
  switch (GetHttpsUpgradeLevel(prefs)) {
    case brave_shields::HttpsUpgradeLevel::kDisabled:
      return false;
    case brave_shields::HttpsUpgradeLevel::kStandard: {
      auto* exceptions_service =
          https_upgrade_exceptions::GetHttpsUpgradeExceptionsService();
      return exceptions_service && exceptions_service->CanUpgradeToHTTPS(url);
    }
    case brave_shields::HttpsUpgradeLevel::kStrict:
      return true;
  }
}

}  // namespace

// Shields decides which navigations are upgraded instead of the HTTPS-Upgrades
// feature, which stays disabled.
#define IsEnabled(FEATURE) IsEnabled(net::features::kBraveHttpsByDefault)
#define IsLocalhost(URL) \
  IsLocalhost(URL) || !BraveShouldUpgradeToHttps(prefs_, URL)

// An interstitial is only shown when an upgraded navigation fails in strict
// mode, standard mode falls back to HTTP silently.
#define GetBoolean(PREF)                   \
  GetInteger(prefs::kHttpsUpgradeLevel) == \
      static_cast<int>(brave_shields::HttpsUpgradeLevel::kStrict)

#include <ios/chrome/browser/https_upgrades/model/https_only_mode_upgrade_tab_helper.mm>

#undef GetBoolean
#undef IsLocalhost
#undef IsEnabled
