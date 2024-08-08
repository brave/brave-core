/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "chrome/browser/ui/browser_tabrestore.h"

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/browser/extensions/brave_webtorrent_navigation_throttle.h"
#include "brave/components/brave_webtorrent/browser/webtorrent_util.h"
#include "chrome/browser/extensions/extension_service.h"
#include "extensions/browser/extension_system.h"
#endif

#define AddRestoredTab AddRestoredTab_ChromiumImpl
#include "src/chrome/browser/ui/browser_tabrestore.cc"
#undef AddRestoredTab

namespace {

#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
void MaybeLoadWebtorrent(Browser* browser,
                         bool from_session_restore,
                         const GURL& restore_url) {
  if (!from_session_restore || !webtorrent::IsWebtorrentURL(restore_url))
    return;

  extensions::BraveWebTorrentNavigationThrottle::MaybeLoadWebtorrent(
      browser->profile(), restore_url);
}
#endif

}  // namespace

namespace chrome {

WebContents* AddRestoredTab(
    Browser* browser,
    const std::vector<SerializedNavigationEntry>& navigations,
    int tab_index,
    int selected_navigation,
    const std::string& extension_app_id,
    std::optional<tab_groups::TabGroupId> group,
    bool select,
    bool pin,
    base::TimeTicks last_active_time_ticks,
    base::Time last_active_time,
    content::SessionStorageNamespace* session_storage_namespace,
    const sessions::SerializedUserAgentOverride& user_agent_override,
    const std::map<std::string, std::string>& extra_data,
    bool from_session_restore,
    std::optional<bool> is_active_browser) {
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  MaybeLoadWebtorrent(
      browser,
      from_session_restore,
      navigations.at(selected_navigation).original_request_url());
#endif

  return AddRestoredTab_ChromiumImpl(
      browser, navigations, tab_index, selected_navigation, extension_app_id,
      group, select, pin, last_active_time_ticks, last_active_time,
      session_storage_namespace, user_agent_override, extra_data,
      from_session_restore, is_active_browser);
}

}  // namespace chrome
