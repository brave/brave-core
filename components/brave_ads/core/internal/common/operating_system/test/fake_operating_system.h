/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_OPERATING_SYSTEM_TEST_FAKE_OPERATING_SYSTEM_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_OPERATING_SYSTEM_TEST_FAKE_OPERATING_SYSTEM_H_

#include <string>

#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system.h"
#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_types.h"

namespace brave_ads {

// A test double for `OperatingSystem` that installs itself as the active
// instance on construction and restores the default on destruction. Call
// `SetType` to change the simulated operating system at any point during a
// test.
class FakeOperatingSystem : public OperatingSystem {
 public:
  FakeOperatingSystem();

  FakeOperatingSystem(const FakeOperatingSystem&) = delete;
  FakeOperatingSystem& operator=(const FakeOperatingSystem&) = delete;

  ~FakeOperatingSystem() override;

  void SetType(OperatingSystemType type);

  // OperatingSystem:
  std::string GetName() const override;
  OperatingSystemType GetType() const override;

 private:
  OperatingSystemType type_ = OperatingSystemType::kUnknown;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_OPERATING_SYSTEM_TEST_FAKE_OPERATING_SYSTEM_H_
