/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_origin/brave_origin_buy_window.h"

#include <utility>

#include "base/strings/strcat.h"
#include "brave/brave_domains/service_domains.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "url/url_constants.h"

namespace {

constexpr int kBuyWindowWidth = 800;
constexpr int kBuyWindowHeight = 700;

BraveOriginBuyWindow* g_buy_window = nullptr;

}  // namespace

// static
void BraveOriginBuyWindow::Show(content::BrowserContext* browser_context,
                                base::OnceClosure on_close) {
  if (g_buy_window) {
    g_buy_window->GetWidget()->Activate();
    return;
  }
  g_buy_window = new BraveOriginBuyWindow(browser_context, std::move(on_close));
}

// static
void BraveOriginBuyWindow::Hide() {
  if (g_buy_window) {
    g_buy_window->GetWidget()->Close();
  }
}

// static
bool BraveOriginBuyWindow::IsShowing() {
  return g_buy_window != nullptr;
}

BraveOriginBuyWindow::BraveOriginBuyWindow(
    content::BrowserContext* browser_context,
    base::OnceClosure on_close)
    : on_close_(std::move(on_close)) {
  SetHasWindowSizeControls(true);
  SetTitle(u"Brave Origin");

  buy_url_ = GURL(
      base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                    brave_domains::GetServicesDomain("account"),
                    "/?intent=checkout&product=origin"}));

  SetLayoutManager(std::make_unique<views::FillLayout>());

  contents_ = content::WebContents::Create(
      content::WebContents::CreateParams(browser_context));
  contents_->SetDelegate(this);

  web_view_ = AddChildView(std::make_unique<views::WebView>(nullptr));
  web_view_->SetWebContents(contents_.get());

  contents_->GetController().LoadURL(buy_url_, content::Referrer(),
                                     ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                                     std::string());

  // Create and show the widget.
  views::Widget::InitParams params(
      views::Widget::InitParams::CLIENT_OWNS_WIDGET,
      views::Widget::InitParams::TYPE_WINDOW);
  params.delegate = this;
  params.bounds = gfx::Rect(kBuyWindowWidth, kBuyWindowHeight);
  widget_ = std::make_unique<views::Widget>();
  widget_->Init(std::move(params));

  GetWidget()->CenterWindow(gfx::Size(kBuyWindowWidth, kBuyWindowHeight));
  GetWidget()->Show();
}

BraveOriginBuyWindow::~BraveOriginBuyWindow() {
  if (contents_) {
    contents_->SetDelegate(nullptr);
  }
  if (g_buy_window == this) {
    g_buy_window = nullptr;
  }
}

void BraveOriginBuyWindow::WindowClosing() {
  if (on_close_) {
    std::move(on_close_).Run();
  }
}

std::u16string BraveOriginBuyWindow::GetAccessibleWindowTitle() const {
  return u"Brave Origin";
}

gfx::Size BraveOriginBuyWindow::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(kBuyWindowWidth, kBuyWindowHeight);
}

content::WebContents* BraveOriginBuyWindow::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params,
    base::OnceCallback<void(content::NavigationHandle&)>
        navigation_handle_callback) {
  // Block all attempts to open new tabs/windows.
  // Only allow navigation within the same WebContents for the checkout flow.
  const GURL& url = params.url;
  if (url.DomainIs("brave.com") || url.DomainIs("brave.software")) {
    // Allow navigation within brave domains by loading in the same contents.
    contents_->GetController().LoadURL(url, content::Referrer(),
                                       ui::PAGE_TRANSITION_LINK, std::string());
  }
  // Return nullptr to block opening a new tab.
  return nullptr;
}

content::WebContents* BraveOriginBuyWindow::AddNewContents(
    content::WebContents* source,
    std::unique_ptr<content::WebContents> new_contents,
    const GURL& target_url,
    WindowOpenDisposition disposition,
    const blink::mojom::WindowFeatures& window_features,
    bool user_gesture,
    bool* was_blocked) {
  // Block all popup windows.
  if (was_blocked) {
    *was_blocked = true;
  }
  return nullptr;
}

bool BraveOriginBuyWindow::HandleKeyboardEvent(
    content::WebContents* source,
    const input::NativeWebKeyboardEvent& event) {
  return unhandled_keyboard_event_handler_.HandleKeyboardEvent(
      event, GetFocusManager());
}

BEGIN_METADATA(BraveOriginBuyWindow)
END_METADATA
