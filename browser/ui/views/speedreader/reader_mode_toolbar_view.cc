/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/speedreader/reader_mode_toolbar_view.h"

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
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
constexpr gfx::RoundedCornersF kRoundedCorners(
    BraveContentsViewUtil::kBorderRadius,
    BraveContentsViewUtil::kBorderRadius,
    0,
    0);

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
      owner_->ActivateContents();
    }
  }

  raw_ptr<ReaderModeToolbarView> owner_ = nullptr;
};

}  // namespace

void ReaderModeToolbarView::SetDelegate(Delegate* delegate) {
  delegate_ = delegate;
}

ReaderModeToolbarView::ReaderModeToolbarView(
    content::BrowserContext* browser_context,
    bool use_rounded_corners)
    : use_rounded_corners_(use_rounded_corners) {
  SetVisible(false);

  toolbar_ = std::make_unique<Toolbar>(this, browser_context);
  AddChildView(toolbar_.get());

  if (use_rounded_corners_) {
    SetBackground(
        views::CreateRoundedRectBackground(kColorToolbar, kRoundedCorners));
  } else {
    SetBorder(views::CreateSolidSidedBorder(gfx::Insets::TLBR(0, 0, 1, 0),
                                            kColorToolbarContentAreaSeparator));
    SetBackground(views::CreateSolidBackground(kColorToolbar));
  }
}

ReaderModeToolbarView::~ReaderModeToolbarView() = default;

void ReaderModeToolbarView::SetVisible(bool visible) {
  if (visible && !toolbar_contents_) {
    content::WebContents::CreateParams create_params(
        toolbar_->GetBrowserContext(), FROM_HERE);
    toolbar_contents_ = content::WebContents::Create(create_params);

    const GURL toolbar_url(kSpeedreaderPanelURL);
    content::NavigationController::LoadURLParams params(toolbar_url);
    toolbar_contents_->GetController().LoadURLWithParams(params);

    toolbar_->SetWebContents(toolbar_contents_.get());
    if (use_rounded_corners_) {
      toolbar_->holder()->SetCornerRadii(kRoundedCorners);
    }
  }
  views::View::SetVisible(visible);
}

content::WebContents* ReaderModeToolbarView::GetWebContentsForTesting() {
  return toolbar_->web_contents();
}

void ReaderModeToolbarView::SwapToolbarContents(
    ReaderModeToolbarView* another_toolbar) {
  if (!another_toolbar) {
    return;
  }

  CHECK_NE(this, another_toolbar);

  auto* contents = toolbar_->web_contents();
  auto* another_contents = another_toolbar->toolbar_->web_contents();
  if (!contents || !another_contents) {
    return;
  }

  toolbar_->SetWebContents(nullptr);
  another_toolbar->toolbar_->SetWebContents(nullptr);

  another_toolbar->toolbar_->SetWebContents(contents);
  toolbar_->SetWebContents(another_contents);
}

void ReaderModeToolbarView::RestoreToolbarContents(
    ReaderModeToolbarView* another_toolbar) {
  if (!another_toolbar) {
    return;
  }

  CHECK_NE(this, another_toolbar);
  toolbar_->SetWebContents(nullptr);
  another_toolbar->toolbar_->SetWebContents(nullptr);

  toolbar_->SetWebContents(toolbar_contents_.get());
  another_toolbar->toolbar_->SetWebContents(
      another_toolbar->toolbar_contents_.get());
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
  if (event.IsOnlyLeftMouseButton() && delegate_) {
    delegate_->OnReaderModeToolbarActivate(this);
  }
  return views::View::OnMousePressed(event);
}

void ReaderModeToolbarView::ActivateContents() {
  if (delegate_) {
    delegate_->OnReaderModeToolbarActivate(this);
  }
}

BEGIN_METADATA(ReaderModeToolbarView)
END_METADATA
