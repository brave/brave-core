// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/browser_tabrestore.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/public/constants.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/ui/tab_ui_helper.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "content/public/browser/browser_context.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "chrome/browser/tab_contents/tab_util.h"

#define AddRestoredTab AddRestoredTab_ChromiumImpl
#define ReplaceRestoredTab ReplaceRestoredTab_ChromiumImpl

#define GetSiteInstanceForNewTab(...) \
  GetSiteInstanceForNewTab(__VA_ARGS__, std::nullopt)

#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#include <chrome/browser/ui/browser_tabrestore.cc>

#undef ReplaceRestoredTab
#undef AddRestoredTab
#if BUILDFLAG(ENABLE_CONTAINERS)
#undef GetSiteInstanceForNewTab
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

namespace chrome {

namespace {

void MaybeRestoreCustomTitleForTab(
    BraveTabStripModel* model,
    int tab_index,
    const std::map<std::string, std::string>& extra_data) {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveRenamingTabs)) {
    return;
  }

  if (!extra_data.contains(tabs::kBraveTabCustomTitleExtraDataKey) ||
      extra_data.at(tabs::kBraveTabCustomTitleExtraDataKey).empty()) {
    return;
  }

  model->SetCustomTitleForTab(
      tab_index,
      base::UTF8ToUTF16(extra_data.at(tabs::kBraveTabCustomTitleExtraDataKey)));
}

}  // namespace

content::WebContents* AddRestoredTab(
    Browser* browser,
    const std::vector<sessions::SerializedNavigationEntry>& navigations,
    int tab_index,
    int selected_navigation,
    const std::string& extension_app_id,
    std::optional<tab_groups::TabGroupId> group,
    bool select,
    bool pin,
    base::TimeTicks last_active_time_ticks,
    base::Time last_active_time,
    content::SessionStorageNamespace* storage_namespace,
    const sessions::SerializedUserAgentOverride& user_agent_override,
    const std::map<std::string, std::string>& extra_data,
    bool from_session_restore,
    std::optional<bool> is_active_browser) {
  auto* web_contents = AddRestoredTab_ChromiumImpl(
      browser, navigations, tab_index, selected_navigation, extension_app_id,
      group, select, pin, last_active_time_ticks, last_active_time,
      storage_namespace, user_agent_override, extra_data, from_session_restore,
      is_active_browser);
  MaybeRestoreCustomTitleForTab(
      static_cast<BraveTabStripModel*>(browser->tab_strip_model()), tab_index,
      extra_data);
  return web_contents;
}

content::WebContents* ReplaceRestoredTab(
    Browser* browser,
    const std::vector<sessions::SerializedNavigationEntry>& navigations,
    int selected_navigation,
    const std::string& extension_app_id,
    content::SessionStorageNamespace* session_storage_namespace,
    const sessions::SerializedUserAgentOverride& user_agent_override,
    const std::map<std::string, std::string>& extra_data,
    bool from_session_restore) {
  auto* web_contents = ReplaceRestoredTab_ChromiumImpl(
      browser, navigations, selected_navigation, extension_app_id,
      session_storage_namespace, user_agent_override, extra_data,
      from_session_restore);
  MaybeRestoreCustomTitleForTab(
      static_cast<BraveTabStripModel*>(browser->tab_strip_model()),
      selected_navigation, extra_data);
  return web_contents;
}

}  // namespace chrome
