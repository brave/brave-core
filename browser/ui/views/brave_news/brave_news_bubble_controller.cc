/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_news/brave_news_bubble_controller.h"

#include <vector>

#include "base/memory/ptr_util.h"
#include "brave/browser/ui/views/brave_news/brave_news_action_icon_view.h"
#include "brave/browser/ui/views/brave_news/brave_news_bubble_view.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

namespace brave_news {
// static
BraveNewsBubbleController*
BraveNewsBubbleController::CreateOrGetFromWebContents(
    content::WebContents* web_contents) {
  CHECK(web_contents);
  BraveNewsBubbleController::CreateForWebContents(web_contents);
  return BraveNewsBubbleController::FromWebContents(web_contents);
}

BraveNewsBubbleController::~BraveNewsBubbleController() = default;

void BraveNewsBubbleController::ShowBubble(
    base::WeakPtr<BraveNewsActionIconView> anchor_view) {
  if (!anchor_view) {
    return;
  }

  bubble_ = new BraveNewsBubbleView(anchor_view.get(), web_contents_);
  views::BubbleDialogDelegateView::CreateBubble(
      base::WrapUnique(
          static_cast<views::BubbleDialogDelegateView*>(bubble_.get())))
      ->Show();
}

BraveNewsBubbleView* BraveNewsBubbleController::GetBubble() {
  return bubble_;
}

void BraveNewsBubbleController::OnBubbleClosed() {
  bubble_ = nullptr;
}

base::WeakPtr<BraveNewsBubbleController>
BraveNewsBubbleController::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

BraveNewsBubbleController::BraveNewsBubbleController(
    content::WebContents* web_contents)
    : content::WebContentsUserData<BraveNewsBubbleController>(*web_contents),
      web_contents_(web_contents) {}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveNewsBubbleController);

}  // namespace brave_news
