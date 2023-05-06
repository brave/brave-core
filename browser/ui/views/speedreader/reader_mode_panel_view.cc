/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/speedreader/reader_mode_panel_view.h"

#include <memory>

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/views/background.h"
#include "url/gurl.h"

namespace content {
struct ContextMenuParams;
}

namespace {

class Toolbar : public views::WebView {
 public:
  explicit Toolbar(content::BrowserContext* browser_context)
      : views::WebView(browser_context) {
    LoadInitialURL(GURL(kSpeedreaderPanelURL));
    EnableSizingFromWebContents(gfx::Size(10, 10), gfx::Size(10000, 500));
  }

  // WebView:
  bool HandleContextMenu(content::RenderFrameHost& render_frame_host,
                         const content::ContextMenuParams& params) override {
    // Ignore context menu.
    return true;
  }
};

}  // namespace

ReaderModePanelView::ReaderModePanelView(
    content::BrowserContext* browser_context) {
  SetBackground(views::CreateThemedSolidBackground(kColorToolbar));

  toolbar_ = std::make_unique<Toolbar>(browser_context);
  AddChildView(toolbar_.get());
}

ReaderModePanelView::~ReaderModePanelView() = default;

gfx::Size ReaderModePanelView::CalculatePreferredSize() const {
  return toolbar_->GetPreferredSize();
}

void ReaderModePanelView::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  views::View::OnBoundsChanged(previous_bounds);

  const auto toolbar_size = toolbar_->GetPreferredSize();

  auto toolbar_bounds = bounds();
  toolbar_bounds.ClampToCenteredSize(toolbar_size);
  toolbar_bounds.Offset(-10, 0);
  toolbar_->SetBoundsRect(toolbar_bounds);
}
