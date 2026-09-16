/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "components/sync_sessions/open_tabs_ui_delegate.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

constexpr char kBraveStubSessionTag[] = "brave_stub_more_session_tag";
constexpr char kBraveSyncedTabsUrl[] = "brave://history/syncedTabs";

}  //  namespace

#include <chrome/browser/ui/tabs/recent_tabs_sub_menu_model.cc>

#include "brave/browser/ui/toolbar/brave_recent_tabs_sub_menu_model.h"

// Methods of BraveRecentTabsSubMenuModel and RecentTabsSubMenuModel's
// MaybeAppendOverflowStubTab() are implemented below instead of
// brave_recent_tabs_sub_menu_model.cc to have the access to functions in
// anonymous namespace in recent_tabs_sub_menu_model.cc

void RecentTabsSubMenuModel::MaybeAppendOverflowStubTab(
    std::vector<const sessions::SessionTab*>& tabs_in_session,
    size_t max_tabs_to_show,
    SimpleMenuModel* device_menu_model) {
  if (tabs_in_session.size() <= max_tabs_to_show) {
    return;
  }
  // Not all the tabs are shown in menu.
  if (!stub_tab_.get()) {
    stub_tab_.reset(new sessions::SessionTab());
    sessions::SerializedNavigationEntry stub_nav_entry;
    stub_nav_entry.set_title(
        l10n_util::GetStringUTF16(IDS_OPEN_MORE_OTHER_DEVICES_SESSIONS));
    stub_nav_entry.set_virtual_url(GURL(kBraveSyncedTabsUrl));
    stub_tab_->navigations.push_back(stub_nav_entry);
    stub_tab_->tab_id = SessionID::NewUnique();
  }
  tabs_in_session[max_tabs_to_show] = stub_tab_.get();
  BuildOtherDevicesTabItem(device_menu_model, kBraveStubSessionTag,
                           *tabs_in_session[max_tabs_to_show]);
}

BraveRecentTabsSubMenuModel::BraveRecentTabsSubMenuModel(
    ui::AcceleratorProvider* accelerator_provider,
    BrowserWindowInterface* browser)
    : RecentTabsSubMenuModel(accelerator_provider, browser) {}

BraveRecentTabsSubMenuModel::~BraveRecentTabsSubMenuModel() {}

void BraveRecentTabsSubMenuModel::ExecuteCommand(int command_id,
                                                 int event_flags) {
  if (IsCommandType(CommandType::Tab, command_id)) {
    const TabItems& tab_items = *GetTabVectorForCommandId(command_id);
    const TabItem& item = tab_items.at(command_id);
    DCHECK(item.tab_id.is_valid() && item.url.is_valid());

    if (item.session_tag == kBraveStubSessionTag) {
      ShowSingletonTabOverwritingNTP(browser_, GURL(kBraveSyncedTabsUrl));
      return;
    }
  }

  if (command_id == IDC_CLEAR_BROWSING_DATA) {
    chrome::ExecuteCommand(browser_, command_id);
    return;
  }

  RecentTabsSubMenuModel::ExecuteCommand(command_id, event_flags);
}
