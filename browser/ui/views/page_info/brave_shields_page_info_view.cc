/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_info/brave_shields_page_info_view.h"

#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/top_chrome/webui_contents_wrapper.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"

namespace {

class ShieldsWebView : public views::WebView,
                       public WebUIContentsWrapper::Host {
  METADATA_HEADER(ShieldsWebView, views::WebView)

 public:
  explicit ShieldsWebView(WebUIContentsWrapper* contents_wrapper)
      : contents_wrapper_(contents_wrapper) {
    CHECK(contents_wrapper_);
    contents_wrapper_->SetHost(weak_factory_.GetWeakPtr());
    SetWebContents(contents_wrapper_->web_contents());
    holder()->SetCornerRadii(gfx::RoundedCornersF(0, 0, 16, 16));
  }

  ~ShieldsWebView() override = default;

  // views::WebView:
  bool HandleContextMenu(content::RenderFrameHost& render_frame_host,
                         const content::ContextMenuParams& params) override {
    return true;
  }

  void VisibilityChanged(views::View* starting_from, bool is_visible) override {
    views::WebView::VisibilityChanged(starting_from, is_visible);
    CHECK(contents_wrapper_);
    if (auto* web_contents = contents_wrapper_->web_contents()) {
      if (is_visible) {
        web_contents->WasShown();
      } else {
        web_contents->WasHidden();
      }
    }
  }

  // WebUIContentsWrapper::Host:
  void ShowUI() override {}
  void CloseUI() override {}

  void ResizeDueToAutoResize(content::WebContents* source,
                             const gfx::Size& new_size) override {
    SetPreferredSize(new_size);
  }

 private:
  raw_ptr<WebUIContentsWrapper> contents_wrapper_ = nullptr;
  base::WeakPtrFactory<ShieldsWebView> weak_factory_{this};
};

BEGIN_METADATA(ShieldsWebView)
END_METADATA

}  // namespace

BraveShieldsPageInfoView::BraveShieldsPageInfoView(
    BrowserWindowInterface* browser_window_interface,
    content::WebContents* web_contents) {
  SetLayoutManager(std::make_unique<views::FillLayout>());
  InitializeWebView(browser_window_interface, web_contents);
}

BraveShieldsPageInfoView::~BraveShieldsPageInfoView() = default;

void BraveShieldsPageInfoView::ChildPreferredSizeChanged(View* child) {
  PreferredSizeChanged();
}

gfx::Size BraveShieldsPageInfoView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  gfx::Size size = views::View::CalculatePreferredSize(available_size);
  constexpr int kMinWebViewHeight = 290;
  size.set_height(std::max(size.height(), kMinWebViewHeight));
  return size;
}

void BraveShieldsPageInfoView::InitializeWebView(
    BrowserWindowInterface* browser_window_interface,
    content::WebContents* web_contents) {
  CHECK(browser_window_interface);
  CHECK(web_contents);
  CHECK(!contents_wrapper_);
  CHECK(!web_view_);

  // Create a WebContents wrapper for the Shields WebUI.
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  contents_wrapper_ = std::make_unique<WebUIContentsWrapperT<ShieldsPanelUI>>(
      GURL(kShieldsPanelURL), profile, IDS_BRAVE_SHIELDS);

  // Associate the WebContents to the BrowserWindowInterface so the Shields
  // WebUI can access TabStripModel and other browser resources.
  webui::SetBrowserWindowInterface(contents_wrapper_->web_contents(),
                                   browser_window_interface);

  // Create WebView that will display the Shields WebUI.
  web_view_ =
      AddChildView(std::make_unique<ShieldsWebView>(contents_wrapper_.get()));
}

BEGIN_METADATA(BraveShieldsPageInfoView)
END_METADATA
