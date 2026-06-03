/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_resource.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
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

namespace brave_ads {

namespace {

bool DoesRequireResource() {
  // Require resource only if:
  // - The user has joined Brave Rewards and opted into notification ads.
  return UserHasOptedInToNotificationAds();
}

}  // namespace

CatalogResource::CatalogResource() {
  ads_client_observation_.Observe(&GetAdsClient());
  database_manager_observation_.Observe(&DatabaseManager::GetInstance());
}

CatalogResource::~CatalogResource() = default;

void CatalogResource::AddObserver(CatalogObserver* const observer) {
  CHECK(observer);

  observers_.AddObserver(observer);
}

void CatalogResource::RemoveObserver(CatalogObserver* const observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);
}

///////////////////////////////////////////////////////////////////////////////

void CatalogResource::Initialize() {
  MaybeRequireCatalog();
  MaybeFetchCatalog();
}

void CatalogResource::MaybeRequireCatalog() {
  DoesRequireResource() ? InitializeCatalogUrlRequest()
                        : ShutdownCatalogUrlRequest();
}

void CatalogResource::InitializeCatalogUrlRequest() {
  if (!catalog_url_request_) {
    BLOG(1, "Initialize catalog URL request");
    catalog_url_request_ = std::make_unique<CatalogUrlRequest>();
    catalog_url_request_->SetDelegate(this);
  }
}

void CatalogResource::ShutdownCatalogUrlRequest() {
  if (catalog_url_request_) {
    catalog_url_request_.reset();
    BLOG(1, "Shutdown catalog URL request");

    ResetCatalog(/*intentional*/ base::DoNothing());
  }
}

void CatalogResource::MaybeFetchCatalog() const {
  if (catalog_url_request_) {
    catalog_url_request_->PeriodicallyFetch();
  }
}

void CatalogResource::NotifyDidFetchCatalog(const CatalogInfo& catalog) {
  observers_.Notify(&CatalogObserver::OnDidFetchCatalog, catalog);
}

void CatalogResource::NotifyFailedToFetchCatalog() {
  observers_.Notify(&CatalogObserver::OnFailedToFetchCatalog);
}

void CatalogResource::OnNotifyDidInitializeAds() {
  Initialize();
}

void CatalogResource::OnNotifyPrefDidChange(const std::string& path) {
  if (DoesMatchUserHasJoinedBraveRewardsPrefPath(path) ||
      DoesMatchUserHasOptedInToNotificationAdsPrefPath(path)) {
    // This condition should include all the preferences that are present in the
    // `DoesRequireResource` function.
    Initialize();
  }
}

void CatalogResource::OnDidFetchCatalog(const CatalogInfo& catalog) {
  SetCatalogLastUpdated(base::Time::Now());

  // Update the ping after fetching, even if saving fails or the catalog is
  // unchanged, to allow server-side control.
  SetCatalogPing(catalog.ping);

  if (!HasCatalogChanged(catalog.id)) {
    return BLOG(1, "Catalog id " << catalog.id << " is up to date");
  }

  SaveCatalog(catalog,
              base::BindOnce(&CatalogResource::OnDidFetchCatalogCallback,
                             weak_factory_.GetWeakPtr(), catalog));
}

void CatalogResource::OnDidFetchCatalogCallback(const CatalogInfo& catalog,
                                                bool success) {
  if (success) {
    NotifyDidFetchCatalog(catalog);
  } else {
    NotifyFailedToFetchCatalog();
  }
}

void CatalogResource::OnFailedToFetchCatalog() {
  NotifyFailedToFetchCatalog();
}

void CatalogResource::OnDidMigrateDatabase(int /*from_version*/,
                                           int /*to_version*/) {
  ResetCatalog(/*intentional*/ base::DoNothing());
}

}  // namespace brave_ads
