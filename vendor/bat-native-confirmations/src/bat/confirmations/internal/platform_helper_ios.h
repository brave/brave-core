/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_IOS_H_
#define BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_IOS_H_

#include <string>

#include "bat/confirmations/internal/platform_helper.h"

namespace confirmations {

class PlatformHelperIos : public PlatformHelper {
 public:
  PlatformHelperIos(const PlatformHelperIos&) = delete;
  PlatformHelperIos& operator=(const PlatformHelperIos&) = delete;

  static PlatformHelperIos* GetInstanceImpl();

 private:
  friend struct base::DefaultSingletonTraits<PlatformHelperIos>;

  PlatformHelperIos();
  ~PlatformHelperIos() override;

  // PlatformHelper impl
  std::string GetPlatformName() const override;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_PLATFORM_HELPER_IOS_H_
