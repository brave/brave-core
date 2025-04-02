/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define CurrentChannelLogoResourceId CurrentChannelLogoResourceId_Unused
#include "src/chrome/browser/ui/webui/current_channel_logo.cc"
#undef CurrentChannelLogoResourceId

namespace webui {

int CurrentChannelLogoResourceId() {
#if !BUILDFLAG(IS_ANDROID)
  switch (chrome::GetChannel()) {
#if defined(OFFICIAL_BUILD)
    case version_info::Channel::CANARY:
      return IDR_PRODUCT_LOGO_32_CANARY;
    case version_info::Channel::DEV:
      return IDR_PRODUCT_LOGO_32_DEV;
    case version_info::Channel::BETA:
      return IDR_PRODUCT_LOGO_32_BETA;
    case version_info::Channel::STABLE:
      return IDR_PRODUCT_LOGO_32;
#else
    case version_info::Channel::CANARY:
    case version_info::Channel::DEV:
    case version_info::Channel::BETA:
    case version_info::Channel::STABLE:
      NOTREACHED();
#endif
    case version_info::Channel::UNKNOWN:
      return IDR_PRODUCT_LOGO_32_DEVELOPMENT;
  }
#endif  // BUILDFLAG(IS_ANDROID)
  return -1;
}

}  // namespace webui
