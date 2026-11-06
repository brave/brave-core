/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_INFOBAR_CONTAINER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_INFOBAR_CONTAINER_VIEW_H_

#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/geometry/rounded_corners_f.h"

class BraveInfoBarContainerView : public InfoBarContainerView {
  METADATA_HEADER(BraveInfoBarContainerView, InfoBarContainerView)
 public:
  explicit BraveInfoBarContainerView(
      infobars::InfoBarContainer::Delegate* delegate);
  BraveInfoBarContainerView(const BraveInfoBarContainerView&) = delete;
  BraveInfoBarContainerView& operator=(const BraveInfoBarContainerView&) =
      delete;
  ~BraveInfoBarContainerView() override;

  // views::View:
  void Layout(PassKey) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

  // InfoBarContainer:
  void PlatformSpecificRemoveInfoBar(infobars::InfoBar* infobar) override;

  // When `enabled`, replaces the flat bottom divider with a floating-card
  // look: margin from the window edges (applied by the browser view's layout;
  // see BraveBrowserViewTabbedLayoutImpl::AdjustInfobarLayout()) plus a
  // rounded-rect outline border. `contents_corner_radii` are the radii of the
  // infobars inside the outline; the outline itself is rounded by its own
  // thickness more, the same way the contents view's outline is.
  void SetRoundedBorderEnabled(
      bool enabled,
      const gfx::RoundedCornersF& contents_corner_radii);

 private:
  // The outline's radii, i.e. the contents radii grown by its own thickness.
  gfx::RoundedCornersF GetOutlineCornerRadii() const;

  void UpdateOutlineBorder();

  // Rounds the infobars' own corners to match the outline. They each paint to
  // an opaque layer that composites over everything the container painted, so
  // square layers would cover the outline's rounded corners.
  void UpdateInfoBarCornerRadii();

  bool rounded_border_enabled_ = false;
  gfx::RoundedCornersF contents_corner_radii_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_INFOBAR_CONTAINER_VIEW_H_
