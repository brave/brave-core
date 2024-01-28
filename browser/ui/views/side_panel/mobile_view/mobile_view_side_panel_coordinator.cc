// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/side_panel/mobile_view/mobile_view_side_panel_coordinator.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/view.h"
#include "url/gurl.h"

MobileViewSidePanelCoordinator::MobileViewSidePanelCoordinator(
    Browser& browser,
    SidePanelRegistry& global_registry,
    const GURL& url)
    : browser_(browser), global_registry_(global_registry), url_(url) {
  CHECK(url_.is_valid());
  global_registry_->Register(std::make_unique<SidePanelEntry>(
      GetEntryKey(), u"MobileView", ui::ImageModel(),
      base::BindRepeating(&MobileViewSidePanelCoordinator::CreateView,
                          base::Unretained(this))));
}

MobileViewSidePanelCoordinator::~MobileViewSidePanelCoordinator() {
  DeregisterEntry();
}

std::unique_ptr<views::View> MobileViewSidePanelCoordinator::CreateView() {
  auto view = std::make_unique<views::View>();
  view->SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical);

  // TODO(simonhong): Implement mobile view top UI.
  label_ = view->AddChildView(std::make_unique<views::Label>());
  auto* webview =
      view->AddChildView(std::make_unique<views::WebView>(browser_->profile()));
  webview->LoadInitialURL(url_);
  Observe(webview->GetWebContents());
  webview->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kUnbounded));

  return view;
}

void MobileViewSidePanelCoordinator::PrimaryPageChanged(content::Page& page) {
  label_->SetText(base::UTF8ToUTF16(
      web_contents()->GetVisibleURL().possibly_invalid_spec()));
}

SidePanelEntry::Key MobileViewSidePanelCoordinator::GetEntryKey() const {
  return SidePanelEntry::Key(SidePanelEntry::Id::kMobileView,
                             sidebar::MobileViewId(url_.spec()));
}

void MobileViewSidePanelCoordinator::DeregisterEntry() {
  global_registry_->Deregister(GetEntryKey());
}
