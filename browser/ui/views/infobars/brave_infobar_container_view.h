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

  // InfoBarContainerView:
  void PlatformSpecificRemoveInfoBar(infobars::InfoBar* infobar) override;

  // views::View:
  void Layout(PassKey) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;

  // When enabled, hides the flat content separator and the solid background
  // in favor of a rounded outline border around the infobars.
  // `contents_corner_radii` are the radii of the contents inside the
  // outline; the outline itself is grown by its own thickness (see
  // GetOutlineCornerRadii()).
  void SetRoundedBorderEnabled(
      bool enabled,
      const gfx::RoundedCornersF& contents_corner_radii);

 private:
  // The outline's radii, i.e. the contents radii grown by its own thickness.
  gfx::RoundedCornersF GetOutlineCornerRadii() const;

  void UpdateOutlineBorder();

  // Rounds the infobars' own corners to match the outline.
  void UpdateInfoBarCornerRadii();

  bool rounded_border_enabled_ = false;
  gfx::RoundedCornersF contents_corner_radii_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_INFOBAR_CONTAINER_VIEW_H_
