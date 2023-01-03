/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_H_

#include <string>

#include "base/no_destructor.h"
#include "bat/ads/internal/common/platform/platform_helper_types.h"

namespace ads {

class PlatformHelper {
 public:
  virtual ~PlatformHelper();

  static PlatformHelper* GetInstance();

  static void SetForTesting(PlatformHelper* platform_helper);

  virtual bool IsMobile() const;
  virtual std::string GetName() const;
  virtual PlatformType GetType() const;

 protected:
  friend class base::NoDestructor<PlatformHelper>;

  PlatformHelper();

 private:
  static PlatformHelper* GetInstanceImpl();
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_PLATFORM_PLATFORM_HELPER_H_
