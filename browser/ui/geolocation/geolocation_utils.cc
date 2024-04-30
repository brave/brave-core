/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/geolocation/geolocation_utils.h"

#include "build/build_config.h"

namespace geolocation {

#if BUILDFLAG(IS_LINUX)
bool IsSystemLocationSettingEnabled() {
  return false;
}
#endif

}  // namespace geolocation
