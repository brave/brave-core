/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/extensions/extension_context_menu_model.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/device_chooser_content_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace {

bool IsBravePanel(content::WebContents* content) {
  return content->GetVisibleURL().EqualsIgnoringRef(
      GURL(kBraveUIWalletPanelURL));
}

void OnWindowClosing(views::Widget* anchor_widget) {
  if (!anchor_widget) {
    return;
  }
  Browser* browser =
      chrome::FindBrowserWithWindow(anchor_widget->GetNativeWindow());
  if (!browser || !browser->tab_strip_model()) {
    return;
  }
  content::WebContents* active =
      browser->tab_strip_model()->GetActiveWebContents();
  if (!active) {
    return;
  }
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(active);
  if (tab_helper) {
    tab_helper->SetCloseOnDeactivate(true);
  }
}

}  // namespace

namespace views {
class BraveBubbleDialogDelegateView : public views::BubbleDialogDelegateView {
 public:
  BraveBubbleDialogDelegateView() : BubbleDialogDelegateView() {}

  static views::Widget* CreateBubble(
      std::unique_ptr<BubbleDialogDelegateView> delegate) {
    if (delegate) {
      delegate->RegisterWindowClosingCallback(
          base::BindOnce(&OnWindowClosing, delegate->anchor_widget()));
    }
    return BubbleDialogDelegateView::CreateBubble(std::move(delegate));
  }
};

}  // namespace views

namespace chrome {

Browser* FindBrowserAndAdjustBubbleForBraveWalletPanel(
    content::WebContents* contents) {
  if (!IsBravePanel(contents))
    return chrome::FindBrowserWithTab(contents);

  Browser* browser = chrome::FindBrowserWithProfile(
      Profile::FromBrowserContext(contents->GetBrowserContext()));
  content::WebContents* active =
      browser->tab_strip_model()->GetActiveWebContents();
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(active);
  if (tab_helper)
    tab_helper->SetCloseOnDeactivate(false);
  return browser;
}

}  // namespace chrome

#define FindBrowserWithTab FindBrowserAndAdjustBubbleForBraveWalletPanel
#define GetActiveWebContents                           \
  GetActiveWebContents() && !IsBravePanel(contents) && \
      browser->tab_strip_model()->GetActiveWebContents

#define BubbleDialogDelegateView BraveBubbleDialogDelegateView

#define SetExtraView(...)    \
  SetExtraView(__VA_ARGS__); \
  SetFootnoteView(device_chooser_content_view_->CreateFootnoteView(browser))

#include "src/chrome/browser/ui/views/permissions/chooser_bubble_ui.cc"

#undef SetExtraView
#undef BubbleDialogDelegateView
#undef GetActiveWebContents
#undef FindBrowserWithTab
