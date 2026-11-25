/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_vertical_tab_strip_scrollbar.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/ui/views/tabs/vertical/vertical_tab_strip_scroll_bar.h"
#include "components/prefs/pref_member.h"
#include "components/prefs/pref_service.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/view_utils.h"

// Deliberately not in an anonymous namespace so that views metadata works.
class BraveVerticalTabStripScrollBar : public VerticalTabStripScrollBar {
  METADATA_HEADER(BraveVerticalTabStripScrollBar, VerticalTabStripScrollBar)

 public:
  BraveVerticalTabStripScrollBar(
      tabs::VerticalTabStripStateController* state_controller,
      PrefService* prefs)
      : VerticalTabStripScrollBar(state_controller) {
    show_scroll_bar_.Init(
        brave_tabs::kVerticalTabsShowScrollbar, prefs,
        base::BindRepeating(&BraveVerticalTabStripScrollBar::OnPrefChanged,
                            base::Unretained(this)));
  }
  BraveVerticalTabStripScrollBar(const BraveVerticalTabStripScrollBar&) =
      delete;
  BraveVerticalTabStripScrollBar& operator=(
      const BraveVerticalTabStripScrollBar&) = delete;
  ~BraveVerticalTabStripScrollBar() override = default;

  bool show_scroll_bar() const { return *show_scroll_bar_; }

 private:
  void OnPrefChanged() {
    auto* scroll_view = views::AsViewClass<views::ScrollView>(parent());
    if (!scroll_view) {
      return;
    }

    scroll_view->SetVerticalScrollBarMode(
        GetBraveVerticalTabStripScrollBarMode(scroll_view));
    scroll_view->InvalidateLayout();
  }

  BooleanPrefMember show_scroll_bar_;
};

BEGIN_METADATA(BraveVerticalTabStripScrollBar)
END_METADATA

std::unique_ptr<VerticalTabStripScrollBar> CreateBraveVerticalTabStripScrollBar(
    tabs::VerticalTabStripStateController* state_controller,
    PrefService* prefs) {
  return std::make_unique<BraveVerticalTabStripScrollBar>(state_controller,
                                                          prefs);
}

views::ScrollView::ScrollBarMode GetBraveVerticalTabStripScrollBarMode(
    views::ScrollView* scroll_view) {
  const auto* scroll_bar = views::AsViewClass<BraveVerticalTabStripScrollBar>(
      scroll_view->vertical_scroll_bar());
  if (scroll_bar && !scroll_bar->show_scroll_bar()) {
    return views::ScrollView::ScrollBarMode::kHiddenButEnabled;
  }
  return views::ScrollView::ScrollBarMode::kEnabled;
}
