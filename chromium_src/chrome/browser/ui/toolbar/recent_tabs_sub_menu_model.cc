/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/ui/singleton_tabs.h"

namespace {

const char kBraveStubSessionTag[] = "brave_stub_more_session_tag";
const char kBraveSyncedTabsUrl[] = "brave://history/syncedTabs";

}  //  namespace

// Patched because this inserting should be done before BuildLocalEntries() in
// ctor and only once.
#define BRAVE_RECENT_TABS_SUB_MENU_MODEL_BUILD \
  InsertItemWithStringIdAt(1, IDC_CLEAR_BROWSING_DATA, IDS_CLEAR_BROWSING_DATA);

#define BRAVE_RECENT_TABS_SUB_MENU_MODEL_BUILD_TABS_FROM_OTHER_DEVICES      \
  if (tabs_in_session.size() > kMaxTabsPerSessionToShow) {                  \
    /* Not all the tabs are shown in menu */                                \
    if (!stub_tab_.get()) {                                                 \
      stub_tab_.reset(new sessions::SessionTab());                          \
      sessions::SerializedNavigationEntry stub_nav_entry;                   \
      stub_nav_entry.set_title(brave_l10n::GetLocalizedResourceUTF16String( \
          IDS_OPEN_MORE_OTHER_DEVICES_SESSIONS));                           \
      stub_nav_entry.set_virtual_url(GURL(kBraveSyncedTabsUrl));            \
      stub_tab_->navigations.push_back(stub_nav_entry);                     \
      stub_tab_->tab_id = SessionID::NewUnique();                           \
    }                                                                       \
    tabs_in_session[kMaxTabsPerSessionToShow] = stub_tab_.get();            \
    BuildOtherDevicesTabItem(this, kBraveStubSessionTag,                    \
                             *tabs_in_session[kMaxTabsPerSessionToShow]);   \
  }

#include "src/chrome/browser/ui/toolbar/recent_tabs_sub_menu_model.cc"

#undef BRAVE_RECENT_TABS_SUB_MENU_MODEL_BUILD_TABS_FROM_OTHER_DEVICES
#undef BRAVE_RECENT_TABS_SUB_MENU_MODEL_BUILD

#include "brave/browser/ui/toolbar/brave_recent_tabs_sub_menu_model.h"

// Methods of BraveRecentTabsSubMenuModel are implemented below instead of
// brave_recent_tabs_sub_menu_model.cc to have the access to functions in
// anonymous namespace in recent_tabs_sub_menu_model.cc

BraveRecentTabsSubMenuModel::BraveRecentTabsSubMenuModel(
    ui::AcceleratorProvider* accelerator_provider,
    Browser* browser)
    : RecentTabsSubMenuModel(accelerator_provider, browser) {}

BraveRecentTabsSubMenuModel::~BraveRecentTabsSubMenuModel() {}

void BraveRecentTabsSubMenuModel::ExecuteCommand(int command_id,
                                                 int event_flags) {
  if (IsTabModelCommandId(command_id)) {
    TabNavigationItems* tab_items = GetTabVectorForCommandId(command_id);
    const TabNavigationItem& item = (*tab_items)[command_id];
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
