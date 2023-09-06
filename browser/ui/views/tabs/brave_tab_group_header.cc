/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"

#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/tab_group_underline.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/background.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace {

SkColor GetGroupBackgroundColorForVerticalTabs(
    const tab_groups::TabGroupId& group_id,
    TabSlotController* controller) {
  DCHECK(base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
      << "This should be called only when the flag is on.";

  if (!controller->GetBrowser()
           ->tab_strip_model()
           ->group_model()
           ->ContainsTabGroup(group_id)) {
    // Can happen in tear-down.
    return gfx::kPlaceholderColor;
  }

  return controller->GetPaintedGroupColor(
      controller->GetGroupColorId(group_id));
}

}  // namespace

BraveTabGroupHeader::~BraveTabGroupHeader() = default;

void BraveTabGroupHeader::AddedToWidget() {
  TabGroupHeader::AddedToWidget();
  if (!ShouldShowVerticalTabs()) {
    return;
  }

  if (editor_bubble_tracker_.is_open()) {
    auto* bubble_delegate = editor_bubble_tracker_.widget()
                                ->widget_delegate()
                                ->AsBubbleDialogDelegate();
    DCHECK(bubble_delegate);
    // Technically, this can happen when tab strip's orientation changes
    // with the editor bubble open. It re-parents the widget which can cause
    // DCHECK failure. We should call SetAnchorView again to reset
    // the anchor widget.
    bubble_delegate->SetAnchorView(this);
  }
}

void BraveTabGroupHeader::VisualsChanged() {
  TabGroupHeader::VisualsChanged();
  if (!ShouldShowVerticalTabs()) {
    return;
  }

  title_->SetEnabledColor(GetGroupBackgroundColorForVerticalTabs(
      group().value(), base::to_address(tab_slot_controller_)));
  title_->SetSubpixelRenderingEnabled(false);

  auto font_list = title_->font_list();
  title_->SetFontList(font_list.DeriveWithWeight(gfx::Font::Weight::MEDIUM)
                          .DeriveWithSizeDelta(13 - font_list.GetFontSize()));

  // We don't draw background for vertical tabs.
  title_chip_->SetBackground(nullptr);

  LayoutTitleChip();
}

void BraveTabGroupHeader::Layout() {
  TabGroupHeader::Layout();
  if (!ShouldShowVerticalTabs()) {
    return;
  }

  LayoutTitleChip();
}

bool BraveTabGroupHeader::ShouldShowVerticalTabs() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    return false;
  }

  return tabs::utils::ShouldShowVerticalTabs(
      tab_slot_controller_->GetBrowser());
}

void BraveTabGroupHeader::LayoutTitleChip() {
  DCHECK(base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
      << "This should be called only when the flag is on.";

  auto title_bounds = GetContentsBounds();
  title_bounds.Inset(gfx::Insets(kPaddingForGroup * 2));
  title_chip_->SetBoundsRect(title_bounds);

  // |title_| is a child view of |title_chip_| and there could be |sync_icon_|
  // before |title_|. So expand |title_|'s width considering that.
  title_->SetSize({title_bounds.width() - title_->x(), title_->height()});
}

BEGIN_METADATA(BraveTabGroupHeader, TabGroupHeader)
END_METADATA
