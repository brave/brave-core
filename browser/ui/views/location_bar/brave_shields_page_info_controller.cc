/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_shields_page_info_controller.h"

#include "base/check_deref.h"
#include "brave/browser/ui/views/page_info/brave_page_info_bubble_view.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/location_bar/location_icon_view.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_specification.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/view_utils.h"

BraveShieldsPageInfoController::BraveShieldsPageInfoController(
    TabStripModel* tab_strip_model,
    LocationIconView* location_icon_view)
    : tab_strip_model_(CHECK_DEREF(tab_strip_model)),
      location_icon_view_(CHECK_DEREF(location_icon_view)) {
  tab_strip_model_->AddObserver(this);
  UpdateShieldsObservation(tab_strip_model_->GetActiveWebContents());
}

BraveShieldsPageInfoController::~BraveShieldsPageInfoController() = default;

void BraveShieldsPageInfoController::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    UpdateShieldsObservation(selection.new_contents);
  }
}

void BraveShieldsPageInfoController::OnResourcesChanged() {}

void BraveShieldsPageInfoController::OnRepeatedReloadsDetected() {
  ShowBubbleForRepeatedReloads();
}

void BraveShieldsPageInfoController::UpdateShieldsObservation(
    content::WebContents* web_contents) {
  shields_observation_.Reset();
  if (web_contents) {
    shields_observation_.Observe(
        brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents));
  }
}

void BraveShieldsPageInfoController::ShowBubbleForRepeatedReloads() {
  if (!shields_observation_.IsObserving()) {
    return;
  }

  auto* web_contents = shields_observation_.GetSource()->web_contents();
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
