// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/common/tab_view.h"

#include <string>

#include "base/feature_list.h"
#include "base/notimplemented.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/vector_icon_types.h"

namespace tabs {
class TabInterface;
}  // namespace tabs

// Tree tab hooks. Defined in brave/browser/ui/views/tabs/tree_tab_view_utils.cc
// so that this upstream target doesn't depend on Brave targets.
namespace tree_tab_view_utils {
bool HasTreeTabDescendants(const tabs::TabInterface* tab);
bool IsTreeTabNodeCollapsed(const tabs::TabInterface* tab);
std::u16string GetTreeTabToggleButtonLabel(const tabs::TabInterface* tab);
void ToggleTreeTabNodeCollapsed(tabs::TabInterface* tab);
void AddTreeTabDescendantsToSelection(tabs::TabInterface* tab);
}  // namespace tree_tab_view_utils

#include <chrome/browser/ui/views/tabs/common/tab_view.cc>

// This method is not reachable now as we don't use this class for our vertical
// tab. Implement this when we use this class for our vertical tab.
bool TabView::IsTabMuteIndicatorNotClickable() {
  NOTIMPLEMENTED();
  return false;
}

// The tree tab collapse/expand button. Created and laid out by the plasters on
// tab_view.cc and tab_view_vertical_layout.cc.
void TabView::UpdateTreeToggleButton() {
  if (!tree_toggle_button_) {
    return;
  }

  const int icon_size = GetLayoutConstant(LayoutConstant::kTabCloseButtonSize);
  const SkColor icon_color =
      tab_styling()->CalculateTargetColors().foreground_color;
  const gfx::VectorIcon& icon =
      tree_tab_view_utils::IsTreeTabNodeCollapsed(GetTabInterface())
          ? vector_icons::kSubmenuArrowChromeRefreshOldIcon
          : vector_icons::kExpandMoreOldIcon;
  const ui::ImageModel image =
      ui::ImageModel::FromVectorIcon(icon, icon_color, icon_size);
  for (auto state : {views::Button::STATE_NORMAL, views::Button::STATE_HOVERED,
                     views::Button::STATE_PRESSED}) {
    tree_toggle_button_->SetImageModel(state, image);
  }

  const std::u16string label =
      tree_tab_view_utils::GetTreeTabToggleButtonLabel(GetTabInterface());
  tree_toggle_button_->SetTooltipText(label);
  tree_toggle_button_->GetViewAccessibility().SetName(label);

  InvalidateLayout();
}

bool TabView::ShouldShowTreeToggleButton() const {
  // Not a vertical tab, or tree tabs are disabled.
  if (!tree_toggle_button_) {
    return false;
  }
  // No room or no need for the button.
  if (IsClosing() || pinned_ || collapsed_) {
    return false;
  }
  if (!tree_tab_view_utils::HasTreeTabDescendants(GetTabInterface())) {
    return false;
  }

  // Same as Brave's own vertical tabs: Shown in place of the close button, and
  // always shown when collapsed.
  return tree_tab_view_utils::IsTreeTabNodeCollapsed(GetTabInterface()) ||
         hovered_ || active_;
}

void TabView::MaybeInitTreeToggleButton() {
  if (orientation_ != TabStripOrientation::kVertical ||
      !base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {
    return;
  }

  CHECK(!tree_toggle_button_);
  tree_toggle_button_ =
      AddChildView(std::make_unique<views::ImageButton>(base::BindRepeating(
          &TabView::OnTreeToggleButtonPressed, base::Unretained(this))));
  tree_toggle_button_->SetImageHorizontalAlignment(
      views::ImageButton::ALIGN_CENTER);
  tree_toggle_button_->SetImageVerticalAlignment(
      views::ImageButton::ALIGN_MIDDLE);
  tree_toggle_button_->SetPreferredSize(close_button_->GetPreferredSize());
  tree_toggle_button_->SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
  tree_toggle_button_->SetVisible(false);
}

void TabView::OnTreeToggleButtonPressed() {
  tree_tab_view_utils::ToggleTreeTabNodeCollapsed(
      const_cast<tabs::TabInterface*>(GetTabInterface()));
}

void TabView::MaybeSelectTreeTabDescendants(const ui::MouseEvent& event) {
  // Only on a plain left click. Modified clicks edit the selection as usual.
  if (!event.IsOnlyLeftMouseButton() || event.IsShiftDown() ||
      IsSelectionModifierDown(event)) {
    return;
  }
  // Not a vertical tab, or tree tabs are disabled.
  if (!tree_toggle_button_) {
    return;
  }

  tree_tab_view_utils::AddTreeTabDescendantsToSelection(
      const_cast<tabs::TabInterface*>(GetTabInterface()));
}
