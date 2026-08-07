// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/page_action/onion_location_page_action_controller.h"

#include <optional>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/tor/onion_location_tab_helper.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_provider.h"
#include "url/origin.h"

namespace page_actions {

namespace {

constexpr SkColor kOnionButtonBackground = SkColorSetRGB(0x8c, 0x30, 0xbb);
constexpr SkColor kOnionButtonTextColor = SK_ColorWHITE;

}  // namespace

OnionLocationPageActionController::OnionLocationPageActionController(
    tabs::TabInterface& tab,
    page_actions::PageActionController& page_action_controller)
    : tab_(tab),
      page_action_controller_(
          static_cast<page_actions::PageActionControllerImpl&>(
              page_action_controller)) {}

OnionLocationPageActionController::~OnionLocationPageActionController() =
    default;

void OnionLocationPageActionController::Init() {
  did_activate_subscription_ = tab_->RegisterDidActivate(base::BindRepeating(
      [](OnionLocationPageActionController* self, tabs::TabInterface* tab) {
        self->AttachToWebContents(tab->GetContents());
        self->UpdatePageAction(tab->GetContents());
      },
      base::Unretained(this)));
  will_discard_contents_subscription_ =
      tab_->RegisterWillDiscardContents(base::BindRepeating(
          [](OnionLocationPageActionController* self, tabs::TabInterface*,
             content::WebContents*, content::WebContents* new_contents) {
            // TabInterface::GetContents() still returns the outgoing contents
            // at this point, so the swapped in contents has to be taken from
            // the argument.
            self->AttachToWebContents(new_contents);
            self->UpdatePageAction(new_contents);
          },
          base::Unretained(this)));
  AttachToWebContents(tab_->GetContents());
  UpdatePageAction(tab_->GetContents());
}

void OnionLocationPageActionController::ExecuteAction() {
  content::WebContents* const contents = tab_->GetContents();
  if (!contents) {
    return;
  }
  auto* helper = tor::OnionLocationTabHelper::FromWebContents(contents);
  if (!helper || !helper->should_show_icon()) {
    return;
  }

  std::optional<url::Origin> initiator_origin;
  if (contents->GetPrimaryMainFrame()->IsErrorDocument()) {
    // The error page's committed URL is the blocked .onion URL, which
    // shouldn't be used as the navigation initiator.
    initiator_origin = helper->initiator_origin();
  } else {
    initiator_origin = url::Origin::Create(contents->GetLastCommittedURL());
  }

  Profile* const profile =
      Profile::FromBrowserContext(contents->GetBrowserContext());
  TorProfileManager::SwitchToTorProfile(profile, helper->onion_location(),
                                        initiator_origin);
}

void OnionLocationPageActionController::AttachToWebContents(
    content::WebContents* contents) {
  if (web_contents() != contents) {
    Observe(contents);
  }
}

void OnionLocationPageActionController::UpdatePageAction(
    content::WebContents* contents) {
  if (!contents) {
    page_action_controller_->Hide(kActionShowOnionLocation);
    return;
  }

  auto* helper = tor::OnionLocationTabHelper::FromWebContents(contents);
  if (!helper || !helper->should_show_icon()) {
    page_action_controller_->Hide(kActionShowOnionLocation);
    page_action_controller_->ClearOverrideChipColors(kActionShowOnionLocation);
    page_action_controller_->SetAlwaysShowLabel(kActionShowOnionLocation,
                                                false);
    page_action_controller_->ClearOverrideText(kActionShowOnionLocation);
    return;
  }

  page_action_controller_->Show(kActionShowOnionLocation);

  const auto onion_location_text =
      base::UTF8ToUTF16(helper->onion_location().spec());
  const ui::ColorProvider& color_provider = contents->GetColorProvider();
  Profile* const profile =
      Profile::FromBrowserContext(contents->GetBrowserContext());

  if (profile->IsTor()) {
    page_action_controller_->OverrideImage(
        kActionShowOnionLocation,
        ui::ImageModel::FromVectorIcon(kLeoProductTorIcon,
                                       kOnionButtonTextColor));
    page_action_controller_->ShowSuggestionChip(kActionShowOnionLocation);
    page_action_controller_->SetAlwaysShowLabel(kActionShowOnionLocation, true);
    page_action_controller_->OverrideText(
        kActionShowOnionLocation,
        l10n_util::GetStringUTF16(IDS_LOCATION_BAR_ONION_AVAILABLE));
    page_action_controller_->OverrideTooltip(
        kActionShowOnionLocation,
        l10n_util::GetStringFUTF16(
            IDS_LOCATION_BAR_ONION_AVAILABLE_TOOLTIP_TEXT,
            onion_location_text));
    page_action_controller_->OverrideChipColors(kActionShowOnionLocation,
                                                kOnionButtonBackground,
                                                kOnionButtonTextColor);
  } else {
    page_action_controller_->OverrideImage(
        kActionShowOnionLocation,
        ui::ImageModel::FromVectorIcon(
            kLeoProductTorIcon,
            color_provider.GetColor(kColorOmniboxResultsIcon)));
    page_action_controller_->SetAlwaysShowLabel(kActionShowOnionLocation,
                                                false);
    page_action_controller_->ClearOverrideText(kActionShowOnionLocation);
    page_action_controller_->OverrideTooltip(
        kActionShowOnionLocation,
        l10n_util::GetStringFUTF16(IDS_LOCATION_BAR_OPEN_IN_TOR_TOOLTIP_TEXT,
                                   onion_location_text));
    page_action_controller_->ClearOverrideChipColors(kActionShowOnionLocation);
  }
}

void OnionLocationPageActionController::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }
  UpdatePageAction(web_contents());
}

}  // namespace page_actions
