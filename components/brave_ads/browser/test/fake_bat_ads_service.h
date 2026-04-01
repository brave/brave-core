/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_BAT_ADS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_BAT_ADS_SERVICE_H_

#include "base/functional/callback.h"
#include "brave/components/brave_ads/browser/test/fake_bat_ads.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_ads::test {

// Minimal in-process implementation of `bat_ads::mojom::BatAdsService`. Binds
// a `FakeBatAds` to the receiver from `Create` so `AdsServiceImpl` has a live
// mojo endpoint to send initialization messages to.
class FakeBatAdsService : public bat_ads::mojom::BatAdsService {
 public:
  FakeBatAdsService(base::RepeatingClosure initialize_callback,
                    base::RepeatingClosure shutdown_callback,
                    bool simulate_initialization_failure);

  FakeBatAdsService(const FakeBatAdsService&) = delete;
  FakeBatAdsService& operator=(const FakeBatAdsService&) = delete;

  ~FakeBatAdsService() override;

  void BindReceiver(mojo::PendingReceiver<bat_ads::mojom::BatAdsService>
                        bat_ads_service_mojom_pending_receiver);

  // bat_ads::mojom::BatAdsService:
  void Create(const base::FilePath& /*service_path*/,
              mojo::PendingAssociatedRemote<bat_ads::mojom::BatAdsClient>
              /*bat_ads_client_mojom_pending_associated_remote*/,
              mojo::PendingAssociatedReceiver<bat_ads::mojom::BatAds>
                  bat_ads_mojom_pending_associated_receiver,
              mojo::PendingReceiver<bat_ads::mojom::BatAdsClientNotifier>
              /*bat_ads_client_notifier_mojom_pending_receiver*/,
              CreateCallback callback) override;

 private:
  base::RepeatingClosure shutdown_callback_;

  FakeBatAds bat_ads_;
  mojo::Receiver<bat_ads::mojom::BatAdsService> receiver_{this};
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_BAT_ADS_SERVICE_H_
