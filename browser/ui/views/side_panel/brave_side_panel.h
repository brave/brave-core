// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_H_

#include <memory>
#include <optional>

#include "base/memory/raw_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "build/build_config.h"
#include "components/prefs/pref_member.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/resize_area_delegate.h"
#include "ui/views/view.h"
#include "ui/views/view_observer.h"

class BrowserView;
class SidePanelResizeWidget;
class ViewShadow;

namespace sidebar {
class SidebarBrowserTest;
}  // namespace sidebar

// Replacement for chromium's SidePanel which defines a
// unique inset and border style compared to Brave.
// SidebarContainerView controls this panel's visibility.
class BraveSidePanel : public views::View,
                       public views::ViewObserver,
                       public views::ResizeAreaDelegate {
  METADATA_HEADER(BraveSidePanel, views::View)
 public:
  // In the past we have been forced to rename these values, as this unscoped
  // enum ends up colliding with SDK constants on MacOS builds. However as these
  // constants are referred to in chromium code, renaming them causes a
  // growing number of substitution overrides to be necessary across the
  // codebase. As part of simplifying the overrides for this class, this
  // particular shadow warning is disabled on this case alone, to permit this
  // enum to be used here.
#if BUILDFLAG(IS_MAC)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#endif  // BUILDFLAG(IS_MAC)
  // Determines the side from which the side panel will appear.
  // LTR / RTL conversions are handled in
  // BrowserViewLayout::LayoutSidePanelView. As such, left will always be on the
  // left side of the browser regardless of LTR / RTL mode.
  enum HorizontalAlignment { kAlignLeft = 0, kAlignRight };
#if BUILDFLAG(IS_MAC)
#pragma clang diagnostic pop
#endif  // BUILDFLAG(IS_MAC)

  // Same signature as chromium SidePanel
  explicit BraveSidePanel(BrowserView* browser_view,
                          HorizontalAlignment horizontal_alignment =
                              HorizontalAlignment::kAlignLeft);
  BraveSidePanel(const BraveSidePanel&) = delete;
  BraveSidePanel& operator=(const BraveSidePanel&) = delete;
  ~BraveSidePanel() override;

  void SetPanelWidth(int width);
  double GetAnimationValue() const;
  void SetHorizontalAlignment(HorizontalAlignment alignment);
  HorizontalAlignment GetHorizontalAlignment();
  bool IsRightAligned();
  gfx::Size GetContentSizeUpperBound() const { return gfx::Size(); }
  bool IsClosing();
  void DisableAnimationsForTesting() {}

  void set_fixed_contents_width(std::optional<int> fixed_width) {
    fixed_contents_width_ = fixed_width;
  }

  // views::ResizeAreaDelegate:
  void OnResize(int resize_amount, bool done_resizing) override;
  void AddHeaderView(std::unique_ptr<views::View> view);

  // views::View:
  void OnThemeChanged() override;
  gfx::Size GetMinimumSize() const override;
  void AddedToWidget() override;
  void Layout(PassKey) override;

  void SetMinimumSidePanelContentsWidthForTesting(int width) {}

 private:
  friend class sidebar::SidebarBrowserTest;

  // views::ViewObserver:
  void OnChildViewAdded(View* observed_view, View* child) override;
  void OnChildViewRemoved(View* observed_view, View* child) override;
  void OnViewPropertyChanged(View* observed_view,
                             const void* key,
                             int64_t old_value) override;

  void UpdateBorder();
  void OnSidePanelWidthChanged();

  // Monitors addition of content view and change content view property that
  // signals toggling of the view's visiblity.
  base::ScopedMultiSourceObservation<View, ViewObserver> scoped_observation_{
      this};

  HorizontalAlignment horizontal_alignment_ = kAlignLeft;
  std::optional<int> starting_width_on_resize_;

  // If this is set, use this width for panel contents during the layout
  // instead of using this panel's bounds. This is used to prevent panel
  // contents layout while sidebar show/hide animation is in-progress.
  std::optional<int> fixed_contents_width_;
  raw_ptr<BrowserView> browser_view_ = nullptr;
  IntegerPrefMember side_panel_width_;
  std::unique_ptr<SidePanelResizeWidget> resize_widget_;
  std::unique_ptr<ViewShadow> shadow_;
  std::unique_ptr<views::View> header_view_;
};

// Alias to the original `SidePanel` to the benefit of upstream code, as
// `BraveSidePanel` is a complete replacement of the upstream class.
using SidePanel = BraveSidePanel;

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_BRAVE_SIDE_PANEL_H_
