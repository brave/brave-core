/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_OBSERVER_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_OBSERVER_MANAGER_H_

#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace ads {

class AdsObserverManager final {
 public:
  AdsObserverManager();

  AdsObserverManager(const AdsObserverManager& other) = delete;
  AdsObserverManager& operator=(const AdsObserverManager& other) = delete;

  AdsObserverManager(AdsObserverManager&& other) noexcept = delete;
  AdsObserverManager& operator=(AdsObserverManager&& other) noexcept = delete;

  ~AdsObserverManager();

  static AdsObserverManager* GetInstance();

  static bool HasInstance();

  void AddObserver(
      mojo::PendingRemote<bat_ads::mojom::BatAdsObserver> observer);

  // Invoked when the statement of accounts have changed.
  void NotifyStatementOfAccountsDidChange() const;

 private:
  mojo::RemoteSet<bat_ads::mojom::BatAdsObserver> observers_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_OBSERVER_MANAGER_H_
