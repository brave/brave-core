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
#include "brave/components/speedreader/features.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/dom_distiller/content/browser/distillable_page_utils.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/theme_provider.h"
#include "ui/views/animation/ink_drop_state.h"
#include "ui/views/metadata/metadata_impl_macros.h"

using DistillState = speedreader::SpeedreaderTabHelper::DistillState;

namespace {
SkColor kReaderIconColor = SkColorSetRGB(0x4c, 0x54, 0xd2);
}  // anonymous namespace

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

SpeedreaderIconView::~SpeedreaderIconView() {
  auto* contents = web_contents();
  if (contents)
    dom_distiller::RemoveObserver(contents, this);
  DCHECK(!DistillabilityObserver::IsInObserverList());
}

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

  if (GetHighlighted() && !IsBubbleShowing())
    AnimateInkDrop(views::InkDropState::HIDDEN, nullptr);

  auto* old_contents = web_contents();
  if (contents != old_contents) {
    if (old_contents)
      dom_distiller::RemoveObserver(old_contents, this);
    dom_distiller::AddObserver(contents, this);
  }

  auto* tab_helper =
      speedreader::SpeedreaderTabHelper::FromWebContents(contents);
  if (!tab_helper) {
    SetVisible(false);
    return;
  }

  const bool is_distilled = tab_helper->IsActiveForMainFrame();

  if (!is_distilled) {
    auto result = dom_distiller::GetLatestResult(contents);
    if (result) {
      const bool visible = result->is_last && result->is_distillable;
      SetVisible(visible);
      label()->SetVisible(false);

      if (GetVisible()) {
        // Reset the icon color
        const ui::ThemeProvider* tp = GetThemeProvider();
        SkColor icon_color_default =
            GetOmniboxColor(tp, OmniboxPart::RESULTS_ICON);
        SetIconColor(icon_color_default);
      }
    }
  }

  if (is_distilled) {
    const DistillState state = tab_helper->PageDistillState();
    DCHECK(state != DistillState::kNone);

    const int label_id = state == DistillState::kReaderMode
                             ? IDS_ICON_READER_MODE_LABEL
                             : IDS_ICON_SPEEDREADER_MODE_LABEL;
    SetLabel(l10n_util::GetStringUTF16(label_id));
    SetIconColor(kReaderIconColor);
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

SkColor SpeedreaderIconView::GetIconLabelBubbleSurroundingForegroundColor()
    const {
  // We can always return this since the text will be set to invisible on
  // non-readable pages.
  return kReaderIconColor;
}

SkColor SpeedreaderIconView::GetIconLabelBubbleInkDropColor() const {
  return kReaderIconColor;
}

SkColor SpeedreaderIconView::GetIconLabelBubbleBackgroundColor() const {
  return icon_label_bubble_delegate_->GetIconLabelBubbleBackgroundColor();
}

void SpeedreaderIconView::OnResult(
    const dom_distiller::DistillabilityResult& result) {
  Update();
}

BEGIN_METADATA(SpeedreaderIconView, PageActionIconView)
END_METADATA
