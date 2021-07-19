/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tooltips/ad_tooltip_platform_bridge.h"

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip_popup.h"

namespace brave_ads {

namespace {

// A BraveTooltipDelegate that passes through events to the ads service
class PassThroughBraveTooltipDelegate
    : public brave_tooltips::BraveTooltipDelegate {
 public:
  PassThroughBraveTooltipDelegate(Profile* profile,
                                  const brave_tooltips::BraveTooltip& tooltip)
      : profile_(profile), tooltip_(tooltip) {}

  void OnShow() override {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(profile_);
    DCHECK(ads_service);

    ads_service->OnShowTooltip(tooltip_.id());
  }

  void OnOkButtonPressed() override {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(profile_);
    DCHECK(ads_service);

    ads_service->OnOkButtonPressedForTooltip(tooltip_.id());
  }

  void OnCancelButtonPressed() override {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(profile_);
    DCHECK(ads_service);

    ads_service->OnCancelButtonPressedForTooltip(tooltip_.id());
  }

 protected:
  ~PassThroughBraveTooltipDelegate() override = default;

 private:
  Profile* profile_ = nullptr;  // NOT OWNED

  brave_tooltips::BraveTooltip tooltip_;

  PassThroughBraveTooltipDelegate(const PassThroughBraveTooltipDelegate&) =
      delete;
  PassThroughBraveTooltipDelegate& operator=(
      const PassThroughBraveTooltipDelegate&) = delete;
};

}  // namespace

AdTooltipPlatformBridge::AdTooltipPlatformBridge(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);
}

AdTooltipPlatformBridge::~AdTooltipPlatformBridge() = default;

void AdTooltipPlatformBridge::ShowTooltip(
    brave_tooltips::BraveTooltip tooltip) {
  // If there's no delegate, replace it with a PassThroughDelegate so clicks go
  // back to the appropriate handler
  tooltip.set_delegate(base::WrapRefCounted(
      new PassThroughBraveTooltipDelegate(profile_, tooltip)));

  brave_tooltips::BraveTooltipPopup::Show(profile_, tooltip);
}

void AdTooltipPlatformBridge::CloseTooltip(const std::string& tooltip_id) {
  brave_tooltips::BraveTooltipPopup::Close(tooltip_id,
                                           /* by_user */ false);
}

}  // namespace brave_ads
