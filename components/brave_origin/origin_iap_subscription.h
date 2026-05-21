/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ORIGIN_ORIGIN_IAP_SUBSCRIPTION_H_
#define BRAVE_COMPONENTS_BRAVE_ORIGIN_ORIGIN_IAP_SUBSCRIPTION_H_

#include "brave/components/brave_origin/mojom/brave_origin_settings.mojom.h"

class PrefService;

namespace brave_origin {

// OriginIAPSubscription is responsible for interaction between
// SubscriptionRenderFrameObserver (renderer process) and browser process
// for Origin subscription linking on Android.
class OriginIAPSubscription final
    : public brave_origin::mojom::OriginIAPSubscription {
 public:
  OriginIAPSubscription(const OriginIAPSubscription&) = delete;
  OriginIAPSubscription& operator=(const OriginIAPSubscription&) = delete;
  explicit OriginIAPSubscription(PrefService* prefs);
  ~OriginIAPSubscription() override;

  // brave_origin::mojom::OriginIAPSubscription
  void GetPurchaseTokenOrderId(
      GetPurchaseTokenOrderIdCallback callback) override;
  void SetLinkStatus(int32_t status) override;

 private:
  raw_ptr<PrefService> prefs_ = nullptr;
};

}  // namespace brave_origin

#endif  // BRAVE_COMPONENTS_BRAVE_ORIGIN_ORIGIN_IAP_SUBSCRIPTION_H_
