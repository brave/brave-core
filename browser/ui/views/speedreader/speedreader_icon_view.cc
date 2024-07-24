// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/speedreader/speedreader_icon_view.h"

#include <string>

#include "brave/app/brave_command_ids.h"
#include "brave/browser/speedreader/speedreader_tab_helper.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/speedreader/common/features.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/animation/ink_drop_host.h"
#include "ui/views/animation/ink_drop_state.h"

SpeedreaderIconView::SpeedreaderIconView(
    CommandUpdater* command_updater,
    IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
    PageActionIconView::Delegate* page_action_icon_delegate)
    : PageActionIconView(command_updater,
                         IDC_SPEEDREADER_ICON_ONCLICK,
                         icon_label_bubble_delegate,
                         page_action_icon_delegate,
                         "SpeedReader",
                         /*ephemeral*/ false) {
  SetVisible(false);
}

SpeedreaderIconView::~SpeedreaderIconView() = default;

void SpeedreaderIconView::UpdateImpl() {
  const auto state = GetDistillState();
  if (!speedreader::DistillStates::IsDistilled(state) &&
      !speedreader::DistillStates::IsDistillable(state)) {
    SetVisible(false);
    return;
  }

  if (views::InkDrop::Get(this)->GetHighlighted() && !IsBubbleShowing()) {
    views::InkDrop::Get(this)->AnimateToState(views::InkDropState::HIDDEN,
                                              nullptr);
  }

  if (const ui::ColorProvider* color_provider = GetColorProvider()) {
    if (speedreader::DistillStates::IsDistilled(state)) {
      const SkColor icon_color_active =
          color_provider->GetColor(kColorSpeedreaderIcon);
      SetIconColor(icon_color_active);
    } else {
      // Reset the icon color
      const SkColor icon_color_default =
          color_provider->GetColor(kColorOmniboxResultsIcon);
      SetIconColor(icon_color_default);
    }
  }

  UpdateIconImage();
  SetVisible(true);
}

bool SpeedreaderIconView::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyRightMouseButton() &&
      event.type() == ui::EventType::kMousePressed) {
    auto* web_contents = GetWebContents();
    if (!web_contents) {
      return PageActionIconView::OnMousePressed(event);
    }
    auto* tab_helper =
        speedreader::SpeedreaderTabHelper::FromWebContents(web_contents);
    if (!tab_helper) {
      return PageActionIconView::OnMousePressed(event);
    }
    tab_helper->ShowSpeedreaderBubble(
        speedreader::SpeedreaderBubbleLocation::kLocationBar);
  }
  return PageActionIconView::OnMousePressed(event);
}

const gfx::VectorIcon& SpeedreaderIconView::GetVectorIcon() const {
  return kLeoProductSpeedreaderIcon;
}

std::u16string SpeedreaderIconView::GetTextForTooltipAndAccessibleName() const {
  const auto state = GetDistillState();
  const int id = (speedreader::DistillStates::IsDistilled(state))
                     ? IDS_SPEEDREADER_ICON_TURN_OFF_READER_MODE
                     : IDS_SPEEDREADER_ICON_TURN_ON_READER_MODE;
  return brave_l10n::GetLocalizedResourceUTF16String(id);
}

void SpeedreaderIconView::OnExecuting(
    PageActionIconView::ExecuteSource execute_source) {}

views::BubbleDialogDelegate* SpeedreaderIconView::GetBubble() const {
  const auto* web_contents = GetWebContents();
  if (!web_contents) {
    return nullptr;
  }

  const auto* tab_helper =
      speedreader::SpeedreaderTabHelper::FromWebContents(web_contents);
  if (!tab_helper) {
    return nullptr;
  }

  return reinterpret_cast<LocationBarBubbleDelegateView*>(
      tab_helper->speedreader_bubble_view());
}

speedreader::DistillState SpeedreaderIconView::GetDistillState() const {
  auto* web_contents = GetWebContents();
  if (web_contents) {
    auto* tab_helper =
        speedreader::SpeedreaderTabHelper::FromWebContents(web_contents);
    if (tab_helper) {
      return tab_helper->PageDistillState();
    }
  }
  return {};
}

BEGIN_METADATA(SpeedreaderIconView)
END_METADATA
