/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_shields_page_info_controller.h"

#include "base/check_deref.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/ui/views/page_info/brave_page_info_bubble_view.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/location_bar/location_icon_view.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_specification.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/view_utils.h"

namespace {

brave_shields::BraveShieldsTabHelper* GetShieldsHelper(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return nullptr;
  }
  return brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents);
}

}  // namespace

BraveShieldsPageInfoController::BraveShieldsPageInfoController(
    TabStripModel* tab_strip_model,
    LocationIconView* location_icon_view)
    : tab_strip_model_(CHECK_DEREF(tab_strip_model)),
      location_icon_view_(CHECK_DEREF(location_icon_view)) {
  tab_strip_model_->AddObserver(this);
  auto* web_contents = tab_strip_model_->GetActiveWebContents();
  if (auto* shields_helper = GetShieldsHelper(web_contents)) {
    shields_helper->AddObserver(this);
  }
}

BraveShieldsPageInfoController::~BraveShieldsPageInfoController() {
  auto* web_contents = tab_strip_model_->GetActiveWebContents();
  if (auto* shields_helper = GetShieldsHelper(web_contents)) {
    shields_helper->RemoveObserver(this);
  }
}

void BraveShieldsPageInfoController::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    if (auto* shields_helper = GetShieldsHelper(selection.old_contents)) {
      shields_helper->RemoveObserver(this);
    }
    if (auto* shields_helper = GetShieldsHelper(selection.new_contents)) {
      shields_helper->AddObserver(this);
    }
  }
}

void BraveShieldsPageInfoController::OnResourcesChanged() {}

void BraveShieldsPageInfoController::OnRepeatedReloadsDetected() {
  // Post a task to show the page info bubble. Since this event occurs in
  // response to a navigation finished event, and the page info bubble may close
  // itself when a navigation finished event occurs, posting a task eliminates
  // the possibility that the bubble will immediately close.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &BraveShieldsPageInfoController::ShowBubbleForRepeatedReloads,
          weak_factory_.GetWeakPtr()));
}

void BraveShieldsPageInfoController::ShowBubbleForRepeatedReloads() {
  auto* web_contents = tab_strip_model_->GetActiveWebContents();
  if (!web_contents) {
    return;
  }

  content::NavigationEntry* entry =
      web_contents->GetController().GetVisibleEntry();
  if (!entry || entry->IsInitialEntry()) {
    return;
  }

  std::unique_ptr<PageInfoBubbleSpecification> specification =
      PageInfoBubbleSpecification::Builder(
          &location_icon_view_.get(),
          location_icon_view_->GetWidget()->GetNativeWindow(), web_contents,
          entry->GetVirtualURL())
          .Build();

  views::BubbleDialogDelegateView* bubble =
      PageInfoBubbleView::CreatePageInfoBubble(std::move(specification));

  auto* page_info_bubble = views::AsViewClass<BravePageInfoBubbleView>(bubble);
  CHECK(page_info_bubble);
  page_info_bubble->SetHighlightedButton(&location_icon_view_.get());
  page_info_bubble->GetWidget()->Show();
  page_info_bubble->OpenShieldsPageAfterRepeatedReloads();
}
