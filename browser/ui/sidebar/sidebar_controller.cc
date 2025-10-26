/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_controller.h"

#include <optional>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "brave/browser/ui/sidebar/sidebar.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/sidebar/sidebar_web_panel_controller.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/components/sidebar/common/features.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "components/prefs/pref_service.h"

namespace sidebar {

namespace {

SidebarService* GetSidebarService(Profile* profile) {
  return SidebarServiceFactory::GetForProfile(profile);
}

std::vector<int> GetAllExistingTabIndexForHost(TabStripModel* tab_strip_model,
                                               const std::string& host) {
  const int tab_count = tab_strip_model->count();
  std::vector<int> all_index;
  for (int i = 0; i < tab_count; ++i) {
    content::WebContents* tab = tab_strip_model->GetWebContentsAt(i);
    if (tab->GetVisibleURL().host() == host)
      all_index.push_back(i);
  }

  return all_index;
}

}  // namespace

SidebarController::SidebarController(Browser* browser, Profile* profile)
    : tab_strip_model_(browser->tab_strip_model()),
      profile_(profile),
      browser_(browser),
      sidebar_model_(new SidebarModel(profile_)) {
  sidebar_service_observed_.Observe(GetSidebarService(profile_));
}

SidebarController::~SidebarController() = default;

bool SidebarController::IsActiveIndex(std::optional<size_t> index) const {
  return sidebar_model_->active_index() == index;
}

bool SidebarController::DoesBrowserHaveOpenedTabForItem(
    const SidebarItem& item) const {
  // This method is only for builtin item's icon state updating.
  DCHECK(item.is_built_in_type());
  DCHECK(!item.open_in_panel);

  const std::vector<Browser*> browsers =
      chrome::FindAllTabbedBrowsersWithProfile(profile_);
  for (Browser* browser : browsers) {
    const auto all_index = GetAllExistingTabIndexForHost(
        browser->tab_strip_model(), item.url.host());
    if (!all_index.empty())
      return true;
  }

  return false;
}

void SidebarController::TearDownPreBrowserWindowDestruction() {
  sidebar_service_observed_.Reset();
  sidebar_ = nullptr;
}

void SidebarController::ActivateItemAt(std::optional<size_t> index,
                                       WindowOpenDisposition disposition) {
  // disengaged means there is no active item.
  if (!index) {
    sidebar_model_->SetActiveIndex(index);
    return;
  }

  DCHECK_LT(index.value(), sidebar_model_->GetAllSidebarItems().size());

  const auto& item = sidebar_model_->GetAllSidebarItems()[*index];

  if (item.is_web_panel_type()) {
    // TODO(https://github.com/brave/brave-browser/issues/33533): web panel item
    // also should be activated.
    GetWebPanelController()->IsShowingWebPanel()
        ? GetWebPanelController()->CloseWebPanel()
        : GetWebPanelController()->OpenWebPanel(item);
    return;
  }

  // Only an item for panel can get activated.
  if (!item.is_web_type() && item.open_in_panel) {
    sidebar_model_->SetActiveIndex(index);

#if BUILDFLAG(ENABLE_AI_CHAT)
    if (sidebar::features::kOpenOneShotLeoPanel.Get() &&
        item.built_in_item_type == SidebarItem::BuiltInItemType::kChatUI) {
      // Prevent one-time Leo panel open.
      profile_->GetPrefs()->SetBoolean(kLeoPanelOneShotOpen, true);
    }
#endif
    return;
  }

  if (disposition != WindowOpenDisposition::CURRENT_TAB) {
    DCHECK_NE(WindowOpenDisposition::UNKNOWN, disposition);
    NavigateParams params(profile_, item.url,
                          ui::PAGE_TRANSITION_AUTO_BOOKMARK);
    params.disposition = disposition;
    params.browser = browser_;
    Navigate(&params);
    return;
  }

  // Iterate whenever builtin shortcut type item icon clicks.
  if (item.is_built_in_type()) {
    IterateOrLoadAtActiveTab(item.url);
    return;
  }

  LoadAtTab(item.url);
}

void SidebarController::ActivatePanelItem(
    SidebarItem::BuiltInItemType panel_item) {
  // For panel item activation, SidePanelUI is the single source of truth.
  auto* side_panel_ui = browser_->GetFeatures().side_panel_ui();
  if (!side_panel_ui) {
    return;
  }
  CHECK(side_panel_ui);
  if (panel_item == SidebarItem::BuiltInItemType::kNone) {
    side_panel_ui->Close();
    return;
  }

  side_panel_ui->Show(sidebar::SidePanelIdFromSideBarItemType(panel_item));
}

void SidebarController::DeactivateCurrentPanel() {
  ActivatePanelItem(SidebarItem::BuiltInItemType::kNone);
}

bool SidebarController::ActiveTabFromOtherBrowsersForHost(const GURL& url) {
  const std::vector<Browser*> browsers =
      chrome::FindAllTabbedBrowsersWithProfile(profile_);
  for (Browser* browser : browsers) {
    // Skip current browser. we are here because current active browser doesn't
    // have a tab that loads |url|.
    if (browser == browser_) {
      continue;
    }

    const auto all_index =
        GetAllExistingTabIndexForHost(browser->tab_strip_model(), url.host());
    if (all_index.empty())
      continue;

    // Pick first tab for simplicity.
    browser->tab_strip_model()->ActivateTabAt(all_index[0]);
    browser->window()->Activate();
    return true;
  }

  return false;
}

void SidebarController::IterateOrLoadAtActiveTab(const GURL& url) {
  // Get target tab index
  const auto all_index =
      GetAllExistingTabIndexForHost(tab_strip_model_, url.host());
  if (all_index.empty()) {
    if (ActiveTabFromOtherBrowsersForHost(url))
      return;

    // Load at current active tab if there is no tab that loaded |url|.
    auto params = GetSingletonTabNavigateParams(browser_, url);
    params.disposition = WindowOpenDisposition::CURRENT_TAB;
    Navigate(&params);
    return;
  }

  const int active_index = tab_strip_model_->active_index();
  for (const auto i : all_index) {
    if (i > active_index) {
      tab_strip_model_->ActivateTabAt(i);
      return;
    }
  }

  tab_strip_model_->ActivateTabAt(all_index[0]);
}

void SidebarController::LoadAtTab(const GURL& url) {
  auto params = GetSingletonTabNavigateParams(browser_, url);
  int tab_index = GetIndexOfExistingTab(browser_, params);
  // If browser has a tab that already loaded |item.url|, just activate it.
  if (tab_index >= 0) {
    tab_strip_model_->ActivateTabAt(tab_index);
  } else {
    // Load on current tab.
    params.disposition = WindowOpenDisposition::CURRENT_TAB;
    Navigate(&params);
  }
}

void SidebarController::OnShowSidebarOptionChanged(
    SidebarService::ShowSidebarOption option) {
  CHECK(sidebar_);
  sidebar_->SetSidebarShowOption(option);
}

void SidebarController::AddItemWithCurrentTab() {
  if (!sidebar::CanAddCurrentActiveTabToSidebar(browser_)) {
    return;
  }

  auto* active_contents = tab_strip_model_->GetActiveWebContents();
  DCHECK(active_contents);
  const GURL url = active_contents->GetVisibleURL();
  const std::u16string title = active_contents->GetTitle();
  GetSidebarService(profile_)->AddItem(SidebarItem::Create(
      url, title, SidebarItem::Type::kTypeWeb,
      SidebarItem::BuiltInItemType::kNone, IsWebPanelFeatureEnabled()));
}

void SidebarController::UpdateActiveItemState(
    std::optional<SidebarItem::BuiltInItemType> active_panel_item) {
  if (!active_panel_item) {
    ActivateItemAt(std::nullopt);
    return;
  }

  if (auto index = sidebar_model_->GetIndexOf(*active_panel_item)) {
    ActivateItemAt(*index);
  }
}

void SidebarController::SetSidebar(Sidebar* sidebar) {
  DCHECK(!sidebar_);
  // |sidebar| can be null in unit test.
  if (!sidebar)
    return;
  sidebar_ = sidebar;

  sidebar_model_->Init(HistoryServiceFactory::GetForProfile(
      profile_, ServiceAccessType::EXPLICIT_ACCESS));
}

SidebarWebPanelController* SidebarController::GetWebPanelController() {
  CHECK(IsWebPanelFeatureEnabled());
  if (!web_panel_controller_) {
    web_panel_controller_ =
        std::make_unique<SidebarWebPanelController>(browser_->GetBrowserView());
  }

  return web_panel_controller_.get();
}

}  // namespace sidebar
