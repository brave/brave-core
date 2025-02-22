/* Copyright (c) 2025 The Brave Authors. All rights reserved.
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
#include "chrome/browser/ui/views/frame/contents_layout_manager.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

// static

std::unique_ptr<views::Widget> widget_ptr_;

void EmailAliasesBubbleView::Show(content::WebContents* web_contents,
                                  views::View* anchor_view,
                                  uint64_t field_renderer_id) {
  auto bubble_view = std::make_unique<EmailAliasesBubbleView>(anchor_view, web_contents, field_renderer_id);
  std::unique_ptr<views::Widget> widget(
      views::BubbleDialogDelegateView::CreateBubble(
          std::move(bubble_view),
          views::Widget::InitParams::CLIENT_OWNS_WIDGET));
  widget->Show();
  widget_ptr_ = std::move(widget);
}

void EmailAliasesBubbleView::Close() {
  if (widget_ptr_) {
    widget_ptr_.reset(nullptr);
  }
}

EmailAliasesBubbleView::EmailAliasesBubbleView(
    views::View* anchor_view,
    content::WebContents* web_contents,
    uint64_t field_renderer_id)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_CENTER),
      web_contents_(web_contents),
      field_renderer_id_(field_renderer_id) {
  SetLayoutManager(std::make_unique<views::FillLayout>());

  auto web_view =
      std::make_unique<views::WebView>(web_contents->GetBrowserContext());
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
  web_view_ = AddChildView(std::move(web_view));

  // Load URL after adding to view hierarchy
  web_view_->LoadInitialURL(GURL(kEmailAliasesBubbleURL));
}

EmailAliasesBubbleView::~EmailAliasesBubbleView() {}

void EmailAliasesBubbleView::FillField(const std::string& alias_address) {
  if (!web_contents_) {
    return;
  }
  content::RenderFrameHost* render_frame_host =
      web_contents_->GetPrimaryMainFrame();
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

gfx::Size EmailAliasesBubbleView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  return gfx::Size(500, 350);
}

void EmailAliasesBubbleView::OnWidgetDestroying(views::Widget* widget) {
  widget_is_being_destroyed_ = true;
}

void EmailAliasesBubbleView::OnAnchorBoundsChanged() {
  if (!widget_is_being_destroyed_) {
    BubbleDialogDelegate::OnAnchorBoundsChanged();
  }
}

BEGIN_METADATA(EmailAliasesBubbleView)
END_METADATA

