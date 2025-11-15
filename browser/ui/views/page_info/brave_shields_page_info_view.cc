/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_info/brave_shields_page_info_view.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/functional/callback.h"
#include "brave/browser/ui/views/page_info/brave_page_info_view_ids.h"
#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/webui/top_chrome/webui_contents_wrapper.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "extensions/common/constants.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/controls/webview/unhandled_keyboard_event_handler.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "url/gurl.h"

namespace {

class ShieldsWebView : public views::WebView,
                       public WebUIContentsWrapper::Host {
  METADATA_HEADER(ShieldsWebView, views::WebView)

 public:
  ShieldsWebView(base::WeakPtr<WebUIContentsWrapper> contents_wrapper,
                 base::RepeatingCallback<void()> close_bubble)
      : contents_wrapper_(std::move(contents_wrapper)),
        close_bubble_(std::move(close_bubble)) {
    contents_wrapper_->SetHost(weak_factory_.GetWeakPtr());
    SetWebContents(contents_wrapper_->web_contents());
    holder()->SetCornerRadii(gfx::RoundedCornersF(0, 0, 16, 16));
  }

  ~ShieldsWebView() override = default;

  // WebUIContentsWrapper::Host:
  void ShowUI() override {}
  void CloseUI() override { close_bubble_.Run(); }

  // views::WebView:
  bool HandleContextMenu(content::RenderFrameHost& render_frame_host,
                         const content::ContextMenuParams& params) override {
    return true;
  }

  void VisibilityChanged(views::View* starting_from, bool is_visible) override {
    views::WebView::VisibilityChanged(starting_from, is_visible);
    if (contents_wrapper_) {
      if (auto* web_contents = contents_wrapper_->web_contents()) {
        if (is_visible) {
          web_contents->WasShown();
        } else {
          web_contents->WasHidden();
        }
      }
    }
  }

  bool HandleKeyboardEvent(
      content::WebContents* source,
      const input::NativeWebKeyboardEvent& event) override {
    return unhandled_keyboard_event_handler_.HandleKeyboardEvent(
        event, GetFocusManager());
  }

  void ResizeDueToAutoResize(content::WebContents* source,
                             const gfx::Size& new_size) override {
    SetPreferredSize(new_size);
  }

 private:
  base::WeakPtr<WebUIContentsWrapper> contents_wrapper_ = nullptr;
  base::RepeatingCallback<void()> close_bubble_;
  views::UnhandledKeyboardEventHandler unhandled_keyboard_event_handler_;
  base::WeakPtrFactory<ShieldsWebView> weak_factory_{this};
};

BEGIN_METADATA(ShieldsWebView)
END_METADATA

void LoadShieldsURL(content::NavigationController& nav_controller,
                    std::string_view query) {
  nav_controller.LoadURL(GURL(kShieldsPanelURL + std::string(query)),
                         content::Referrer(), ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                         /*extra_headers=*/std::string());
}

}  // namespace

BraveShieldsPageInfoView::BraveShieldsPageInfoView(
    BrowserWindowInterface* browser_window_interface,
    base::RepeatingCallback<void()> close_bubble)
    : browser_(browser_window_interface) {
  CHECK(browser_window_interface);

  contents_wrapper_ = CreateContentsWrapper();
  SetLayoutManager(std::make_unique<views::FillLayout>());
  web_view_ = AddChildView(std::make_unique<ShieldsWebView>(
      contents_wrapper_->GetWeakPtr(), std::move(close_bubble)));
  web_view_->SetID(static_cast<int>(BravePageInfoViewID::kShieldsWebView));
}

BraveShieldsPageInfoView::~BraveShieldsPageInfoView() = default;

bool BraveShieldsPageInfoView::ShouldShowForWebContents(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return false;
  }

  const GURL& url = web_contents->GetLastCommittedURL();

  if (url.SchemeIs(url::kAboutScheme) || url.SchemeIs(url::kBlobScheme) ||
      url.SchemeIs(url::kDataScheme) || url.SchemeIs(url::kFileSystemScheme) ||
      url.SchemeIs(kMagnetScheme) || url.SchemeIs(kBraveUIScheme) ||
      url.SchemeIs(content::kChromeUIScheme) ||
      url.SchemeIs(extensions::kExtensionScheme)) {
    return false;
  }

  return true;
}

void BraveShieldsPageInfoView::ShowRepeatedReloadsView() {
  LoadShieldsURL(contents_wrapper_->web_contents()->GetController(),
                 "?mode=afterRepeatedReloads");
}

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

std::unique_ptr<WebUIContentsWrapper>
BraveShieldsPageInfoView::CreateContentsWrapper() {
  // Create a new contents wrapper for the Shields WebUI.
  auto wrapper = std::make_unique<WebUIContentsWrapperT<ShieldsPanelUI>>(
      GURL(kShieldsPanelURL), browser_->GetProfile(), IDS_BRAVE_SHIELDS);

  // Associate the WebContents to the BrowserWindowInterface so the Shields
  // WebUI can access TabStripModel and other browser resources.
  webui::SetBrowserWindowInterface(wrapper->web_contents(), browser_);

  return wrapper;
}

BEGIN_METADATA(BraveShieldsPageInfoView)
END_METADATA
