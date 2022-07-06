/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PREFS_PREF_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PREFS_PREF_MANAGER_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/internal/prefs/pref_manager_observer.h"

namespace ads {

class PrefManager final {
 public:
  PrefManager();
  ~PrefManager();

  PrefManager(const PrefManager&) = delete;
  PrefManager& operator=(const PrefManager&) = delete;

  static PrefManager* GetInstance();

  static bool HasInstance();

  void AddObserver(PrefManagerObserver* observer);
  void RemoveObserver(PrefManagerObserver* observer);

  void OnPrefChanged(const std::string& path) const;

 private:
  void NotifyPrefChanged(const std::string& path) const;

  base::ObserverList<PrefManagerObserver> observers_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PREFS_PREF_MANAGER_H_
