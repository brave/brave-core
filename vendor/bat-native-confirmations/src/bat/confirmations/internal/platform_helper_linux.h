/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_LINUX_H_
#define BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_LINUX_H_

#include <string>

#include "bat/confirmations/internal/platform_helper.h"

namespace confirmations {

class PlatformHelperLinux : public PlatformHelper {
 public:
  PlatformHelperLinux(const PlatformHelperLinux&) = delete;
  PlatformHelperLinux& operator=(const PlatformHelperLinux&) = delete;

  static PlatformHelperLinux* GetInstanceImpl();

 private:
  friend struct base::DefaultSingletonTraits<PlatformHelperLinux>;

  PlatformHelperLinux();
  ~PlatformHelperLinux() override;

  // PlatformHelper impl
  std::string GetPlatformName() const override;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_LINUX_H_
