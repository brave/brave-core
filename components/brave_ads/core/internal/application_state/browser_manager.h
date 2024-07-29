/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_APPLICATION_STATE_BROWSER_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_APPLICATION_STATE_BROWSER_MANAGER_H_

#include <optional>

#include "base/observer_list.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_manager_observer.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

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

  bool IsActive() const { return is_active_.value_or(false); }

  bool IsInForeground() const { return is_in_foreground_.value_or(false); }

 private:
  void NotifyBrowserDidBecomeActive() const;
  void NotifyBrowserDidResignActive() const;
  void LogBrowserActiveState() const;

  void NotifyBrowserDidEnterForeground() const;
  void NotifyBrowserDidEnterBackground() const;
  void InitializeBrowserBackgroundState();
  void LogBrowserBackgroundState() const;

  // AdsClientNotifierObserver:
  void OnNotifyDidInitializeAds() override;
  void OnNotifyBrowserDidBecomeActive() override;
  void OnNotifyBrowserDidResignActive() override;
  void OnNotifyBrowserDidEnterForeground() override;
  void OnNotifyBrowserDidEnterBackground() override;

  base::ObserverList<BrowserManagerObserver> observers_;

  std::optional<bool> is_active_;

  std::optional<bool> is_in_foreground_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_APPLICATION_STATE_BROWSER_MANAGER_H_
