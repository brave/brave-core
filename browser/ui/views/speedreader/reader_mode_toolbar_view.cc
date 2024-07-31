/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/speedreader/reader_mode_toolbar_view.h"

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "content/public/browser/browser_context.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/accessibility/view_accessibility.h"
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
  Toolbar(ReaderModeToolbarView* owner,
          content::BrowserContext* browser_context)
      : views::WebView(browser_context), owner_(owner) {
    set_allow_accelerators(true);
  }

 private:
  // WebView:
  bool HandleContextMenu(content::RenderFrameHost& render_frame_host,
                         const content::ContextMenuParams& params) override {
    // Ignore context menu.
    return true;
  }

  void DidGetUserInteraction(const blink::WebInputEvent& event) override {
    if (event.GetType() == blink::WebInputEvent::Type::kMouseDown) {
      owner_->NotifyActive();
    }
  }

  raw_ptr<ReaderModeToolbarView> owner_ = nullptr;
};

}  // namespace

ReaderModeToolbarView::ReaderModeToolbarView(Browser* browser) {
  SetVisible(false);
  SetBackground(views::CreateThemedSolidBackground(kColorToolbar));

  if (!BraveBrowser::ShouldUseBraveWebViewRoundedCorners(browser)) {
    SetBorder(views::CreateThemedSolidSidedBorder(
        gfx::Insets::TLBR(0, 0, 1, 0), kColorToolbarContentAreaSeparator));
  }

  toolbar_ = std::make_unique<Toolbar>(this, browser->profile());
  AddChildView(toolbar_.get());
}

ReaderModeToolbarView::~ReaderModeToolbarView() = default;

void ReaderModeToolbarView::SetVisible(bool visible) {
  if (visible && !toolbar_loaded_) {
    toolbar_->LoadInitialURL(GURL(kSpeedreaderPanelURL));
    toolbar_loaded_ = true;
  }
  views::View::SetVisible(visible);
}

void ReaderModeToolbarView::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ReaderModeToolbarView::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

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

bool ReaderModeToolbarView::OnMousePressed(const ui::MouseEvent& event) {
  if (event.IsOnlyLeftMouseButton()) {
    NotifyActive();
  }
  return View::OnMousePressed(event);
}

void ReaderModeToolbarView::NotifyActive() {
  for (auto& observer : observers_) {
    observer.OnReaderModeToolbarActive(this);
  }
}

BEGIN_METADATA(ReaderModeToolbarView)
END_METADATA
