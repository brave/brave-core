/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PLATFORM_PLATFORM_HELPER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PLATFORM_PLATFORM_HELPER_H_

#include <string>

#include "bat/ads/internal/platform/platform_helper_types.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace ads {

class PlatformHelper {
 public:
  PlatformHelper(const PlatformHelper&) = delete;
  PlatformHelper& operator=(const PlatformHelper&) = delete;

  static PlatformHelper* GetInstance();

  void SetForTesting(PlatformHelper* platform_helper);

  virtual bool IsMobile() const;
  virtual std::string GetPlatformName() const;
  virtual PlatformType GetPlatform() const;

 protected:
  friend struct base::DefaultSingletonTraits<PlatformHelper>;

  PlatformHelper();
  virtual ~PlatformHelper();

  static PlatformHelper* GetInstanceImpl();
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PLATFORM_PLATFORM_HELPER_H_
