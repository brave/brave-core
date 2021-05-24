/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_CONTROLLER_H_
#define BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_CONTROLLER_H_

#include <map>
#include <memory>
#include <string>

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip.h"

namespace brave_tooltips {
class BraveTooltipPopup;
}  // namespace brave_tooltips

namespace brave_ads {

class AdsTooltipsController : public brave_tooltips::BraveTooltipDelegate {
 public:
  explicit AdsTooltipsController(Profile* profile);
  ~AdsTooltipsController() override;

  AdsTooltipsController(const AdsTooltipsController&) = delete;
  AdsTooltipsController& operator=(const AdsTooltipsController&) = delete;

  void ShowTooltip(std::unique_ptr<brave_tooltips::BraveTooltip> tooltip);
  void CloseTooltip(const std::string& tooltip_id);

 private:
  // brave_tooltips::BraveTooltipDelegate:
  void OnTooltipWidgetDestroyed(const std::string& tooltip_id) override;

  Profile* profile_ = nullptr;  // NOT OWNED
  std::map<std::string, brave_tooltips::BraveTooltipPopup* /* NOT OWNED */>
      tooltip_popups_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_CONTROLLER_H_
