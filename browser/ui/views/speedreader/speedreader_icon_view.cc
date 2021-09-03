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
#include "ui/views/animation/ink_drop.h"
#include "ui/views/animation/ink_drop_host_view.h"
#include "ui/views/animation/ink_drop_state.h"

using DistillState = speedreader::DistillState;

SpeedreaderIconView::SpeedreaderIconView(
    CommandUpdater* command_updater,
    IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
    PageActionIconView::Delegate* page_action_icon_delegate,
    PrefService* pref_service)
    : PageActionIconView(command_updater,
                         IDC_SPEEDREADER_ICON_ONCLICK,
                         icon_label_bubble_delegate,
                         page_action_icon_delegate) {
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

  if (views::InkDrop::Get(this)->GetHighlighted() && !IsBubbleShowing()) {
    views::InkDrop::Get(this)->AnimateToState(views::InkDropState::HIDDEN,
                                              nullptr);
  }

  const ui::ThemeProvider* theme_provider = GetThemeProvider();
  const DistillState state = GetDistillState();
  const bool is_distilled = speedreader::PageStateIsDistilled(state);

  if (!is_distilled) {
    if (state == DistillState::kSpeedreaderOnDisabledPage ||
        state == DistillState::kPageProbablyReadable) {
      SetVisible(true);
    } else {
      SetVisible(false);
    }

    if (GetVisible()) {
      // Reset the icon color
      if (theme_provider) {
        SkColor icon_color_default =
            GetOmniboxColor(theme_provider, OmniboxPart::RESULTS_ICON);
        SetIconColor(icon_color_default);
      }
      UpdateIconImage();
    }
  }

  if (is_distilled) {
    UpdateIconImage();
    if (theme_provider)
      SetIconColor(theme_provider->GetColor(
          BraveThemeProperties::COLOR_SPEEDREADER_ICON));
    SetVisible(true);
  }
}

const gfx::VectorIcon& SpeedreaderIconView::GetVectorIcon() const {
  const DistillState state = GetDistillState();
  if (state == DistillState::kSpeedreaderMode ||
      state == DistillState::kSpeedreaderOnDisabledPage) {
    return kBraveSpeedreaderModeIcon;
  } else {
    return kBraveReaderModeIcon;
  }
}

std::u16string SpeedreaderIconView::GetTextForTooltipAndAccessibleName() const {
  int id;
  const DistillState state = GetDistillState();
  switch (state) {
    case DistillState::kSpeedreaderMode:
    case DistillState::kSpeedreaderOnDisabledPage:
      id = IDS_SPEEDREADER_ICON_SPEEDREADER_SETTINGS;
      break;
    case DistillState::kReaderMode:
      id = IDS_SPEEDREADER_ICON_TURN_OFF_READER_MODE;
      break;
    default:
      id = IDS_SPEEDREADER_ICON_TURN_ON_READER_MODE;
  }
  return l10n_util::GetStringUTF16(id);
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

DistillState SpeedreaderIconView::GetDistillState() const {
  DistillState state = DistillState::kUnknown;
  auto* web_contents = GetWebContents();
  if (web_contents) {
    auto* tab_helper =
        speedreader::SpeedreaderTabHelper::FromWebContents(web_contents);
    if (tab_helper)
      state = tab_helper->PageDistillState();
  }
  return state;
}

BEGIN_METADATA(SpeedreaderIconView, PageActionIconView)
END_METADATA
