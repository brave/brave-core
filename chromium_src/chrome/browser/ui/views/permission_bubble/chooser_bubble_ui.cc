/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
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

}  // namespace

namespace views {
class BraveBubbleDialogDelegateView : public views::BubbleDialogDelegateView {
 public:
  BraveBubbleDialogDelegateView() : BubbleDialogDelegateView() {}

  static views::Widget* CreateBubble(
      std::unique_ptr<BubbleDialogDelegateView> delegate) {
    return BubbleDialogDelegateView::CreateBubble(std::move(delegate));
  }
  void WindowClosing() override {
    views::BubbleDialogDelegateView::WindowClosing();
    Browser* browser =
        chrome::FindBrowserWithWindow(anchor_widget()->GetNativeWindow());
    content::WebContents* active =
        browser->tab_strip_model()->GetActiveWebContents();
    auto* tab_helper =
        brave_wallet::BraveWalletTabHelper::FromWebContents(active);
    if (tab_helper)
      tab_helper->ClosePanelOnDeactivate(true);
  }
};

}  // namespace views

namespace chrome {

Browser* FindBrowserAndAdjustBubbleForBraveWalletPanel(
    content::WebContents* contents) {
  if (!IsBravePanel(contents))
    return chrome::FindBrowserWithWebContents(contents);

  Browser* browser = chrome::FindBrowserWithProfile(
      Profile::FromBrowserContext(contents->GetBrowserContext()));
  content::WebContents* active =
      browser->tab_strip_model()->GetActiveWebContents();
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(active);
  if (tab_helper)
    tab_helper->ClosePanelOnDeactivate(false);
  return browser;
}

}  // namespace chrome

#define FindBrowserWithWebContents FindBrowserAndAdjustBubbleForBraveWalletPanel
#define GetActiveWebContents                           \
  GetActiveWebContents() && !IsBravePanel(contents) && \
      browser->tab_strip_model()->GetActiveWebContents

#define BubbleDialogDelegateView BraveBubbleDialogDelegateView
#include "../../../../../../../chrome/browser/ui/views/permission_bubble/chooser_bubble_ui.cc"
#undef BubbleDialogDelegateView
#undef GetActiveWebContents
#undef FindBrowserWithWebContents
