// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/speedreader/speedreader_icon_view.h"

#include <string>

#include "base/feature_list.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/app/brave_command_ids.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/components/speedreader/features.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/theme_provider.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/animation/ink_drop_state.h"

using DistillState = speedreader::SpeedreaderTabHelper::DistillState;

SpeedreaderIconView::SpeedreaderIconView(
    CommandUpdater* command_updater,
    IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
    PageActionIconView::Delegate* page_action_icon_delegate,
    PrefService* pref_service)
    : PageActionIconView(command_updater,
                         IDC_SPEEDREADER_ICON_ONCLICK,
                         this, /* Make ourselves the icon bubble delegate */
                         page_action_icon_delegate),
      icon_label_bubble_delegate_(icon_label_bubble_delegate) {
  SetVisible(false);
}

SpeedreaderIconView::~SpeedreaderIconView() = default;

void SpeedreaderIconView::UpdateImpl() {
  if (!base::FeatureList::IsEnabled(speedreader::kSpeedreaderFeature)) {
    SetVisible(false);
    return;
  }

  auto* contents = GetWebContents();
  if (!contents || !contents->GetLastCommittedURL().SchemeIsHTTPOrHTTPS()) {
    SetVisible(false);
    return;
  }

  if (ink_drop()->GetHighlighted() && !IsBubbleShowing())
    ink_drop()->AnimateToState(views::InkDropState::HIDDEN, nullptr);

  auto* tab_helper =
      speedreader::SpeedreaderTabHelper::FromWebContents(contents);
  if (!tab_helper) {
    SetVisible(false);
    return;
  }

  const ui::ThemeProvider* theme_provider = GetThemeProvider();
  const DistillState state = tab_helper->PageDistillState();
  const bool is_distilled =
      speedreader::SpeedreaderTabHelper::PageStateIsDistilled(state);

  if (!is_distilled) {
    if (state == DistillState::kSpeedreaderOnDisabledPage) {
      SetLabel(l10n_util::GetStringUTF16(IDS_ICON_SPEEDREADER_MODE_LABEL));
      SetVisible(true);
      label()->SetVisible(true);
      UpdateLabelColors();
    } else if (state == DistillState::kPageProbablyReadable) {
      SetVisible(true);
      label()->SetVisible(false);
    } else {
      SetVisible(false);
      label()->SetVisible(false);
    }

    if (GetVisible()) {
      // Reset the icon color
      if (theme_provider) {
        SkColor icon_color_default =
            GetOmniboxColor(theme_provider, OmniboxPart::RESULTS_ICON);
        SetIconColor(icon_color_default);
      }
    }
  }

  if (is_distilled) {
    const int label_id = state == DistillState::kReaderMode
                             ? IDS_ICON_READER_MODE_LABEL
                             : IDS_ICON_SPEEDREADER_MODE_LABEL;
    SetLabel(l10n_util::GetStringUTF16(label_id));
    UpdateLabelColors();
    if (theme_provider)
      SetIconColor(theme_provider->GetColor(
          BraveThemeProperties::COLOR_SPEEDREADER_ICON));
    SetVisible(true);
    label()->SetVisible(true);
  }

  Observe(contents);
}

const gfx::VectorIcon& SpeedreaderIconView::GetVectorIcon() const {
  return kSpeedreaderIcon;
}

std::u16string SpeedreaderIconView::GetTextForTooltipAndAccessibleName() const {
  return l10n_util::GetStringUTF16(GetActive() ? IDS_EXIT_DISTILLED_PAGE
                                               : IDS_DISTILL_PAGE);
}

void SpeedreaderIconView::OnExecuting(
    PageActionIconView::ExecuteSource execute_source) {}

views::BubbleDialogDelegate* SpeedreaderIconView::GetBubble() const {
  auto* web_contents = GetWebContents();
  if (!web_contents)
    return nullptr;

  auto* tab_helper =
      speedreader::SpeedreaderTabHelper::FromWebContents(web_contents);
  if (!tab_helper)
    return nullptr;

  return reinterpret_cast<LocationBarBubbleDelegateView*>(
      tab_helper->speedreader_bubble_view());
}

SkColor SpeedreaderIconView::GetLabelColorOr(SkColor fallback) const {
  auto* web_contents = GetWebContents();
  if (!web_contents)
    return fallback;

  const ui::ThemeProvider* theme_provider = GetThemeProvider();
  if (!theme_provider)
    return fallback;

  auto* tab_helper =
      speedreader::SpeedreaderTabHelper::FromWebContents(web_contents);
  if (!tab_helper)
    return fallback;

  const DistillState state = tab_helper->PageDistillState();
  if (speedreader::SpeedreaderTabHelper::PageStateIsDistilled(state))
    return theme_provider->GetColor(
        BraveThemeProperties::COLOR_SPEEDREADER_ICON);

  return fallback;
}

SkColor SpeedreaderIconView::GetIconLabelBubbleSurroundingForegroundColor()
    const {
  const auto fallback = icon_label_bubble_delegate_
                            ->GetIconLabelBubbleSurroundingForegroundColor();
  return GetLabelColorOr(fallback);
}

SkColor SpeedreaderIconView::GetIconLabelBubbleInkDropColor() const {
  const auto fallback =
      icon_label_bubble_delegate_->GetIconLabelBubbleInkDropColor();
  return GetLabelColorOr(fallback);
}

SkColor SpeedreaderIconView::GetIconLabelBubbleBackgroundColor() const {
  return icon_label_bubble_delegate_->GetIconLabelBubbleBackgroundColor();
}

BEGIN_METADATA(SpeedreaderIconView, PageActionIconView)
END_METADATA
