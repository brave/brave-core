/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_OPERATING_SYSTEM_OPERATING_SYSTEM_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_OPERATING_SYSTEM_OPERATING_SYSTEM_H_

#include <string>

#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_types.h"

namespace brave_ads {

class OperatingSystem {
 public:
  static const OperatingSystem& GetInstance();

  static void SetForTesting(const OperatingSystem* operating_system);

  OperatingSystem();

  OperatingSystem(const OperatingSystem&) = delete;
  OperatingSystem& operator=(const OperatingSystem&) = delete;

  virtual ~OperatingSystem();

  virtual std::string GetName() const;
  virtual OperatingSystemType GetType() const;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_OPERATING_SYSTEM_OPERATING_SYSTEM_H_
