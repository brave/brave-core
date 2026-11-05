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
  void OnThemeChanged() override;

  // When `enabled`, replaces the flat bottom divider with a floating-card
  // look: margin from the window edges (applied by the browser view's
  // layout; see BraveBrowserViewTabbedLayoutImpl::AdjustInfobarLayout()) plus
  // a rounded-rect outline border using `corner_radii`.
  void SetRoundedBorderEnabled(bool enabled,
                               const gfx::RoundedCornersF& corner_radii);

 private:
  bool rounded_border_enabled_ = false;
  gfx::RoundedCornersF rounded_border_corner_radii_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_INFOBAR_CONTAINER_VIEW_H_
