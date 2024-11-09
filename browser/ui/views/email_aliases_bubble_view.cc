/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/email_aliases_bubble_view.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/core/browser/autofill_driver.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

// static

std::unique_ptr<views::Widget> widget_ptr_;

void EmailAliasesBubbleView::Show(Browser* browser,
                                  uint64_t field_renderer_id) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  views::View* anchor_view = browser_view->GetLocationBarView();
  std::unique_ptr<views::Widget> widget(
      views::BubbleDialogDelegateView::CreateBubble(
          std::make_unique<EmailAliasesBubbleView>(anchor_view, browser,
                                                   field_renderer_id),
          views::Widget::InitParams::CLIENT_OWNS_WIDGET));
  widget->Show();
  widget_ptr_ = std::move(widget);
}

void EmailAliasesBubbleView::Close() {
  if (widget_ptr_) {
    widget_ptr_.reset(nullptr);
  }
}

EmailAliasesBubbleView::EmailAliasesBubbleView(views::View* anchor_view,
                                               Browser* browser,
                                               uint64_t field_renderer_id)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_CENTER),
      browser_(browser),
      field_renderer_id_(field_renderer_id) {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  auto* web_view = new views::WebView(browser->profile());
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
  web_view->SetPreferredSize(gfx::Size(500, 350));
  AddChildView(web_view);

  // Load URL after adding to view hierarchy
  web_view->LoadInitialURL(GURL(kEmailAliasesBubbleURL));
}

EmailAliasesBubbleView::~EmailAliasesBubbleView() {}

void EmailAliasesBubbleView::OnWidgetVisibilityChanged(views::Widget* widget,
                                                       bool visible) {
  std::cout << "OnWidgetVisibilityChanged" << std::endl;
  BubbleDialogDelegateView::OnWidgetVisibilityChanged(widget, visible);
  Close();
}

void EmailAliasesBubbleView::FillField(const std::string& alias_address) {
  if (!browser_) {
    return;
  }
  content::WebContents* web_contents =
      browser_->tab_strip_model()->GetActiveWebContents();
  if (!web_contents) {
    return;
  }
  content::RenderFrameHost* render_frame_host =
      web_contents->GetPrimaryMainFrame();
  if (!render_frame_host) {
    return;
  }
  autofill::AutofillDriver* driver =
      autofill::ContentAutofillDriver::GetForRenderFrameHost(render_frame_host);
  if (!driver) {
    return;
  }
  autofill::LocalFrameToken frame_token = driver->GetFrameToken();
  auto field_global_id = autofill::FieldGlobalId(
      frame_token, autofill::FieldRendererId(field_renderer_id_));
  driver->ApplyFieldAction(autofill::mojom::FieldActionType::kReplaceAll,
                           autofill::mojom::ActionPersistence::kFill,
                           field_global_id, base::UTF8ToUTF16(alias_address));
}

// static
void EmailAliasesBubbleView::FillFieldWithNewAlias(
    const std::string& field_value) {
  if (!widget_ptr_) {
    return;
  }
  auto* email_aliases_bubble_view = static_cast<EmailAliasesBubbleView*>(
      widget_ptr_.get()->widget_delegate());
  if (!email_aliases_bubble_view) {
    return;
  }
  email_aliases_bubble_view->FillField(field_value);
}

BEGIN_METADATA(EmailAliasesBubbleView)
END_METADATA
