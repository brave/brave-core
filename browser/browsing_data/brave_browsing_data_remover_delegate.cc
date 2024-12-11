/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/brave_browsing_data_remover_delegate.h"

#include <memory>
#include <utility>

#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/brave_news/brave_news_controller_factory.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "build/build_config.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/buildflags.h"
#include "components/browsing_data/content/browsing_data_helper.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/browsing_data_remover.h"

BraveBrowsingDataRemoverDelegate::BraveBrowsingDataRemoverDelegate(
    content::BrowserContext* browser_context)
    : ChromeBrowsingDataRemoverDelegate(browser_context),
      profile_(Profile::FromBrowserContext(browser_context)) {}

BraveBrowsingDataRemoverDelegate::~BraveBrowsingDataRemoverDelegate() = default;

void BraveBrowsingDataRemoverDelegate::RemoveEmbedderData(
    const base::Time& delete_begin,
    const base::Time& delete_end,
    uint64_t remove_mask,
    content::BrowsingDataFilterBuilder* filter_builder,
    uint64_t origin_type_mask,
    base::OnceCallback<void(/*failed_data_types=*/uint64_t)> callback) {
  ChromeBrowsingDataRemoverDelegate::RemoveEmbedderData(
      delete_begin, delete_end, remove_mask, filter_builder, origin_type_mask,
      std::move(callback));

  // We do this because ChromeBrowsingDataRemoverDelegate::RemoveEmbedderData()
  // doesn't clear shields settings with non all time range.
  // The reason is upstream assumes that plugins type only as empty string
  // resource ids with plugins type. but we use plugins type to store our
  // shields settings with non-empty resource ids.
  if (remove_mask & chrome_browsing_data_remover::DATA_TYPE_CONTENT_SETTINGS) {
    ClearShieldsSettings(delete_begin, delete_end);
  }

  if (remove_mask & chrome_browsing_data_remover::DATA_TYPE_HISTORY) {
    // Brave News feed cache
    if (auto* brave_news_controller =
            brave_news::BraveNewsControllerFactory::GetForBrowserContext(
                profile_)) {
      brave_news_controller->ClearHistory();
    }
    // AI Chat history but only associated content, not neccessary if we
    // are also deleting entire AI Chat history.
    if (!(remove_mask &
          chrome_browsing_data_remover::DATA_TYPE_BRAVE_LEO_HISTORY)) {
      ai_chat::AIChatService* ai_chat_service =
          ai_chat::AIChatServiceFactory::GetForBrowserContext(profile_);
      if (ai_chat_service) {
        ai_chat_service->DeleteAssociatedWebContent(delete_begin, delete_end);
      }
    }
  }

  if (remove_mask & chrome_browsing_data_remover::DATA_TYPE_BRAVE_LEO_HISTORY) {
    ai_chat::AIChatService* ai_chat_service =
        ai_chat::AIChatServiceFactory::GetForBrowserContext(profile_);
    if (ai_chat_service) {
      ai_chat_service->DeleteConversations(delete_begin, delete_end);
    }
  }
}

void BraveBrowsingDataRemoverDelegate::ClearShieldsSettings(
    base::Time begin_time,
    base::Time end_time) {
  if (begin_time.is_null() && (end_time.is_null() || end_time.is_max())) {
    // For all time range, we don't need to do anything here because
    // ChromeBrowsingDataRemoverDelegate::RemoveEmbedderData() nukes whole
    // plugins type.
    return;
  }

  auto* map = HostContentSettingsMapFactory::GetForProfile(profile_);
  auto* provider =
      static_cast<content_settings::BravePrefProvider*>(map->GetPrefProvider());
  for (const auto& content_type :
       content_settings::GetShieldsContentSettingsTypes()) {
    ContentSettingsForOneType settings =
        map->GetSettingsForOneType(content_type);
    for (const ContentSettingPatternSource& setting : settings) {
      base::Time last_modified = setting.metadata.last_modified();
      if (last_modified >= begin_time &&
          (last_modified < end_time || end_time.is_null())) {
        provider->SetWebsiteSetting(setting.primary_pattern,
                                    setting.secondary_pattern, content_type,
                                    base::Value(), {});
      }
    }
  }
}
