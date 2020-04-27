/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_MAC_H_
#define BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_MAC_H_

#include <string>

#include "bat/confirmations/internal/platform_helper.h"

namespace confirmations {

class PlatformHelperMac : public PlatformHelper {
 public:
  PlatformHelperMac(const PlatformHelperMac&) = delete;
  PlatformHelperMac& operator=(const PlatformHelperMac&) = delete;

  static PlatformHelperMac* GetInstanceImpl();

 private:
  friend struct base::DefaultSingletonTraits<PlatformHelperMac>;

  PlatformHelperMac();
  ~PlatformHelperMac() override;

  // PlatformHelper impl
  std::string GetPlatformName() const override;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_MAC_H_
