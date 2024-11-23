/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/tab_search/tab_search_ui.h"

#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/grit/brave_generated_resources.h"

#define TabSearchUI TabSearchUI_ChromiumImpl
#include "src/chrome/browser/ui/webui/tab_search/tab_search_ui.cc"
#undef TabSearchUI

TabSearchUI::TabSearchUI(content::WebUI* web_ui)
    : TabSearchUI_ChromiumImpl(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  base::Value::Dict update_data;

  update_data.Set("tabOrganizationEnabled",
                  ai_chat::IsAIChatEnabled(profile->GetPrefs()) &&
                      ai_chat::features::IsTabFocusEnabled() &&
                      profile->GetPrefs()->GetBoolean(
                          ai_chat::prefs::kBraveAIChatTabFocusEnabled));

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
      "tabOrganizationFeatureExplanation",
      l10n_util::GetStringUTF16(IDS_BRAVE_ORGANIZE_TAB_FEATURE_EXPLANATION));

  update_data.Set(
      "tabOrganizationLearnMoreLabel",
      l10n_util::GetStringUTF16(IDS_BRAVE_ORGANIZE_TAB_LEARN_MORE_LABEL));

  content::WebUIDataSource::Update(profile, chrome::kChromeUITabSearchHost,
                                   std::move(update_data));
}

TabSearchUI::~TabSearchUI() = default;
