// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/common/tab_view.h"

#include <unordered_set>
#include <utility>

#include "base/feature_list.h"
#include "base/notimplemented.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/views/tabs/tree_tab_view_utils.h"
#include "brave/components/tabs/public/tree_tab_node.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model_selection_state.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/models/image_model.h"

#define BRAVE_TAB_VIEW_CONSTRUCTOR                         \
  if (orientation_ == TabStripOrientation::kVertical &&    \
      base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) { \
    InitTreeToggleButton();                                \
  }

#define BRAVE_TAB_VIEW_UPDATE_COLORS UpdateTreeToggleButton();

// Select the tree tab subtree together so that dragging a tree tab parent
// moves its descendants as well. When it's not dragged, the selection is reset
// to the clicked tab on mouse release.
#define BRAVE_TAB_VIEW_ON_MOUSE_PRESSED                        \
  if (event.IsOnlyLeftMouseButton() && !event.IsShiftDown() && \
      !IsSelectionModifierDown(event)) {                       \
    SelectTreeTabDescendants();                                \
  }

#include <chrome/browser/ui/views/tabs/common/tab_view.cc>

#undef BRAVE_TAB_VIEW_ON_MOUSE_PRESSED
#undef BRAVE_TAB_VIEW_UPDATE_COLORS
#undef BRAVE_TAB_VIEW_CONSTRUCTOR

// This method is not reachable now as we don't use this class for our vertical
// tab. Implement this when we use this class for our vertical tab.
bool TabView::IsTabMuteIndicatorNotClickable() {
  NOTIMPLEMENTED();
  return false;
}

void TabView::InitTreeToggleButton() {
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

bool TabView::ShouldShowTreeToggleButton() const {
  if (!tree_toggle_button_ || IsClosing() || pinned_ || collapsed_) {
    return false;
  }

  const tabs::TreeTabNode* node =
      tree_tab_view_utils::GetTreeTabNodeForView(this);
  if (!node || node->height() <= 0) {
    // No descendants.
    return false;
  }

  // Same as Brave's own vertical tabs: Shown in place of the close button, and
  // always shown when collapsed.
  return node->collapsed() || hovered_ || active_;
}

void TabView::UpdateTreeToggleButton() {
  if (!tree_toggle_button_) {
    return;
  }

  if (const tabs::TreeTabNode* node =
          tree_tab_view_utils::GetTreeTabNodeForView(this)) {
    const int icon_size =
        GetLayoutConstant(LayoutConstant::kTabCloseButtonSize);
    const SkColor icon_color =
        tab_styling()->CalculateTargetColors().foreground_color;
    const auto& icon = node->collapsed()
                           ? vector_icons::kSubmenuArrowChromeRefreshOldIcon
                           : vector_icons::kExpandMoreOldIcon;
    const auto image =
        ui::ImageModel::FromVectorIcon(icon, icon_color, icon_size);
    for (auto state :
         {views::Button::STATE_NORMAL, views::Button::STATE_HOVERED,
          views::Button::STATE_PRESSED}) {
      tree_toggle_button_->SetImageModel(state, image);
    }
  }

  InvalidateLayout();
}

void TabView::OnTreeToggleButtonPressed() {
  const tabs::TreeTabNode* node =
      tree_tab_view_utils::GetTreeTabNodeForView(this);
  tabs::TabInterface* tab = const_cast<tabs::TabInterface*>(GetTabInterface());
  if (!node || !tab || !tab->GetBrowserWindowInterface()) {
    return;
  }

  // Tree tab nodes only exist with BraveTabStripModel.
  auto* model = static_cast<BraveTabStripModel*>(
      tab->GetBrowserWindowInterface()->GetTabStripModel());
  model->SetTreeTabNodeCollapsed(node->id(), !node->collapsed());
}

void TabView::SelectTreeTabDescendants() {
  if (!tree_toggle_button_ ||
      !tree_tab_view_utils::GetTreeTabNodeForView(this)) {
    return;
  }

  tabs::TabInterface* tab = const_cast<tabs::TabInterface*>(GetTabInterface());
  if (!tab || !tab->GetBrowserWindowInterface()) {
    return;
  }

  // Tree tab nodes only exist with BraveTabStripModel.
  auto* model = static_cast<BraveTabStripModel*>(
      tab->GetBrowserWindowInterface()->GetTabStripModel());
  const int index = model->GetIndexOfTab(tab);
  if (index == TabStripModel::kNoTab || !model->IsTabSelected(index)) {
    return;
  }

  std::unordered_set<tabs::TabInterface*> descendants;
  for (int descendant_index : model->GetTreeTabDescendantIndices(index)) {
    descendants.insert(model->GetTabAtIndex(descendant_index));
  }
  if (descendants.empty()) {
    return;
  }

  tabs::TabStripModelSelectionState new_selection = model->selection_model();
  new_selection.AppendTabsToSelection(descendants);
  model->SetSelectionFromModel(std::move(new_selection));
}
