/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_STUDIES_STUDIES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_STUDIES_STUDIES_H_

#include "base/scoped_observation.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

namespace brave_ads {

class AdsClient;

class Studies final : public AdsClientNotifierObserver {
 public:
  Studies();

  Studies(const Studies&) = delete;
  Studies& operator=(const Studies&) = delete;

  ~Studies() override;

 private:
  // AdsClientNotifierObserver:
  void OnNotifyDidInitializeAds() override;

  base::ScopedObservation<AdsClient, AdsClientNotifierObserver>
      ads_client_observation_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_STUDIES_STUDIES_H_
