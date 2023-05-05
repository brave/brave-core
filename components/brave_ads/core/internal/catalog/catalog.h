/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_H_

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_observer.h"
#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"
#include "brave/components/brave_ads/core/internal/database/database_manager_observer.h"

namespace brave_ads {

struct CatalogInfo;

class Catalog final : public DatabaseManagerObserver {
 public:
  Catalog();

  Catalog(const Catalog&) = delete;
  Catalog& operator=(const Catalog&) = delete;

  Catalog(Catalog&&) noexcept = delete;
  Catalog& operator=(Catalog&&) noexcept = delete;

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
  void OnDidMigrateDatabase(int from_version, int to_version) override;

  base::ObserverList<CatalogObserver> observers_;

  bool is_fetching_ = false;

  Timer timer_;
  BackoffTimer retry_timer_;

  base::WeakPtrFactory<Catalog> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CATALOG_H_
