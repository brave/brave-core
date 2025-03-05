/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/brave_side_panel_view_base.h"

#include "brave/browser/ui/color/brave_color_id.h"
#include "chrome/browser/ui/views/side_panel/side_panel_content_proxy.h"
#include "chrome/browser/ui/views/side_panel/side_panel_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/background.h"

BraveSidePanelViewBase::BraveSidePanelViewBase() {
  // Originally SidePanelEntry's Content was
  // [ReadLater|Bookmarks]SidePanelWebView and it's availability is set to true
  // when SidePanelWebUIView::ShowUI() and then proxy's availability callback is
  // executed. However, we use parent view(BraveReadLaterSidePanelView) to have
  // panel specific header view and this class becomes SidePanelEntry's Content.
  // To make this content available when SidePanelWebUIVew::ShowUI() is called,
  // this observes WebView's visibility because it's set as visible when
  // ShowUI() is called.
  // NOTE: If we use our own reading list page and it has loading spinner, maybe
  // we can set `true` here.
  SidePanelUtil::GetSidePanelContentProxy(this)->SetAvailable(false);
  SetBackground(
      views::CreateSolidBackground(kColorSidebarPanelHeaderBackground));
}

BraveSidePanelViewBase::~BraveSidePanelViewBase() = default;

void BraveSidePanelViewBase::StartObservingWebWebViewVisibilityChange(
    views::View* web_view) {
  view_observation_.Observe(web_view);
}

void BraveSidePanelViewBase::OnViewVisibilityChanged(
    views::View* observed_view,
    views::View* starting_view) {
  // Once it becomes available, stop observing becuase its availablity is
  // not changed from now on.
  if (observed_view->GetVisible()) {
    SidePanelUtil::GetSidePanelContentProxy(this)->SetAvailable(true);
    view_observation_.Reset();
  }
}

BEGIN_METADATA(BraveSidePanelViewBase)
END_METADATA
