/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/leo_assistant_api.h"

#include <vector>

#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/ai_chat/common/pref_names.h"
#include "brave/components/sidebar/sidebar_item.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

namespace {

const std::vector<sidebar::SidebarItem>::const_iterator FindAiChatSidebarItem(
    const std::vector<sidebar::SidebarItem>& items) {
  return base::ranges::find_if(items, [](const auto& item) {
    return item.built_in_item_type ==
           sidebar::SidebarItem::BuiltInItemType::kChatUI;
  });
}

bool ShowLeoAssistantIconVisibleIfNot(
    sidebar::SidebarService* sidebar_service) {
  const auto hidden_items = sidebar_service->GetHiddenDefaultSidebarItems();
  const auto item_hidden_iter = FindAiChatSidebarItem(hidden_items);

  if (item_hidden_iter != hidden_items.end()) {
    sidebar_service->AddItem(*item_hidden_iter);
    return true;
  }

  return false;
}

bool HideLeoAssistantIconifNot(sidebar::SidebarService* sidebar_service) {
  const auto visible_items = sidebar_service->items();
  const auto item_visible_iter = FindAiChatSidebarItem(visible_items);

  if (item_visible_iter != visible_items.end()) {
    sidebar_service->RemoveItemAt(item_visible_iter - visible_items.begin());
    return true;
  }

  return false;
}

}  // namespace

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction LeoSetShowLeoAssistantIconFunction::Run() {
  auto* service = sidebar::SidebarServiceFactory::GetForProfile(
      Profile::FromBrowserContext(browser_context()));

  if (ShowLeoAssistantIconVisibleIfNot(service) ||
      HideLeoAssistantIconifNot(service)) {
    return RespondNow(WithArguments(true));
  }

  return RespondNow(WithArguments(false));
}

ExtensionFunction::ResponseAction LeoGetShowLeoAssistantIconFunction::Run() {
  auto* service = sidebar::SidebarServiceFactory::GetForProfile(
      Profile::FromBrowserContext(browser_context()));
  const auto hidden_items = service->GetHiddenDefaultSidebarItems();
  return RespondNow(
      WithArguments(!base::ranges::any_of(hidden_items, [](const auto& item) {
        return item.built_in_item_type ==
               sidebar::SidebarItem::BuiltInItemType::kChatUI;
      })));
}

ExtensionFunction::ResponseAction LeoResetFunction::Run() {
  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* service = sidebar::SidebarServiceFactory::GetForProfile(profile);

  ShowLeoAssistantIconVisibleIfNot(service);
  profile->GetPrefs()->SetBoolean(ai_chat::prefs::kBraveChatHasSeenDisclaimer,
                                  false);
  profile->GetPrefs()->SetBoolean(
      ai_chat::prefs::kBraveChatAutoGenerateQuestions, false);

  return RespondNow(WithArguments(true));
}

}  // namespace api
}  // namespace extensions
