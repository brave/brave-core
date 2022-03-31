/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_controller.h"

#include <vector>

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_model_data.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace sidebar {

namespace {

SidebarService* GetSidebarService(Browser* browser) {
  return SidebarServiceFactory::GetForProfile(browser->profile());
}

std::vector<int> GetAllExistingTabIndexForHost(Browser* browser,
                                               const std::string& host) {
  const int tab_count = browser->tab_strip_model()->count();
  std::vector<int> all_index;
  for (int i = 0; i < tab_count; ++i) {
    content::WebContents* tab = browser->tab_strip_model()->GetWebContentsAt(i);
    if (tab->GetVisibleURL().host() == host)
      all_index.push_back(i);
  }

  return all_index;
}

}  // namespace

SidebarController::SidebarController(BraveBrowser* browser, Profile* profile)
    : browser_(browser), sidebar_model_(new SidebarModel(profile)) {
  sidebar_service_observed_.Observe(GetSidebarService(browser_));
}

SidebarController::~SidebarController() = default;

bool SidebarController::IsActiveIndex(int index) const {
  return sidebar_model_->active_index() == index;
}

bool SidebarController::DoesBrowserHaveOpenedTabForItem(
    const SidebarItem& item) const {
  DCHECK(!item.open_in_panel);
  return !GetAllExistingTabIndexForHost(browser_, item.url.host()).empty();
}

void SidebarController::ActivateItemAt(int index,
                                       WindowOpenDisposition disposition) {
  // -1 means there is no active item.
  DCHECK_GE(index, -1);
  if (index == -1) {
    sidebar_model_->SetActiveIndex(index);
    return;
  }

  const auto item = sidebar_model_->GetAllSidebarItems()[index];
  // Only an item for panel can get activated.
  if (item.open_in_panel) {
    sidebar_model_->SetActiveIndex(index);
    return;
  }

  if (disposition != WindowOpenDisposition::CURRENT_TAB) {
    DCHECK_NE(WindowOpenDisposition::UNKNOWN, disposition);
    NavigateParams params(browser_->profile(), item.url,
                          ui::PAGE_TRANSITION_AUTO_BOOKMARK);
    params.disposition = disposition;
    params.browser = browser_;
    Navigate(&params);
    return;
  }

  // Iterate whenever builtin panel icon clicks.
  if (IsBuiltInType(item)) {
    IterateOrLoadAtActiveTab(item.url);
    return;
  }

  LoadAtTab(item.url);
}

void SidebarController::IterateOrLoadAtActiveTab(const GURL& url) {
  // Get target tab index
  const auto all_index = GetAllExistingTabIndexForHost(browser_, url.host());
  // Load at current active tab if there is no tab that loaded |url|.
  if (all_index.empty()) {
    auto params = GetSingletonTabNavigateParams(browser_, url);
    params.disposition = WindowOpenDisposition::CURRENT_TAB;
    Navigate(&params);
    return;
  }

  auto* tab_strip_model = browser_->tab_strip_model();
  const int active_index = tab_strip_model->active_index();
  for (const auto i : all_index) {
    if (i > active_index) {
      tab_strip_model->ActivateTabAt(i);
      return;
    }
  }

  tab_strip_model->ActivateTabAt(all_index[0]);
}

void SidebarController::LoadAtTab(const GURL& url) {
  auto params = GetSingletonTabNavigateParams(browser_, url);
  int tab_index = GetIndexOfExistingTab(browser_, params);
  // If browser has a tab that already loaded |item.url|, just activate it.
  if (tab_index >= 0) {
    browser_->tab_strip_model()->ActivateTabAt(tab_index);
  } else {
    // Load on current tab.
    params.disposition = WindowOpenDisposition::CURRENT_TAB;
    Navigate(&params);
  }
}

void SidebarController::OnShowSidebarOptionChanged(
    SidebarService::ShowSidebarOption option) {
  // Clear active state whenever sidebar enabled state is changed.
  ActivateItemAt(-1);
  UpdateSidebarVisibility();
}

void SidebarController::AddItemWithCurrentTab() {
  if (!sidebar::CanAddCurrentActiveTabToSidebar(browser_))
    return;

  auto* active_contents = browser_->tab_strip_model()->GetActiveWebContents();
  DCHECK(active_contents);
  const GURL url = active_contents->GetVisibleURL();
  const std::u16string title = active_contents->GetTitle();
  GetSidebarService(browser_)->AddItem(
      SidebarItem::Create(url, title, SidebarItem::Type::kTypeWeb,
                          SidebarItem::BuiltInItemType::kNone, false));
}

void SidebarController::SetSidebar(Sidebar* sidebar) {
  DCHECK(!sidebar_);
  // |sidebar| can be null in unit test.
  if (!sidebar)
    return;
  sidebar_ = sidebar;

  UpdateSidebarVisibility();
  sidebar_model_->Init(HistoryServiceFactory::GetForProfile(
      browser_->profile(), ServiceAccessType::EXPLICIT_ACCESS));
}

void SidebarController::UpdateSidebarVisibility() {
  DCHECK(sidebar_);
  sidebar_->SetSidebarShowOption(
      GetSidebarService(browser_)->GetSidebarShowOption());
}

}  // namespace sidebar
