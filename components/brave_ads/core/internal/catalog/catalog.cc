/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog.h"

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/brave_ads_feature.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/ads/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_request.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

namespace brave_ads {

namespace {

bool DoesRequireResource() {
  const bool is_required_for_new_tab_page_ads_when_always_triggering_ad_events =
      kShouldAlwaysRunService.Get() &&
      kShouldAlwaysTriggerNewTabPageAdEvents.Get() &&
      UserHasOptedInToNewTabPageAds();

  return UserHasOptedInToBravePrivateAds() || UserHasOptedInToBraveNews() ||
         is_required_for_new_tab_page_ads_when_always_triggering_ad_events;
}

}  // namespace

Catalog::Catalog() {
  AdsClientHelper::AddObserver(this);
  DatabaseManager::GetInstance().AddObserver(this);
}

Catalog::~Catalog() {
  AdsClientHelper::RemoveObserver(this);
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
  MaybeAllowCatalogRequest();
  MaybeFetchCatalog();
}

void Catalog::MaybeAllowCatalogRequest() {
  DoesRequireResource() ? InitializeCatalogRequest() : ShutdownCatalogRequest();
}

void Catalog::InitializeCatalogRequest() {
  if (!catalog_request_) {
    BLOG(1, "Initialize catalog request");
    catalog_request_ = std::make_unique<CatalogRequest>();
    catalog_request_->SetDelegate(this);
  }
}

void Catalog::ShutdownCatalogRequest() {
  if (catalog_request_) {
    catalog_request_.reset();
    BLOG(1, "Shutdown catalog request");

    ResetCatalog();
    BLOG(1, "Reset catalog");
  }
}

void Catalog::MaybeFetchCatalog() const {
  if (catalog_request_) {
    catalog_request_->PeriodicallyFetch();
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
  if (path == prefs::kEnabled || path == brave_news::prefs::kBraveNewsOptedIn ||
      path == brave_news::prefs::kNewTabPageShowToday ||
      path == ntp_background_images::prefs::kNewTabPageShowBackgroundImage ||
      path == ntp_background_images::prefs::
                  kNewTabPageShowSponsoredImagesBackgroundImage) {
    Initialize();
  }
}

void Catalog::OnDidFetchCatalog(const CatalogInfo& catalog) {
  BLOG(1, "Successfully fetched catalog");

  SetCatalogLastUpdated(base::Time::Now());

  if (!HasCatalogChanged(catalog.id)) {
    return BLOG(1, "Catalog id " << catalog.id << " is up to date");
  }

  SaveCatalog(catalog);

  NotifyDidUpdateCatalog(catalog);
}

void Catalog::OnFailedToFetchCatalog() {
  BLOG(1, "Failed to fetch catalog");

  NotifyFailedToUpdateCatalog();
}

void Catalog::OnDidMigrateDatabase(const int /*from_version*/,
                                   const int /*to_version*/) {
  ResetCatalog();
}

}  // namespace brave_ads
