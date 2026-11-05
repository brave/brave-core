/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_infobar_container_view.h"

#include <memory>

#include "base/check.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/size.h"
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

BEGIN_METADATA(BraveInfoBarContainerView)
END_METADATA
