/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog.h"

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

namespace brave_ads {

namespace {

bool DoesRequireResourceForNewTabPageAds() {
  return UserHasOptedInToNewTabPageAds() &&
         (UserHasJoinedBraveRewards() ||
          ShouldAlwaysTriggerNewTabPageAdEvents());
}

bool DoesRequireResource() {
  return UserHasOptedInToBraveNewsAds() ||
         DoesRequireResourceForNewTabPageAds() ||
         UserHasOptedInToNotificationAds();
}

}  // namespace

Catalog::Catalog() {
  AddAdsClientNotifierObserver(this);
  DatabaseManager::GetInstance().AddObserver(this);
}

Catalog::~Catalog() {
  RemoveAdsClientNotifierObserver(this);
  DatabaseManager::GetInstance().RemoveObserver(this);
}

void Catalog::AddObserver(CatalogObserver* observer) {
  CHECK(observer);
  observers_.AddObserver(observer);
}

void Catalog::RemoveObserver(CatalogObserver* observer) {
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
  if (path == brave_rewards::prefs::kEnabled ||
      path == prefs::kOptedInToNotificationAds ||
      path == brave_news::prefs::kBraveNewsOptedIn ||
      path == brave_news::prefs::kNewTabPageShowToday ||
      path == ntp_background_images::prefs::kNewTabPageShowBackgroundImage ||
      path == ntp_background_images::prefs::
                  kNewTabPageShowSponsoredImagesBackgroundImage) {
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

void Catalog::OnDidMigrateDatabase(const int /*from_version=*/,
                                   const int /*to_version=*/) {
  ResetCatalog();
}

}  // namespace brave_ads
