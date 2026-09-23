// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/youtube/youtube_network_change_observer.h"

#include "base/containers/fixed_flat_set.h"
#include "brave/ios/browser/youtube/pref_names.h"
#include "brave/ios/browser/youtube/youtube_quality_javascript_feature.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "net/base/network_change_notifier.h"

namespace youtube {

namespace {
constexpr auto kAllowedHosts = base::MakeFixedFlatSet<std::string_view>(
    {"m.youtube.com", "www.youtube.com", "youtube.com"});
}

YouTubeNetworkChangeObserver::YouTubeNetworkChangeObserver(
    web::WebState* web_state)
    : web_state_(web_state) {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

YouTubeNetworkChangeObserver::~YouTubeNetworkChangeObserver() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void YouTubeNetworkChangeObserver::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  if (web_state_->IsBeingDestroyed()) {
    return;
  }
  bool is_youtube =
      kAllowedHosts.contains(web_state_->GetLastCommittedURL().host());
  if (is_youtube && net::NetworkChangeNotifier::IsConnectionCellular(type)) {
    PrefService* prefs =
        ProfileIOS::FromBrowserState(web_state_->GetBrowserState())->GetPrefs();
    AutoQualityMode mode = static_cast<AutoQualityMode>(
        prefs->GetInteger(prefs::kAutoQualityMode));
    if (mode == AutoQualityMode::kWifiOnly) {
      YouTubeQualityJavaScriptFeature::GetInstance()->ResetQuality(web_state_);
    }
  }
}

}  // namespace youtube
