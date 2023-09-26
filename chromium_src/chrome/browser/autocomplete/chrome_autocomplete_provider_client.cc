// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/chrome/browser/autocomplete/chrome_autocomplete_provider_client.cc"

#include "brave/components/ai_chat/browser/ai_chat_tab_helper.h"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#endif  // BUILDFLAG(!IS_ANDROID)

#if BUILDFLAG(ENABLE_COMMANDER)
#include "brave/browser/ui/commander/commander_service_factory.h"
#include "brave/components/commander/browser/commander_frontend_delegate.h"
#endif  // BUILDFLAG(ENABLE_COMMANDER)

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/common/features.h"
#include "brave/components/ai_chat/common/pref_names.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_COMMANDER)
commander::CommanderFrontendDelegate*
ChromeAutocompleteProviderClient::GetCommanderDelegate() {
  return commander::CommanderServiceFactory::GetForBrowserContext(profile_);
}
#endif  // BUILDFLAG(ENABLE_COMMANDER)

#if BUILDFLAG(ENABLE_AI_CHAT)
void ChromeAutocompleteProviderClient::OpenLeo(const std::u16string& query) {
#if !BUILDFLAG(IS_ANDROID)
  DCHECK(base::FeatureList::IsEnabled(ai_chat::features::kAIChat));

  // Note that we're getting the last active browser. This is what upstream
  // does when they open the history journey from the omnibox. This seem to be
  // good enough because
  // * The time between the user typing and the journey opening is very small,
  // so active browser is unlikely to be changed
  // * Even if the active browser is changed, it'd be better to open the Leo in
  // the new active browser.
  Browser* browser = BrowserList::GetInstance()->GetLastActive();
  if (!browser) {
    return;
  }

  // Activate the panel.
  auto* sidebar_controller =
      static_cast<BraveBrowser*>(browser)->sidebar_controller();
  sidebar_controller->ActivatePanelItem(
      sidebar::SidebarItem::BuiltInItemType::kChatUI);

  // Send the query to the AIChat's backend.
  auto* chat_tab_helper = ai_chat::AIChatTabHelper::FromWebContents(
      browser->tab_strip_model()->GetActiveWebContents());
  DCHECK(chat_tab_helper);
  ai_chat::mojom::ConversationTurn turn = {
      ai_chat::mojom::CharacterType::HUMAN,
      ai_chat::mojom::ConversationTurnVisibility::VISIBLE,
      base::UTF16ToUTF8(query)};
  chat_tab_helper->MakeAPIRequestWithConversationHistoryUpdate(std::move(turn));
#endif
}

bool ChromeAutocompleteProviderClient::IsLeoProviderEnabled() {
#if BUILDFLAG(IS_ANDROID)
  return false;
#else
  return GetPrefs()->GetBoolean(
      ai_chat::prefs::kBraveChatAutocompleteProviderEnabled);
#endif
}
#endif  // BUILDFLAG(ENABLE_AI_CHAT)
