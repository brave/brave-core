/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision.h"

#include <optional>

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_path_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

namespace brave_ads {

namespace {

bool DoesRequireResourceForNewTabPageAds() {
  // Require resource only if:
  // - The user has opted into new tab page ads and has either joined Brave
  //   Rewards or new tab page ad events should always be triggered.
  return UserHasOptedInToNewTabPageAds() &&
         (UserHasJoinedBraveRewards() ||
          ShouldAlwaysTriggerNewTabPageAdEvents());
}

bool DoesRequireResource() {
  // Require resource only if:
  // - The user has opted into Brave News ads.
  // - The user has opted into new tab page ads and has either joined Brave
  //   Rewards or new tab page ad events should always be triggered.
  // - The user has joined Brave Rewards and opted into notification ads.
  return UserHasOptedInToBraveNewsAds() ||
         DoesRequireResourceForNewTabPageAds() ||
         UserHasOptedInToNotificationAds();
}

}  // namespace

Subdivision::Subdivision() {
  GetAdsClient().AddObserver(this);
}

Subdivision::~Subdivision() {
  GetAdsClient().RemoveObserver(this);
}

void Subdivision::AddObserver(SubdivisionObserver* const observer) {
  observers_.AddObserver(observer);
}

void Subdivision::RemoveObserver(SubdivisionObserver* const observer) {
  observers_.RemoveObserver(observer);
}

///////////////////////////////////////////////////////////////////////////////

void Subdivision::Initialize() {
  MaybeRequireSubdivision();

  MaybePeriodicallyFetchSubdivision();
}

void Subdivision::MaybeRequireSubdivision() {
  DoesRequireResource() ? InitializeSubdivisionUrlRequest()
                        : ShutdownSubdivisionUrlRequest();
}

void Subdivision::InitializeSubdivisionUrlRequest() {
  if (!subdivision_url_request_) {
    BLOG(1, "Initialize subdivision URL request");
    subdivision_url_request_ = std::make_unique<SubdivisionUrlRequest>();
    subdivision_url_request_->SetDelegate(this);
  }
}

void Subdivision::ShutdownSubdivisionUrlRequest() {
  if (subdivision_url_request_) {
    subdivision_url_request_.reset();
    BLOG(1, "Shutdown subdivision URL request");
  }
}

void Subdivision::MaybePeriodicallyFetchSubdivision() {
  if (subdivision_url_request_) {
    subdivision_url_request_->PeriodicallyFetch();
  }
}

void Subdivision::NotifyDidUpdateSubdivision(const std::string& subdivision) {
  for (auto& observer : observers_) {
    observer.OnDidUpdateSubdivision(subdivision);
  }
}

void Subdivision::OnNotifyDidInitializeAds() {
  Initialize();
}

void Subdivision::OnNotifyPrefDidChange(const std::string& path) {
  if (DoesMatchUserHasJoinedBraveRewardsPrefPath(path) ||
      DoesMatchUserHasOptedInToBraveNewsAdsPrefPath(path) ||
      DoesMatchUserHasOptedInToNewTabPageAdsPrefPath(path) ||
      DoesMatchUserHasOptedInToNotificationAdsPrefPath(path)) {
    // This condition should include all the preferences that are present in the
    // `DoesRequireResource` function.
    Initialize();
  }
}

void Subdivision::OnDidFetchSubdivision(const std::string& subdivision) {
  CHECK(!subdivision.empty());

  const std::optional<std::string> country_code =
      GetSubdivisionCountryCode(subdivision);
  const std::optional<std::string> subdivision_code =
      GetSubdivisionCode(subdivision);

  if (!country_code || !subdivision_code) {
    return;
  }

  NotifyDidUpdateSubdivision(subdivision);
}

}  // namespace brave_ads
