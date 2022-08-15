/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_H_

#include "base/observer_list.h"
#include "bat/ads/internal/base/timer/backoff_timer.h"
#include "bat/ads/internal/base/timer/timer.h"
#include "bat/ads/internal/catalog/catalog_observer.h"
#include "bat/ads/internal/database/database_manager_observer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

struct CatalogInfo;

class Catalog final : public DatabaseManagerObserver {
 public:
  Catalog();
  Catalog(const Catalog&) = delete;
  Catalog& operator=(const Catalog&) = delete;
  ~Catalog() override;

  void AddObserver(CatalogObserver* observer);
  void RemoveObserver(CatalogObserver* observer);

  void MaybeFetch();

 private:
  void Fetch();
  void OnFetch(const mojom::UrlResponseInfo& url_response);
  void FetchAfterDelay();

  void Retry();
  void OnRetry();

  void NotifyDidUpdateCatalog(const CatalogInfo& catalog) const;
  void NotifyFailedToUpdateCatalog() const;

  // DatabaseManagerObserver:
  void OnDidMigrateDatabase(const int from_version,
                            const int to_version) override;

  base::ObserverList<CatalogObserver> observers_;

  bool is_processing_ = false;

  Timer timer_;
  BackoffTimer retry_timer_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_H_
