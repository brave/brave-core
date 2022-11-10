/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_OBSERVER_H_

#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace ads {

class AdsObserver : public bat_ads::mojom::BatAdsObserver {
 public:
  AdsObserver();

  AdsObserver(const AdsObserver& other) = delete;
  AdsObserver& operator=(const AdsObserver& other) = delete;

  AdsObserver(AdsObserver&& other) noexcept = delete;
  AdsObserver& operator=(AdsObserver&& other) noexcept = delete;

  ~AdsObserver() override;

  // Binds the receiver, connecting it to a new PendingRemote which is returned
  // for transmission elsewhere (typically to a Remote who will consume it to
  // start making calls).
  mojo::PendingRemote<bat_ads::mojom::BatAdsObserver> Bind();

  // Indicates whether the receiver is bound, meaning it may continue to receive
  // Interface method calls from a remote caller.
  bool IsBound() const { return receiver_.is_bound(); }

  // Resets the receiver to an unbound state. An unbound Receiver will NEVER
  // schedule method calls or disconnection notifications, and any pending tasks
  // which were scheduled prior to unbinding are effectively cancelled.
  void Reset() { receiver_.reset(); }

  // Invoked when ads has successfully initialized.
  void OnDidInitializeAds() override {}

  // Invoked when ads fail to initialize.
  void OnFailedToInitializeAds() override {}

  // Invoked when the statement of accounts have changed.
  void OnStatementOfAccountsDidChange() override {}

 private:
  mojo::Receiver<bat_ads::mojom::BatAdsObserver> receiver_{this};
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_ADS_OBSERVER_H_
