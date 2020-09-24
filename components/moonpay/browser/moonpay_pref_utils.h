/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MOONPAY_BROWSER_MOONPAY_PREF_UTILS_H_
#define BRAVE_COMPONENTS_MOONPAY_BROWSER_MOONPAY_PREF_UTILS_H_

class PrefRegistrySimple;

namespace moonpay {

class MoonpayPrefUtils {
 public:
  explicit MoonpayPrefUtils();
  ~MoonpayPrefUtils();

  static void RegisterPrefs(PrefRegistrySimple* registry);
};

}  // namespace moonpay

#endif  // BRAVE_COMPONENTS_MOONPAY_BROWSER_MOONPAY_PREF_UTILS_H_
