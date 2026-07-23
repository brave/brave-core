/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_SERVICES_BAT_ADS_SERVICE_LAUNCH_H_
#define BRAVE_BROWSER_BRAVE_ADS_SERVICES_BAT_ADS_SERVICE_LAUNCH_H_

#include "base/memory/ref_counted.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace brave_ads {

// Tracks a single `Launch` attempt, so a superseded one cannot leave two
// Bat Ads Service instances running at once. `Bind` and `Cancel` must only
// run as tasks on the same dedicated `SingleThreadTaskRunner`.
class BatAdsServiceLaunch final
    : public base::RefCountedThreadSafe<BatAdsServiceLaunch> {
 public:
  BatAdsServiceLaunch();

  void Bind(mojo::PendingReceiver<bat_ads::mojom::BatAdsService>
                bat_ads_service_pending_receiver);
  void Cancel();

 private:
  friend class base::RefCountedThreadSafe<BatAdsServiceLaunch>;
  ~BatAdsServiceLaunch();

  bool cancelled_ = false;
  mojo::SelfOwnedReceiverRef<bat_ads::mojom::BatAdsService> receiver_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_SERVICES_BAT_ADS_SERVICE_LAUNCH_H_
