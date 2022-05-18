/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVER_AD_SERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVER_AD_SERVER_H_

#include "base/observer_list.h"
#include "bat/ads/internal/ad_server/ad_server_observer.h"
#include "bat/ads/internal/base/backoff_timer.h"
#include "bat/ads/internal/base/timer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

class Catalog;

class AdServer final {
 public:
  AdServer();
  ~AdServer();

  void AddObserver(AdServerObserver* observer);
  void RemoveObserver(AdServerObserver* observer);

  void MaybeFetch();

 private:
  void Fetch();
  void OnFetch(const mojom::UrlResponse& url_response);

  void SaveCatalog(const Catalog& catalog);

  void FetchAfterDelay();

  void Retry();
  void OnRetry();

  void NotifyCatalogUpdated(const Catalog& catalog) const;
  void NotifyCatalogFailed() const;

  base::ObserverList<AdServerObserver> observers_;

  bool is_processing_ = false;

  Timer timer_;
  BackoffTimer retry_timer_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVER_AD_SERVER_H_
