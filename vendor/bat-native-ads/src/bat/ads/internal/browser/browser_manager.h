/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BROWSER_BROWSER_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BROWSER_BROWSER_MANAGER_H_

#include "base/observer_list.h"
#include "bat/ads/internal/browser/browser_manager_observer.h"

namespace ads {

class BrowserManager final {
 public:
  BrowserManager();
  ~BrowserManager();

  BrowserManager(const BrowserManager&) = delete;
  BrowserManager& operator=(const BrowserManager&) = delete;

  static BrowserManager* Get();

  static bool HasInstance();

  void AddObserver(BrowserManagerObserver* observer);
  void RemoveObserver(BrowserManagerObserver* observer);

  void OnDidBecomeActive();
  void OnDidResignActive();

  void OnDidEnterForeground();
  void OnDidEnterBackground();

  void SetActive(const bool is_active) { is_active_ = is_active; }

  bool IsActive() const { return is_active_ && is_foreground_; }

  void SetForeground(const bool is_foreground) {
    is_foreground_ = is_foreground;
  }

  bool IsForeground() const { return is_foreground_; }

 private:
  void NotifyBrowserDidBecomeActive() const;
  void NotifyBrowserDidResignActive() const;
  void NotifyBrowserDidEnterForeground() const;
  void NotifyBrowserDidEnterBackground() const;

  base::ObserverList<BrowserManagerObserver> observers_;

  bool is_active_ = false;

  bool is_foreground_ = false;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BROWSER_BROWSER_MANAGER_H_
