/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/tab_search/tab_search_ui.h"

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/grit/brave_generated_resources.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#define TabSearchUI TabSearchUI_ChromiumImpl
#include <chrome/browser/ui/webui/tab_search/tab_search_ui.cc>
#undef TabSearchUI

TabSearchUI::TabSearchUI(content::WebUI* web_ui)
    : TabSearchUI_ChromiumImpl(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  base::Value::Dict update_data;

#if BUILDFLAG(ENABLE_AI_CHAT)
  update_data.Set(
      "tabOrganizationEnabled",
      ai_chat::AIChatServiceFactory::GetForBrowserContext(profile) &&
          ai_chat::features::IsTabOrganizationEnabled() &&
          profile->GetPrefs()->GetBoolean(
              ai_chat::prefs::kBraveAIChatTabOrganizationEnabled));

  // Show FRE if user doesn't explicitly enable/disble the feature pref.
  update_data.Set("showTabOrganizationFRE",
                  !profile->GetPrefs()->HasPrefPath(
                      ai_chat::prefs::kBraveAIChatTabOrganizationEnabled));
#else
  update_data.Set("tabOrganizationEnabled", false);
  update_data.Set("showTabOrganizationFRE", false);
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

  update_data.Set("autoTabGroupsSelectorHeading",
                  l10n_util::GetStringUTF16(IDS_BRAVE_ORGANIZE_TAB_TITLE));

  update_data.Set("tabOrganizationTitle",
                  l10n_util::GetStringUTF16(IDS_BRAVE_ORGANIZE_TAB_TITLE));

  update_data.Set("tabOrganizationSubtitle",
                  l10n_util::GetStringUTF16(IDS_BRAVE_ORGANIZE_TAB_SUBTITLE));

  update_data.Set("tabOrganizationSuggestedTopicsSubtitle",
                  l10n_util::GetStringUTF16(
                      IDS_BRAVE_ORGANIZE_TAB_SUGGESTED_TOPICS_SUBTITLE));

  update_data.Set("tabOrganizationTopicInputPlaceholder",
                  l10n_util::GetStringUTF16(
                      IDS_BRAVE_ORGANIZE_TAB_TOPIC_INPUT_PLACEHOLDER));

  update_data.Set(
      "tabOrganizationSubmitButtonLabel",
      l10n_util::GetStringUTF16(IDS_BRAVE_ORGANIZE_TAB_SUBMIT_BUTTON_LABEL));
  update_data.Set(
      "tabOrganizationUndoButtonLabel",
      l10n_util::GetStringUTF16(IDS_BRAVE_ORGANIZE_TAB_UNDO_BUTTON_LABEL));

  update_data.Set(
      "tabOrganizationWindowCreatedMessage",
      l10n_util::GetStringUTF16(IDS_BRAVE_ORGANIZE_TAB_WINDOW_CREATED_MESSAGE));

  update_data.Set(
      "tabOrganizationSendTabDataMessage",
      l10n_util::GetStringUTF16(IDS_BRAVE_ORGANIZE_TAB_SEND_TAB_DATA_MESSAGE));

  update_data.Set(
      "tabOrganizationLearnMoreLabel",
      l10n_util::GetStringUTF16(IDS_BRAVE_ORGANIZE_TAB_LEARN_MORE_LABEL));

  update_data.Set("tabOrganizationGoPremiumButtonLabel",
                  l10n_util::GetStringUTF16(
                      IDS_BRAVE_ORGANIZE_TAB_GO_PREMIUM_BUTTON_LABEL));

  update_data.Set(
      "tabOrganizationDismissButtonLabel",
      l10n_util::GetStringUTF16(IDS_BRAVE_ORGANIZE_TAB_DISMISS_BUTTON_LABEL));

  update_data.Set(
      "tabOrganizationPrivacyDisclaimer",
      l10n_util::GetStringUTF16(IDS_BRAVE_ORGANIZE_TAB_PRIVACY_DISCLAIMER));

  update_data.Set(
      "tabOrganizationEnableButtonLabel",
      l10n_util::GetStringUTF16(IDS_BRAVE_ORGANIZE_TAB_ENABLE_BUTTON_LABEL));

  content::WebUIDataSource::Update(profile, chrome::kChromeUITabSearchHost,
                                   std::move(update_data));
}

TabSearchUI::~TabSearchUI() = default;
