/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_BROWSER_BROWSER_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_BROWSER_BROWSER_MANAGER_H_

#include "base/observer_list.h"
#include "brave/components/brave_ads/core/internal/browser/browser_manager_observer.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"

namespace brave_ads {

class BrowserManager final : public AdsClientNotifierObserver {
 public:
  BrowserManager();

  BrowserManager(const BrowserManager&) = delete;
  BrowserManager& operator=(const BrowserManager&) = delete;

  BrowserManager(BrowserManager&&) noexcept = delete;
  BrowserManager& operator=(BrowserManager&&) noexcept = delete;

  ~BrowserManager() override;

  static BrowserManager& GetInstance();

  void AddObserver(BrowserManagerObserver* observer);
  void RemoveObserver(BrowserManagerObserver* observer);

  bool IsActive() const { return is_active_; }

  bool IsInForeground() const { return is_in_foreground_; }

 private:
  void NotifyBrowserDidBecomeActive() const;
  void NotifyBrowserDidResignActive() const;
  void LogBrowserActiveState() const;

  void NotifyBrowserDidEnterForeground() const;
  void NotifyBrowserDidEnterBackground() const;
  void LogBrowserForegroundState() const;

  // AdsClientNotifierObserver:
  void OnNotifyDidInitializeAds() override;
  void OnNotifyBrowserDidBecomeActive() override;
  void OnNotifyBrowserDidResignActive() override;
  void OnNotifyBrowserDidEnterForeground() override;
  void OnNotifyBrowserDidEnterBackground() override;

  base::ObserverList<BrowserManagerObserver> observers_;

  bool is_active_ = false;

  bool is_in_foreground_ = false;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_BROWSER_BROWSER_MANAGER_H_
