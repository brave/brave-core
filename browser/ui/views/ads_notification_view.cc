/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stddef.h>
#include <string>
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/browser/ui/views/ads_notification_view.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/views/controls/webview/web_contents_set_background_color.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "url/gurl.h"
#include "ui/views/views_delegate.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/label.h"

namespace {
  AdsNotificationView* g_active_ads_window = nullptr;
  views::WebView* wv = nullptr;
constexpr gfx::Size kContainerSize(328, 200);
// constexpr gfx::Size kBigContainerSize(328, 500);
constexpr gfx::Size kSmallContainerSize(328, 50);
constexpr SkColor kBackground = SkColorSetRGB(0xf5, 0xf5, 0xf5);
  // const base::StringPiece16 kGoogleLlc(STRING16_LITERAL("Google LLC"));
}  // namespace

// static
views::Widget* AdsNotificationView::Show(Profile* profile,
                               const GURL& url,
                               const gfx::Rect& rect) {
  if (g_active_ads_window)
    g_active_ads_window->Close();

  views::Widget* window = new views::Widget;
  views::Widget::InitParams window_params;
  window_params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  // params.parent = BrowserView::GetBrowserViewForBrowser(browser)->GetWidget()->GetNativeView();
  window_params.bounds = { 0, 0, 1500, 1000 };

  // g_active_ads_window = new AdsNotificationView(profile);

  // create container
  views::View* container = new views::View();
   views::View* wv_container = new views::View();
  /*
  views::FlexLayout* flex_layout = new views::FlexLayout();
  flex_layout->SetOrientation(views::LayoutOrientation::kVertical);
  container->SetLayoutManager(flex_layout);
  */
//  views::FlexLayout* container_layout = container->SetLayoutManager(std::make_unique<views::FlexLayout>());
//  container_layout->SetOrientation(views::LayoutOrientation::kVertical).SetIgnoreDefaultMainAxisMargins(true);
  // container->SetLayoutManager(std::make_unique<views::FillLayout>());
  container->SetLayoutManager(std::make_unique<views::BoxLayout>(views::BoxLayout::Orientation::kVertical, gfx::Insets(), 0));
  container->SetSize(kSmallContainerSize);

  // AddChildView(wv);
  // add header to container
//  views::Textfield* tf = new views::Textfield();

  views::Label* tv = new views::Label(base::ASCIIToUTF16("toplevel"));
  tv->SetBackgroundColor(kBackground);
  container->AddChildView(tv);

   wv_container->SetLayoutManager(std::make_unique<views::FillLayout>());
  wv = views::ViewsDelegate::GetInstance()->GetWebViewForWindow();
  wv_container->SetSize(kContainerSize);
  wv_container->SetPreferredSize(kContainerSize);
  wv_container->SizeToPreferredSize();
  wv_container->AddChildView(wv);
  // container->AddChildView(wv_container);
  // container->AddChildView(wv);
 // views::TextField* tf2 = new views::TextField();


  // params.delegate = g_active_ads_window;

  views::Label* tv2 = new views::Label(base::ASCIIToUTF16("bottomlevel"));
  tv2->SetBackgroundColor(kBackground);
  window_params.type = views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
  window_params.opacity = views::Widget::InitParams::WindowOpacity::kOpaque;
  window_params.shadow_type = views::Widget::InitParams::ShadowType::kDrop;
  window->Init(std::move(window_params));
  window->CenterWindow(window_params.bounds.size());
  //window->ShowInactive();
  window->Show();
  window->SetContentsView(container);

//  std::unique_ptr<views::Widget> child(new views::Widget());
  views::Widget* child = new views::Widget;
  views::Widget::InitParams child_params(views::Widget::InitParams::TYPE_POPUP);
  child_params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  child_params.opacity = views::Widget::InitParams::WindowOpacity::kOpaque;
  // child_params.parent = window->GetNativeView();
  child_params.bounds = { 1000, 500, 200, 200 };
  child_params.parent = window->GetNativeWindow();
  child->Init(std::move(child_params));
  // child->SetBoundsConstrained(child_params.bounds);
  // child->CenterWindow(child_params.bounds.size());
  // child->SetBounds(gfx::Rect(0, 250, 200, 200));
  child->Show();
  child->SetContentsView(wv_container);
  return window;
}

AdsNotificationView::~AdsNotificationView() {
  LOG(ERROR) << __FUNCTION__;
}

// AdsNotificationView::AdsNotificationView(Browser* browser, const GURL& url) {
AdsNotificationView::AdsNotificationView(Profile* profile) {
  // Life cycle control
  //  * When this is hidden?
  //  * This can
  //  * Ads windows should be
  // Which profiles should be used for this webview?
  // Profile can be removed during the runtime.
  // This window should be destroyed when |browser->profile()| is closed/removed?
  // Also for browser?
  // auto* profile = browser->profile();
  // registrar_.Add(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
  //                  content::Source<Profile>(profile_));
  // How to know ads notification window is visible or not?
  // This could be happen when multiple profiles are used.

  auto* web_view = new views::WebView(profile);
  // auto* web_view = new views::WebView(browser->profile());
  views::WebContentsSetBackgroundColor::CreateForWebContentsWithColor(
      web_view->GetWebContents(),
      SK_ColorTRANSPARENT);
  // web_view->LoadInitialURL(GURL("http://techslides.com/demos/sample-videos/small.mp4"));
  web_view->LoadInitialURL(GURL("https://m.media-amazon.com/images/I/418oH6YjpFL.jpg"));
  /*
  if (web_view->EmbedsFullscreenWidget()) {
    LOG(INFO) << "* WARNING * embeds fullscreen!";
  } else {
    LOG(INFO) << "* INFO * does not embed fullscreen!";
  }
  */
  AddChildView(web_view);
}

void AdsNotificationView::Close() {
  GetWidget()->Close();
}
