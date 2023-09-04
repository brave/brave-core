/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_controller.h"

#include <vector>

#include "base/check_op.h"
#include "base/containers/contains.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_tab_helper.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace sidebar {

namespace {

constexpr SidePanelEntryId kTabSpecificPanelEntryIds[] = {
    SidePanelEntryId::kChatUI};

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
  browser->tab_strip_model()->AddObserver(this);
}

SidebarController::~SidebarController() = default;

bool SidebarController::IsActiveIndex(absl::optional<size_t> index) const {
  return sidebar_model_->active_index() == index;
}

bool SidebarController::GetIsPanelOperationFromActiveTabChangeAndReset() {
  return std::exchange(operation_from_active_tab_change_, false);
}

bool SidebarController::DoesBrowserHaveOpenedTabForItem(
    const SidebarItem& item) const {
  // This method is only for builtin item's icon state updating.
  DCHECK(sidebar::IsBuiltInType(item));
  DCHECK(!item.open_in_panel);

  const std::vector<Browser*> browsers =
      chrome::FindAllTabbedBrowsersWithProfile(browser_->profile());
  for (Browser* browser : browsers) {
    const auto all_index =
        GetAllExistingTabIndexForHost(browser, item.url.host());
    if (!all_index.empty())
      return true;
  }

  return false;
}

void SidebarController::ActivateItemAt(absl::optional<size_t> index,
                                       WindowOpenDisposition disposition) {
  SidebarTabHelper* helper = nullptr;
  if (auto* active_tab = browser_->tab_strip_model()->GetActiveWebContents()) {
    helper = SidebarTabHelper::FromWebContents(active_tab);
  }
  DVLOG(1) << __func__
           << " index: " << (index ? base::NumberToString(*index) : "none")
           << " helper: " << helper;

  // disengaged means there is no active item.
  if (!index) {
    sidebar_model_->SetActiveIndex(index);

    if (helper) {
      // This tab doesn't have a tab-specific panel now
      helper->RegisterPanelInactive();
    }
    return;
  }

  DCHECK_LT(index.value(), sidebar_model_->GetAllSidebarItems().size());

  const auto& item = sidebar_model_->GetAllSidebarItems()[*index];
  // Only an item for panel can get activated.
  if (item.open_in_panel) {
    sidebar_model_->SetActiveIndex(index);
    // Handle when item type is not tab-specific
    if (!base::Contains(kTabSpecificPanelEntryIds,
                        SidePanelIdFromSideBarItem(item))) {
      // Register that this tab doesn't have a tab-specific panel anymore.
      if (helper) {
        helper->RegisterPanelInactive();
      }
    } else {
      // Handle when item type is tab-specific
      if (helper) {
        // Register that this tab now has a tab-specific panel.
        helper->RegisterPanelActive(item.built_in_item_type);
      }
    }
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

  // Iterate whenever builtin shortcut type item icon clicks.
  if (IsBuiltInType(item)) {
    IterateOrLoadAtActiveTab(item.url);
    return;
  }

  LoadAtTab(item.url);
}

void SidebarController::ActivatePanelItem(
    SidebarItem::BuiltInItemType panel_item) {
  // For panel item activation, SidePanelUI is the single source of truth.
  auto* panel_ui = SidePanelUI::GetSidePanelUIForBrowser(browser_);
  if (!panel_ui) {
    return;
  }
  if (panel_item == SidebarItem::BuiltInItemType::kNone) {
    panel_ui->Close();
    return;
  }

  panel_ui->Show(sidebar::SidePanelIdFromSideBarItemType(panel_item));
}

void SidebarController::DeactivateCurrentPanel() {
  ActivatePanelItem(SidebarItem::BuiltInItemType::kNone);
}

bool SidebarController::ActiveTabFromOtherBrowsersForHost(const GURL& url) {
  const std::vector<Browser*> browsers =
      chrome::FindAllTabbedBrowsersWithProfile(browser_->profile());
  for (Browser* browser : browsers) {
    // Skip current browser. we are here because current active browser doesn't
    // have a tab that loads |url|.
    if (browser == browser_)
      continue;

    const auto all_index = GetAllExistingTabIndexForHost(browser, url.host());
    if (all_index.empty())
      continue;

    // Pick first tab for simplicity.
    browser->tab_strip_model()->ActivateTabAt(all_index[0]);
    browser->window()->Activate();
    return true;
  }

  return false;
}

void SidebarController::SetBrowserActivePanelKey(
    absl::optional<SidePanelEntryKey> entry_key) {
  DCHECK(entry_key);
  if (base::Contains(kTabSpecificPanelEntryIds, entry_key->id())) {
    return;
  }

  // Remember to restore this item if any Tab opens a tab-specific panel in
  // the future.
  browser_active_panel_key_ = entry_key;
}

void SidebarController::ClearBrowserActivePanelKey() {
  browser_active_panel_key_ = absl::nullopt;
}

void SidebarController::IterateOrLoadAtActiveTab(const GURL& url) {
  // Get target tab index
  const auto all_index = GetAllExistingTabIndexForHost(browser_, url.host());
  if (all_index.empty()) {
    if (ActiveTabFromOtherBrowsersForHost(url))
      return;

    // Load at current active tab if there is no tab that loaded |url|.
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
  sidebar_->SetSidebarShowOption(option);
}

void SidebarController::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (!selection.active_tab_changed()) {
    return;
  }

  auto* panel_ui = SidePanelUI::GetSidePanelUIForBrowser(browser_);
  if (!panel_ui) {
    return;
  }

  // When the active tab changes, make sure to restore either any tab-specific
  // panel, or the browser-wide panel if the tab doesn't have a tab-specific
  // panel.
  DVLOG(1) << __func__ << " : Active tab changed.";

  // Is there a tab-specific panel for the new active tab?
  absl::optional<SidebarItem::BuiltInItemType> wanted_panel =
      selection.new_contents
          ? SidebarTabHelper::FromWebContents(selection.new_contents)
                ->active_panel()
          : absl::nullopt;
  if (wanted_panel.has_value()) {
    auto wanted_index = sidebar_model_->GetIndexOf(wanted_panel.value());
    // built-in items can be removed in-between tab activations, so we might
    // not get a valid index for this type.
    if (wanted_index.has_value() &&
        sidebar_model_->active_index() != wanted_index.value()) {
      // Don't pass through null values, as we don't want to close existing
      // panels if we're not opening a tab-specific panel.
      if (!panel_ui->GetCurrentEntryId()) {
        // Open/Close by tab change doesn't need animation.
        // UI could refer this for prevent animation.
        operation_from_active_tab_change_ = true;
      }

      DVLOG(1) << __func__ << " : Show per-tab panel";
      ActivatePanelItem(wanted_panel.value());
    }
    return;
  }

  // If active tab has contextual side panel, we should not activate another
  // panel here. Instead, SidePanelCoordinator::OnTabStripModelChanged()
  // will set contextual panel for active tab. As SidePanelRegistry is only
  // available from views, we need to ask it to view layer(sidebar_).
  // TODO(simonhong): Check kChatUI could be contextual entry instead of
  // handling via SidebarTabHelper.
  if (sidebar_->HasActiveContextualEntry()) {
    DVLOG(1) << __func__
             << " : Just return cause there is active contextual entry.";
    return;
  }

  // There is no tab-specific panel for the new active tab
  if (browser_active_panel_key_) {
    // Show browser active panel if existed.
    DVLOG(1) << __func__ << " : Show browser active panel.";
    panel_ui->Show(browser_active_panel_key_.value());
  } else {
    // Otherwise, close panel if it was previous tab specific panel.
    auto current_id = panel_ui->GetCurrentEntryId();
    if (current_id && base::Contains(kTabSpecificPanelEntryIds, *current_id)) {
      DVLOG(1) << __func__ << " : Close previous per-tab panel.";
      operation_from_active_tab_change_ = true;
      panel_ui->Close();
    }
  }
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

  sidebar_model_->Init(HistoryServiceFactory::GetForProfile(
      browser_->profile(), ServiceAccessType::EXPLICIT_ACCESS));
}

}  // namespace sidebar
