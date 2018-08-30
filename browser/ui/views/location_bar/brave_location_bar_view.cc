/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// We want the subclass to inherit from BraveLocationBar, not LocationBar
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "brave/browser/ui/location_bar/brave_location_bar.h"
#define LocationBar BraveLocationBar

#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"

void BraveLocationBarView::Init() {
  // base method calls Update and Layout
  LocationBarView::Init();
  // brave action buttons
  brave_actions_ = new BraveActionsContainer(browser_, profile());
  brave_actions_->Init();
  AddChildView(brave_actions_);
  // Call Update again to cause a Layout
  Update(nullptr);
}

void BraveLocationBarView::Layout() {
  LocationBarView::Layout(brave_actions_ ? brave_actions_ : nullptr);
}

void BraveLocationBarView::Update(const content::WebContents* contents) {
  // base Init calls update before our Init is run, so our children
  // may not be initialized yet
  if (brave_actions_) {
    brave_actions_->Update();
  }
  LocationBarView::Update(contents);
}

void BraveLocationBarView::OnChanged() {
  if (brave_actions_) {
    // Do not show actions whilst omnibar is open or url is being edited
    const bool should_hide = GetToolbarModel()->input_in_progress() &&
                      !omnibox_view_->text().empty();
    brave_actions_->SetShouldHide(should_hide);
  }

  // OnChanged calls Layout
  LocationBarView::OnChanged();
}

gfx::Size BraveLocationBarView::CalculatePreferredSize() const {
  gfx::Size min_size = LocationBarView::CalculatePreferredSize();
  if (brave_actions_ && brave_actions_->visible()){
    const int brave_actions_min = brave_actions_->GetMinimumSize().width();
    const int extra_width = brave_actions_min +
                              GetLayoutConstant(LOCATION_BAR_ELEMENT_PADDING);
    min_size.Enlarge(extra_width, 0);
  }
  return min_size;
}

// Provide base class implementation for Update override that has been added to
// header via a patch. This should never be called as the only instantiated
// implementation should be our |BraveLocationBarView|.
void LocationBarView::Layout() {
  Layout(nullptr);
}
