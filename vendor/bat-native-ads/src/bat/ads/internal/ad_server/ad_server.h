/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_SERVER_AD_SERVER_H_
#define BAT_ADS_INTERNAL_AD_SERVER_AD_SERVER_H_

#include "bat/ads/internal/ad_server/ad_server_observer.h"
#include "bat/ads/internal/backoff_timer.h"
#include "bat/ads/internal/timer.h"
#include "bat/ads/mojom.h"

namespace ads {

class Catalog;

class AdServer {
 public:
  AdServer();

  ~AdServer();

  void AddObserver(
      AdServerObserver* observer);
  void RemoveObserver(
      AdServerObserver* observer);

  void MaybeFetch();

 private:
  base::ObserverList<AdServerObserver> observers_;

  bool is_processing_ = false;

  Timer timer_;

  void Fetch();
  void OnFetch(
      const UrlResponse& url_response);

  void SaveCatalog(
      const Catalog& catalog);

  BackoffTimer retry_timer_;
  void Retry();
  void OnRetry();

  void FetchAfterDelay();

  void NotifyCatalogUpdated(
      const Catalog& catalog);
  void NotifyCatalogFailed();
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_SERVER_AD_SERVER_H_
