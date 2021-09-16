/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BROWSER_MANAGER_BROWSER_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BROWSER_MANAGER_BROWSER_MANAGER_H_

namespace ads {

class BrowserManager final {
 public:
  BrowserManager();
  ~BrowserManager();

  BrowserManager(const BrowserManager&) = delete;
  BrowserManager& operator=(const BrowserManager&) = delete;

  static BrowserManager* Get();

  static bool HasInstance();

  void SetActive(const bool is_active);
  bool IsActive() const;
  void OnActive();
  void OnInactive();

  void SetForegrounded(const bool is_foregrounded);
  bool IsForegrounded() const;
  void OnForegrounded();
  void OnBackgrounded();

 private:
  bool is_active_ = false;

  bool is_foregrounded_ = false;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BROWSER_MANAGER_BROWSER_MANAGER_H_
