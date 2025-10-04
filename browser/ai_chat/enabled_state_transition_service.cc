// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/enabled_state_transition_service.h"

#include "base/check.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_ui_data_source.h"

#if !BUILDFLAG(IS_ANDROID)
#include <algorithm>

#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#endif

namespace ai_chat {

EnabledStateTransitionService::EnabledStateTransitionService(
    content::BrowserContext* context)
    : profile_(Profile::FromBrowserContext(context)),
#if !BUILDFLAG(IS_ANDROID)
      sidebar_service_(sidebar::SidebarServiceFactory::GetForProfile(profile_)),
#endif
      prefs_(user_prefs::UserPrefs::Get(context)) {
  CHECK(profile_);
  CHECK(prefs_);
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      prefs::kEnabledByPolicy,
      base::BindRepeating(
          &EnabledStateTransitionService::OnEnabledByPolicyChanged,
          base::Unretained(this)));
}

EnabledStateTransitionService::~EnabledStateTransitionService() = default;

void EnabledStateTransitionService::OnEnabledByPolicyChanged() {
  const bool enabled = IsAIChatEnabled(prefs_);
  if (!enabled) {
    content::WebUIDataSource::CreateAndAdd(profile_, kAIChatUIHost);
#if !BUILDFLAG(IS_ANDROID)
    CloseAIChatTabs();
#endif
  }
#if !BUILDFLAG(IS_ANDROID)
  UpdateSidebarState(enabled);
#endif
}

#if !BUILDFLAG(IS_ANDROID)
void EnabledStateTransitionService::CloseAIChatTabs() {
  for (Browser* browser : *BrowserList::GetInstance()) {
    if (browser->profile() != profile_) {
      continue;
    }

    TabStripModel* tab_strip = browser->tab_strip_model();

    // Deregister kChatUI from each tab's side panel registry
    for (int i = 0; i < tab_strip->count(); ++i) {
      if (auto* tab = tab_strip->GetTabAtIndex(i)) {
        if (SidePanelRegistry* tab_registry =
                tab->GetTabFeatures()->side_panel_registry()) {
          tab_registry->Deregister(
              SidePanelEntry::Key(SidePanelEntryId::kChatUI));
        }
      }
    }

    // Close any tabs showing AI Chat UI
    for (int i = tab_strip->count() - 1; i >= 0; --i) {
      auto* web_contents = tab_strip->GetWebContentsAt(i);
      const GURL& url = web_contents->GetLastCommittedURL();
      if (url.SchemeIs(content::kChromeUIScheme) &&
          url.host() == kAIChatUIHost) {
        tab_strip->CloseWebContentsAt(i, TabCloseTypes::CLOSE_NONE);
      }
    }
  }
}

void EnabledStateTransitionService::UpdateSidebarState(bool enabled) {
  if (enabled) {
    const auto hidden_items = sidebar_service_->GetHiddenDefaultSidebarItems();
    const auto iter = std::ranges::find(
        hidden_items, sidebar::SidebarItem::BuiltInItemType::kChatUI,
        &sidebar::SidebarItem::built_in_item_type);
    if (iter != hidden_items.end()) {
      sidebar_service_->AddItem(*iter);
    }
  } else {
    const auto visible_items = sidebar_service_->items();
    const auto iter = std::ranges::find(
        visible_items, sidebar::SidebarItem::BuiltInItemType::kChatUI,
        &sidebar::SidebarItem::built_in_item_type);
    if (iter != visible_items.end()) {
      sidebar_service_->RemoveItemAt(iter - visible_items.begin());
    }
  }
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace ai_chat
