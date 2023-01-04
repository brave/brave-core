/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BROWSER_BROWSER_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BROWSER_BROWSER_MANAGER_H_

#include "base/observer_list.h"
#include "bat/ads/internal/browser/browser_manager_observer.h"

namespace ads {

class BrowserManager final {
 public:
  BrowserManager();

  BrowserManager(const BrowserManager& other) = delete;
  BrowserManager& operator=(const BrowserManager& other) = delete;

  BrowserManager(BrowserManager&& other) noexcept = delete;
  BrowserManager& operator=(BrowserManager&& other) noexcept = delete;

  ~BrowserManager();

  static BrowserManager* GetInstance();

  static bool HasInstance();

  void AddObserver(BrowserManagerObserver* observer);
  void RemoveObserver(BrowserManagerObserver* observer);

  void OnBrowserDidBecomeActive();
  void OnBrowserDidResignActive();
  void SetBrowserIsActive(const bool is_active) { is_active_ = is_active; }
  bool IsBrowserActive() const { return is_active_ && is_in_foreground_; }

  void OnBrowserDidEnterForeground();
  void OnBrowserDidEnterBackground();
  void SetBrowserIsInForeground(const bool is_in_foreground) {
    is_in_foreground_ = is_in_foreground;
  }
  bool IsBrowserInForeground() const { return is_in_foreground_; }

 private:
  void NotifyBrowserDidBecomeActive() const;
  void NotifyBrowserDidResignActive() const;
  void LogBrowserActiveState() const;

  void NotifyBrowserDidEnterForeground() const;
  void NotifyBrowserDidEnterBackground() const;
  void LogBrowserForegroundState() const;

  base::ObserverList<BrowserManagerObserver> observers_;

  bool is_active_ = false;

  bool is_in_foreground_ = false;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BROWSER_BROWSER_MANAGER_H_
