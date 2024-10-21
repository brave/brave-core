/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/onion_location_view.h"

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/tor/onion_location_tab_helper.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"

namespace {

constexpr SkColor kOnionButtonBackgound = SkColorSetRGB(0x8c, 0x30, 0xbb);
constexpr SkColor kOnionButtonTextColor = SK_ColorWHITE;
constexpr int kOnionButtonCornerRadius = 8;

}  // namespace

OnionLocationView::OnionLocationView(
    Profile* profile,
    IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
    PageActionIconView::Delegate* page_action_icon_delegate)
    : PageActionIconView(/*command_updater=*/nullptr,
                         /*command_id=*/0,
                         icon_label_bubble_delegate,
                         page_action_icon_delegate,
                         "Tor",
                         /*ephemeral*/ false),
      profile_(profile) {
  SetVisible(false);
}

OnionLocationView::~OnionLocationView() = default;

const gfx::VectorIcon& OnionLocationView::GetVectorIcon() const {
  return kLeoProductTorIcon;
}

void OnionLocationView::UpdateImpl() {
  if (!GetWebContents()) {
    SetVisible(false);
    return;
  }
  auto* helper = tor::OnionLocationTabHelper::FromWebContents(GetWebContents());
  const bool show_icon = helper && helper->should_show_icon();

  if (show_icon) {
    const auto onion_location_text =
        base::UTF8ToUTF16(helper->onion_location().spec());

    if (profile_->IsTor()) {
      SetIconColor(kOnionButtonTextColor);
      SetTextColor(views::Button::STATE_DISABLED, kOnionButtonTextColor);
      SetEnabledTextColors(kOnionButtonTextColor);
      SetBackground(views::CreateRoundedRectBackground(
          kOnionButtonBackgound, kOnionButtonCornerRadius));

      label()->SetVisible(true);
      SetLabel(brave_l10n::GetLocalizedResourceUTF16String(
                   IDS_LOCATION_BAR_ONION_AVAILABLE),
               l10n_util::GetStringFUTF16(
                   IDS_LOCATION_BAR_ONION_AVAILABLE_TOOLTIP_TEXT,
                   onion_location_text));
    } else {
      if (const ui::ColorProvider* color_provider = GetColorProvider()) {
        const SkColor icon_color_default =
            color_provider->GetColor(kColorOmniboxResultsIcon);
        SetIconColor(icon_color_default);
      }
      SetLabel({}, l10n_util::GetStringFUTF16(
                       IDS_LOCATION_BAR_OPEN_IN_TOR_TOOLTIP_TEXT,
                       onion_location_text));
    }
  }
  SetVisible(show_icon);
}

views::BubbleDialogDelegate* OnionLocationView::GetBubble() const {
  return nullptr;
}

void OnionLocationView::OnExecuting(
    PageActionIconView::ExecuteSource execute_source) {
  SetHighlighted(false);
  if (!GetVisible()) {
    return;
  }

  auto* helper = tor::OnionLocationTabHelper::FromWebContents(GetWebContents());
  if (helper) {
    TorProfileManager::SwitchToTorProfile(profile_, helper->onion_location());
  }
}

BEGIN_METADATA(OnionLocationView)
END_METADATA
