/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_AD_TOOLTIP_PLATFORM_BRIDGE_H_
#define BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_AD_TOOLTIP_PLATFORM_BRIDGE_H_

#include <string>

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip.h"

namespace brave_ads {

class AdTooltipPlatformBridge {
 public:
  explicit AdTooltipPlatformBridge(Profile* profile);
  ~AdTooltipPlatformBridge();

  void ShowTooltip(brave_tooltips::BraveTooltip tooltip);
  void CloseTooltip(const std::string& tooltip_id);

 private:
  Profile* profile_ = nullptr;  // NOT OWNED

  AdTooltipPlatformBridge(const AdTooltipPlatformBridge&) = delete;
  AdTooltipPlatformBridge& operator=(const AdTooltipPlatformBridge&) = delete;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_AD_TOOLTIP_PLATFORM_BRIDGE_H_
