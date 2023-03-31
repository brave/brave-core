/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_HELPER_H_

#include "brave/components/brave_ads/core/ads_client.h"

namespace brave_ads {

class AdsClientNotifierObserver;

class AdsClientHelper final {
 public:
  explicit AdsClientHelper(AdsClient* ads_client);

  AdsClientHelper(const AdsClientHelper& other) = delete;
  AdsClientHelper& operator=(const AdsClientHelper& other) = delete;

  AdsClientHelper(AdsClientHelper&& other) noexcept = delete;
  AdsClientHelper& operator=(AdsClientHelper&& other) noexcept = delete;

  ~AdsClientHelper();

  static AdsClient* GetInstance();

  static bool HasInstance();

  static void AddObserver(AdsClientNotifierObserver* observer);
  static void RemoveObserver(AdsClientNotifierObserver* observer);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_HELPER_H_
