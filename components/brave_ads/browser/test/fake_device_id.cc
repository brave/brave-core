/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/test/fake_device_id.h"

#include <utility>

#include "base/check.h"

namespace brave_ads::test {

FakeDeviceId::FakeDeviceId() = default;

FakeDeviceId::~FakeDeviceId() = default;

void FakeDeviceId::GetDeviceId(DeviceIdCallback callback) {
  callback_ = std::move(callback);
}

void FakeDeviceId::Complete() {
  CHECK(callback_);
  std::move(callback_).Run(/*device_id=*/"foo");
}

}  // namespace brave_ads::test
