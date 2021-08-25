/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tooltips/ads_tooltips_controller.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_ads/tooltips/ads_captcha_tooltip.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip_attributes.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip_popup.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
#include "brave/browser/brave_adaptive_captcha/brave_adaptive_captcha_service_factory.h"
#endif

namespace brave_ads {

AdsTooltipsController::AdsTooltipsController(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);
}

AdsTooltipsController::~AdsTooltipsController() = default;

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
void AdsTooltipsController::ShowCaptchaTooltip(const std::string& payment_id,
                                               const std::string& captcha_id,
                                               bool enable_cancel_button) {
  const std::u16string title = l10n_util::GetStringUTF16(
      IDS_BRAVE_ADS_SCHEDULED_CAPTCHA_NOTIFICATION_TITLE);
  const std::u16string body = l10n_util::GetStringUTF16(
      IDS_BRAVE_ADS_SCHEDULED_CAPTCHA_NOTIFICATION_BODY);
  const std::u16string ok_button_text = l10n_util::GetStringUTF16(
      IDS_BRAVE_ADS_SCHEDULED_CAPTCHA_NOTIFICATION_OK_BUTTON_TEXT);
  const std::u16string cancel_button_text = l10n_util::GetStringUTF16(
      IDS_BRAVE_ADS_SCHEDULED_CAPTCHA_NOTIFICATION_CANCEL_BUTTON_TEXT);

  brave_adaptive_captcha::BraveAdaptiveCaptchaService*
      brave_adaptive_captcha_service = nullptr;
  brave_adaptive_captcha_service =
      brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::GetForProfile(
          profile_);
  brave_tooltips::BraveTooltipAttributes tooltip_attributes(
      title, body, ok_button_text, cancel_button_text);
  tooltip_attributes.set_cancel_button_enabled(enable_cancel_button);
  auto captcha_tooltip = std::make_unique<AdsCaptchaTooltip>(
      brave_adaptive_captcha_service, tooltip_attributes, payment_id,
      captcha_id);

  // If there's no delegate, set one so that clicks go back to the appropriate
  // handler
  captcha_tooltip->set_delegate(AsWeakPtr());

  const std::string tooltip_id = captcha_tooltip->id();
  DCHECK(!tooltip_popups_[tooltip_id]);
  tooltip_popups_[tooltip_id] = new brave_tooltips::BraveTooltipPopup(
      profile_, std::move(captcha_tooltip));
}

void AdsTooltipsController::CloseCaptchaTooltip() {
  if (!tooltip_popups_[kScheduledCaptchaTooltipId]) {
    return;
  }

  tooltip_popups_[kScheduledCaptchaTooltipId]->Close(false);
}
#endif

void AdsTooltipsController::OnTooltipWidgetDestroyed(
    const std::string& tooltip_id) {
  DCHECK(!tooltip_id.empty());

  // Note: The pointed-to BraveTooltipPopup members are deallocated by their
  // containing Widgets
  tooltip_popups_.erase(tooltip_id);
}

}  // namespace brave_ads
