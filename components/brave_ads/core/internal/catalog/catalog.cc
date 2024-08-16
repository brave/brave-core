/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog.h"

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_path_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

namespace brave_ads {

namespace {

bool DoesRequireResourceForNewTabPageAds() {
  // Require resource only if:
  // - The user has opted into new tab page ads and either joined Brave Rewards
  //   or new tab page ad events should always be triggered.
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

Catalog::Catalog() {
  GetAdsClient()->AddObserver(this);
  DatabaseManager::GetInstance().AddObserver(this);
}

Catalog::~Catalog() {
  GetAdsClient()->RemoveObserver(this);
  DatabaseManager::GetInstance().RemoveObserver(this);
}

void Catalog::AddObserver(CatalogObserver* const observer) {
  CHECK(observer);

  observers_.AddObserver(observer);
}

void Catalog::RemoveObserver(CatalogObserver* const observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);
}

///////////////////////////////////////////////////////////////////////////////

void Catalog::Initialize() {
  MaybeRequireCatalog();
  MaybeFetchCatalog();
}

void Catalog::MaybeRequireCatalog() {
  DoesRequireResource() ? InitializeCatalogUrlRequest()
                        : ShutdownCatalogUrlRequest();
}

void Catalog::InitializeCatalogUrlRequest() {
  if (!catalog_url_request_) {
    BLOG(1, "Initialize catalog URL request");
    catalog_url_request_ = std::make_unique<CatalogUrlRequest>();
    catalog_url_request_->SetDelegate(this);
  }
}

void Catalog::ShutdownCatalogUrlRequest() {
  if (catalog_url_request_) {
    catalog_url_request_.reset();
    BLOG(1, "Shutdown catalog URL request");

    ResetCatalog();
    BLOG(1, "Reset catalog");
  }
}

void Catalog::MaybeFetchCatalog() const {
  if (catalog_url_request_) {
    catalog_url_request_->PeriodicallyFetch();
  }
}

void Catalog::NotifyDidUpdateCatalog(const CatalogInfo& catalog) const {
  for (CatalogObserver& observer : observers_) {
    observer.OnDidUpdateCatalog(catalog);
  }
}

void Catalog::NotifyFailedToUpdateCatalog() const {
  for (CatalogObserver& observer : observers_) {
    observer.OnFailedToUpdateCatalog();
  }
}

void Catalog::OnNotifyDidInitializeAds() {
  Initialize();
}

void Catalog::OnNotifyPrefDidChange(const std::string& path) {
  if (DoesMatchUserHasJoinedBraveRewardsPrefPath(path) ||
      DoesMatchUserHasOptedInToBraveNewsAdsPrefPath(path) ||
      DoesMatchUserHasOptedInToNewTabPageAdsPrefPath(path) ||
      DoesMatchUserHasOptedInToNotificationAdsPrefPath(path)) {
    // This condition should include all the preferences that are present in the
    // `DoesRequireResource` function.
    Initialize();
  }
}

void Catalog::OnDidFetchCatalog(const CatalogInfo& catalog) {
  SetCatalogLastUpdated(base::Time::Now());

  if (!HasCatalogChanged(catalog.id)) {
    return BLOG(1, "Catalog id " << catalog.id << " is up to date");
  }

  SaveCatalog(catalog);

  NotifyDidUpdateCatalog(catalog);
}

void Catalog::OnFailedToFetchCatalog() {
  NotifyFailedToUpdateCatalog();
}

void Catalog::OnDidMigrateDatabase(const int /*from_version*/,
                                   const int /*to_version*/) {
  ResetCatalog();
}

}  // namespace brave_ads
