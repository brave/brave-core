/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/test/fake_bat_ads_service.h"

#include <utility>

#include "base/functional/callback.h"

namespace brave_ads::test {

FakeBatAdsService::FakeBatAdsService(base::RepeatingClosure initialize_callback,
                                     base::RepeatingClosure shutdown_callback,
                                     bool simulate_initialization_failure)
    : shutdown_callback_(std::move(shutdown_callback)),
      bat_ads_(std::move(initialize_callback),
               simulate_initialization_failure) {}

FakeBatAdsService::~FakeBatAdsService() = default;

void FakeBatAdsService::BindReceiver(
    mojo::PendingReceiver<bat_ads::mojom::BatAdsService>
        bat_ads_service_pending_receiver) {
  bat_ads_service_receiver_.Bind(std::move(bat_ads_service_pending_receiver));
  // Notify the factory when `AdsServiceImpl` closes its end of the pipe so
  // tests can wait for the full shutdown sequence to complete.
  bat_ads_service_receiver_.set_disconnect_handler(shutdown_callback_);
}

void FakeBatAdsService::Create(
    const base::FilePath& /*service_path*/,
    mojo::PendingAssociatedRemote<bat_ads::mojom::BatAdsClient>
    /*bat_ads_client_pending_associated_remote*/,
    mojo::PendingAssociatedReceiver<bat_ads::mojom::BatAds>
        bat_ads_pending_associated_receiver,
    mojo::PendingReceiver<bat_ads::mojom::BatAdsClientNotifier>
        bat_ads_client_notifier_pending_receiver,
    CreateCallback callback) {
  bat_ads_.BindReceiver(std::move(bat_ads_pending_associated_receiver));
  bat_ads_client_notifier_.BindReceiver(
      std::move(bat_ads_client_notifier_pending_receiver));
  std::move(callback).Run();
}

}  // namespace brave_ads::test
