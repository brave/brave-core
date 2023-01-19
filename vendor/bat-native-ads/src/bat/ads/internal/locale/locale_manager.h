/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_LOCALE_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_LOCALE_MANAGER_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/internal/locale/locale_manager_observer.h"

namespace ads {

class LocaleManager final {
 public:
  LocaleManager();

  LocaleManager(const LocaleManager& other) = delete;
  LocaleManager& operator=(const LocaleManager& other) = delete;

  LocaleManager(LocaleManager&& other) noexcept = delete;
  LocaleManager& operator=(LocaleManager&& other) noexcept = delete;

  ~LocaleManager();

  static LocaleManager* GetInstance();

  static bool HasInstance();

  void AddObserver(LocaleManagerObserver* observer);
  void RemoveObserver(LocaleManagerObserver* observer);

  void OnLocaleDidChange(const std::string& locale);

 private:
  void NotifyLocaleDidChange(const std::string& locale) const;

  base::ObserverList<LocaleManagerObserver> observers_;

  std::string locale_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_LOCALE_MANAGER_H_
