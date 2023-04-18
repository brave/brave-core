/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_H_

#include <string>

#include "base/no_destructor.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_types.h"

namespace brave_ads {

class PlatformHelper {
 public:
  virtual ~PlatformHelper();

  static const PlatformHelper& GetInstance();

  static void SetForTesting(const PlatformHelper* platform_helper);

  virtual bool IsMobile() const;
  virtual std::string GetName() const;
  virtual PlatformType GetType() const;

 protected:
  friend class base::NoDestructor<PlatformHelper>;

  PlatformHelper();

 private:
  static const PlatformHelper& GetInstanceImpl();
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_H_
