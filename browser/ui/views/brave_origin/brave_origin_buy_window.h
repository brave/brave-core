/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ORIGIN_BRAVE_ORIGIN_BUY_WINDOW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ORIGIN_BRAVE_ORIGIN_BUY_WINDOW_H_

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_delegate.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/webview/unhandled_keyboard_event_handler.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace views {
class WebView;
}  // namespace views

// Standalone window that loads the Brave Origin checkout page.
// Navigation is blocked to prevent the user from navigating away from
// the checkout flow.
class BraveOriginBuyWindow : public views::WidgetDelegateView,
                             public content::WebContentsDelegate {
  METADATA_HEADER(BraveOriginBuyWindow, views::WidgetDelegateView)

 public:
  // Shows the buy window. |on_close| is called when the window is closed.
  static void Show(content::BrowserContext* browser_context,
                   base::OnceClosure on_close);
  static void Hide();
  static bool IsShowing();

  BraveOriginBuyWindow(const BraveOriginBuyWindow&) = delete;
  BraveOriginBuyWindow& operator=(const BraveOriginBuyWindow&) = delete;

 private:
  BraveOriginBuyWindow(content::BrowserContext* browser_context,
                       base::OnceClosure on_close);
  ~BraveOriginBuyWindow() override;

  // views::WidgetDelegateView:
  void WindowClosing() override;
  std::u16string GetAccessibleWindowTitle() const override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;

  // content::WebContentsDelegate - navigation blocking:
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params,
      base::OnceCallback<void(content::NavigationHandle&)>
          navigation_handle_callback) override;
  content::WebContents* AddNewContents(
      content::WebContents* source,
      std::unique_ptr<content::WebContents> new_contents,
      const GURL& target_url,
      WindowOpenDisposition disposition,
      const blink::mojom::WindowFeatures& window_features,
      bool user_gesture,
      bool* was_blocked) override;
  bool HandleKeyboardEvent(content::WebContents* source,
                           const input::NativeWebKeyboardEvent& event) override;

  std::unique_ptr<views::Widget> widget_;
  std::unique_ptr<content::WebContents> contents_;
  raw_ptr<views::WebView> web_view_ = nullptr;
  GURL buy_url_;
  base::OnceClosure on_close_;

  views::UnhandledKeyboardEventHandler unhandled_keyboard_event_handler_;

  base::WeakPtrFactory<BraveOriginBuyWindow> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ORIGIN_BRAVE_ORIGIN_BUY_WINDOW_H_
