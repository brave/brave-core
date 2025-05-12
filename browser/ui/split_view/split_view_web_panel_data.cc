/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/split_view/split_view_web_panel_data.h"

#include "brave/browser/ui/split_view/split_view_view.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/navigation_handle.h"
#include "url/gurl.h"

SplitViewWebPanelData::SplitViewWebPanelData() = default;
SplitViewWebPanelData::~SplitViewWebPanelData() = default;

void SplitViewWebPanelData::LoadInWebPanel(const sidebar::SidebarItem& item) {
  if (tab_for_web_panel_item_.contains(item.url)) {
    tab_for_active_web_panel_ = tab_for_web_panel_item_[item.url];
    auto tab_index = browser_->tab_strip_model()->GetIndexOfTab(tab_for_active_web_panel_);
    CHECK_NE(TabStripModel::kNoTab, tab_index);
    // Activate it.
    browser_->tab_strip_model()->ActivateTabAt(tab_index);
    return;
  }

  auto params = GetSingletonTabNavigateParams(browser_, item.url);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  if (auto handle = Navigate(&params)) {
    auto* tab = tabs::TabInterface::GetFromContents(handle->GetWebContents());
    tab_for_web_panel_item_[item.url] = tab;
    tab_for_active_web_panel_ = tab;
  }

  view_->Update();
}

void SplitViewWebPanelData::OnTabWillBeRemoved(content::WebContents* contents, int index) {
  auto* tab = tabs::TabInterface::GetFromContents(contents);
  bool need_update = false;
  if (tab == tab_for_active_web_panel_) {
    tab_for_active_web_panel_ = nullptr;
    need_update = true;
  } else if (browser_->tab_strip_model()->count() == 2) {
    // If the only tab is for web panel tab,
    // close split view.
    tab_for_active_web_panel_ = nullptr;
    need_update = true;
  }

  GURL url;
  for (auto& i : tab_for_web_panel_item_) {
    if (i.second == tab) {
      url = i.first;
      break;
    }
  }
  if (!url.is_empty()) {
    tab_for_web_panel_item_.erase(url);
  }

  if (need_update) {
    if (!tab_for_web_panel_item_.empty()) {
      tab_for_active_web_panel_ = tab_for_web_panel_item_.begin()->second;
    }
    view_->Update();
  }
}

void SplitViewWebPanelData::SetBrowser(Browser* browser) {
  browser_ = browser;
  browser_->tab_strip_model()->AddObserver(this);
}

content::WebContents* SplitViewWebPanelData::GetActivePanelContents() const {
  return HasActiveWebPanel() ? tab_for_active_web_panel_->GetContents() : nullptr;
}

void SplitViewWebPanelData::UpdateActivePanelContents(content::WebContents* new_active_contents) {
  CHECK(HasActiveWebPanel());

  auto* tab = tabs::TabInterface::GetFromContents(new_active_contents);
  for (auto& i : tab_for_web_panel_item_) {
    if (i.second == tab) {
      tab_for_active_web_panel_ = tab;
      break;
    }
  }
}

bool SplitViewWebPanelData::HasActiveWebPanel() const {
  return !!tab_for_active_web_panel_;
}

int SplitViewWebPanelData::GetSizeDelta() {
  // TODO: Set/Get current panel size.
  return 300;
}
