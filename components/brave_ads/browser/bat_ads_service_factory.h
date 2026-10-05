/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BAT_ADS_SERVICE_FACTORY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BAT_ADS_SERVICE_FACTORY_H_

#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_ads {

class BatAdsServiceFactory {
 public:
  virtual ~BatAdsServiceFactory() = default;

  // Launches a new Bat Ads Service.
  virtual mojo::Remote<bat_ads::mojom::BatAdsService> Launch() = 0;

  // Invalidates the in-flight `Launch`. A bind that has not yet run becomes
  // a no-op; a bind that already ran has its service closed instead of
  // being left running after the caller has moved on.
  virtual void Invalidate() = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_BAT_ADS_SERVICE_FACTORY_H_
