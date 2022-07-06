/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_LOCALE_MANAGER_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_LOCALE_MANAGER_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"

namespace ads {

class LocaleManagerObserver : public base::CheckedObserver {
 public:
  // Invoked when the |locale| did change.
  virtual void OnLocaleDidChange(const std::string& locale) {}

 protected:
  ~LocaleManagerObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_LOCALE_MANAGER_OBSERVER_H_
