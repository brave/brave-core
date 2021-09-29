/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_CLIENT_HELPER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_CLIENT_HELPER_H_

namespace ads {

class AdsClient;

class AdsClientHelper final {
 public:
  explicit AdsClientHelper(AdsClient* ads_client);
  ~AdsClientHelper();

  static AdsClient* Get();

  static bool HasInstance();
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_CLIENT_HELPER_H_
