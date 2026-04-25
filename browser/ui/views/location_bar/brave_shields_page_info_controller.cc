/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_shields_page_info_controller.h"

#include "base/check_deref.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/ui/views/page_info/brave_page_info_bubble_view.h"
#include "chrome/browser/ui/user_education/browser_user_education_interface.h"
#include "chrome/browser/ui/views/location_bar/location_icon_view.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_specification.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/page_info/page_info.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/view_utils.h"
#include "url/gurl.h"

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
    LocationIconView* location_icon_view)
    : location_icon_view_(CHECK_DEREF(location_icon_view)) {}

BraveShieldsPageInfoController::~BraveShieldsPageInfoController() {
  if (auto* shields_helper = GetShieldsHelper(web_contents())) {
    shields_helper->RemoveObserver(this);
  }
}

void BraveShieldsPageInfoController::UpdateWebContents(
    content::WebContents* contents) {
  if (!contents || contents == web_contents()) {
    return;
  }

  // Stop observing the shields tab helper for the old WebContents.
  if (auto* shields_helper = GetShieldsHelper(web_contents())) {
    shields_helper->RemoveObserver(this);
  }

  // Start observing the new WebContents and its shields helper.
  Observe(contents);
  if (auto* shields_helper = GetShieldsHelper(contents)) {
    shields_helper->AddObserver(this);
  }

  MaybeShowShieldsIPH();
}

void BraveShieldsPageInfoController::PrimaryPageChanged(content::Page& page) {
  MaybeShowShieldsIPH();
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

void BraveShieldsPageInfoController::MaybeShowShieldsIPH() {
  if (!web_contents()) {
    return;
  }

  content::NavigationEntry* entry =
      web_contents()->GetController().GetVisibleEntry();
  if (entry->IsInitialEntry()) {
    return;
  }

  // Only attempt to show the IPH if the "normal" PageInfo bubble will be
  // displayed for this URL.
  GURL url = entry->GetVirtualURL();
  if (PageInfo::IsFileOrInternalPage(url) ||
      url.SchemeIs(content_settings::kExtensionScheme)) {
    return;
  }

  if (auto* user_education =
          BrowserUserEducationInterface::MaybeGetForWebContentsInTab(
              web_contents())) {
    user_education->MaybeShowFeaturePromo(
        feature_engagement::kIPHBraveShieldsInPageInfoFeature);
  }
}

void BraveShieldsPageInfoController::ShowBubbleForRepeatedReloads() {
  if (!web_contents()) {
    return;
  }

  content::NavigationEntry* entry =
      web_contents()->GetController().GetVisibleEntry();
  if (!entry || entry->IsInitialEntry()) {
    return;
  }

  std::unique_ptr<PageInfoBubbleSpecification> specification =
      PageInfoBubbleSpecification::Builder(
          &location_icon_view_.get(),
          location_icon_view_->GetWidget()->GetNativeWindow(), web_contents(),
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
