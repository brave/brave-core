/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_DEVICE_ID_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_DEVICE_ID_H_

#include "brave/components/brave_ads/browser/device_id/device_id.h"

namespace brave_ads::test {

// Defers the `GetDeviceId` callback until `Complete` is called. Because the
// callback installs pref change registrars and triggers the first
// `MaybeStartBatAdsService` evaluation, this lets tests control when the entire
// startup sequence begins.
class FakeDeviceId : public DeviceId {
 public:
  FakeDeviceId();

  FakeDeviceId(const FakeDeviceId&) = delete;
  FakeDeviceId& operator=(const FakeDeviceId&) = delete;

  ~FakeDeviceId() override;

  // DeviceId:
  void GetDeviceId(DeviceIdCallback callback) override;

  // Runs the captured callback with a device ID, completing the pending
  // request.
  void Complete();

 private:
  DeviceIdCallback callback_;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_DEVICE_ID_H_
