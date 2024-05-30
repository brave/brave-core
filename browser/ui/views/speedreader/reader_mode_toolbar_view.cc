/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/speedreader/reader_mode_toolbar_view.h"

#include <memory>

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "content/public/browser/browser_context.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "url/gurl.h"

namespace content {
struct ContextMenuParams;
}

namespace {

constexpr gfx::Size kToolbarSize{870, 40};

class Toolbar : public views::WebView {
 public:
  explicit Toolbar(content::BrowserContext* browser_context)
      : views::WebView(browser_context) {
    set_allow_accelerators(true);
    LoadInitialURL(GURL(kSpeedreaderPanelURL));
  }

 private:
  // WebView:
  bool HandleContextMenu(content::RenderFrameHost& render_frame_host,
                         const content::ContextMenuParams& params) override {
    // Ignore context menu.
    return true;
  }
};

}  // namespace

ReaderModeToolbarView::ReaderModeToolbarView(
    content::BrowserContext* browser_context) {
  SetBackground(views::CreateThemedSolidBackground(kColorToolbar));
  toolbar_ = std::make_unique<Toolbar>(browser_context);
  AddChildView(toolbar_.get());
}

ReaderModeToolbarView::~ReaderModeToolbarView() = default;

content::WebContents* ReaderModeToolbarView::GetWebContentsForTesting() {
  return toolbar_->web_contents();
}

gfx::Size ReaderModeToolbarView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return kToolbarSize;
}

void ReaderModeToolbarView::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  views::View::OnBoundsChanged(previous_bounds);
  auto toolbar_bounds = GetLocalBounds();
  toolbar_bounds.ClampToCenteredSize(kToolbarSize);
#if BUILDFLAG(IS_WIN)
  if (toolbar_bounds.width() >= kToolbarSize.width()) {
    toolbar_bounds.Offset(-7, 0);
  }
#endif
  toolbar_->SetBoundsRect(toolbar_bounds);
}

BEGIN_METADATA(ReaderModeToolbarView)
END_METADATA
