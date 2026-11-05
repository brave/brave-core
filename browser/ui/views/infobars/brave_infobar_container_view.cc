/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_infobar_container_view.h"

#include <memory>

#include "base/check.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/border.h"
#include "ui/views/view.h"

namespace {

constexpr int kContentSeparatorHeightDip = 1;

// Upstream draws a heavy BubbleBorder drop shadow here; Brave wants a
// flatter look, but removing it entirely leaves no browser-drawn line
// between the infobar and the page, which a malicious page can exploit to
// make itself look like a continuation of the infobar. Paint a thin
// hairline instead so the boundary stays visible (and unspoofable, since
// it's painted by the browser, not the page).
class ContentSeparator : public views::View {
  METADATA_HEADER(ContentSeparator, views::View)

 public:
  ContentSeparator() {
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
  }

  // views::View:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override {
    return gfx::Size(0, kContentSeparatorHeightDip);
  }

  void OnPaint(gfx::Canvas* canvas) override {
    canvas->FillRect(
        GetLocalBounds(),
        GetColorProvider()->GetColor(kColorInfoBarContentAreaSeparator));
  }

  void OnThemeChanged() override {
    views::View::OnThemeChanged();
    // ContentSeparator is a layer, it needs to be manually marked for
    // redraw when the theme changes.
    SchedulePaint();
  }
};

BEGIN_METADATA(ContentSeparator)
END_METADATA

}  // namespace

BraveInfoBarContainerView::BraveInfoBarContainerView(
    infobars::InfoBarContainer::Delegate* delegate)
    : InfoBarContainerView(delegate) {
  // Replace upstream's heavy drop shadow with a thin separator so there's
  // still a browser-drawn (unspoofable) line at the infobar/page boundary.
  DCHECK(content_shadow_);
  RemoveChildViewT(content_shadow_.ExtractAsDangling());
  content_shadow_ = AddChildView(std::make_unique<ContentSeparator>());
}

BraveInfoBarContainerView::~BraveInfoBarContainerView() = default;

void BraveInfoBarContainerView::Layout(PassKey) {
  LayoutSuperclass<InfoBarContainerView>(this);

  if (!rounded_border_enabled_) {
    return;
  }

  // InfoBarContainerView::Layout() sizes each infobar (and content_shadow_)
  // to this container's raw bounds, with no awareness of SetBorder()'s
  // insets. Shrink them by the outline's own thickness so the rounded-rect
  // outline painted in OnPaintBorder() (which runs after children, but is
  // still hidden behind any infobar content occupying the exact same pixels,
  // since each infobar paints to its own opaque layer) has a reserved ring
  // to show in.
  for (views::View* child : children()) {
    if (child == content_shadow_) {
      continue;
    }
    gfx::Rect bounds = child->bounds();
    bounds.Inset(kRoundedCornersContentsOutlineThickness);
    child->SetBoundsRect(bounds);
  }
}

void BraveInfoBarContainerView::OnThemeChanged() {
  InfoBarContainerView::OnThemeChanged();

  if (rounded_border_enabled_) {
    // The border's color is baked in at creation; rebuild it so it picks up
    // the new theme's colors.
    SetRoundedBorderEnabled(true, rounded_border_corner_radii_);
  }
}

void BraveInfoBarContainerView::SetRoundedBorderEnabled(
    bool enabled,
    const gfx::RoundedCornersF& corner_radii) {
  rounded_border_enabled_ = enabled;
  rounded_border_corner_radii_ = corner_radii;

  // The flat hairline divider is only used in the classic (non-rounded)
  // style; the outline border below takes over marking the boundary when
  // rounded corners are in use.
  DCHECK(content_shadow_);
  content_shadow_->SetVisible(!enabled);

  if (!enabled) {
    SetBorder(nullptr);
    return;
  }

  auto* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }
  SetBorder(BraveContentsViewUtil::CreateContentsOutlineBorder(color_provider,
                                                               corner_radii));
}

BEGIN_METADATA(BraveInfoBarContainerView)
END_METADATA
