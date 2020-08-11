/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/content_settings/brave_content_setting_bubble_model.h"

#include <memory>
#include <string>

#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/plugins/plugin_utils.h"
#include "chrome/browser/subresource_filter/chrome_subresource_filter_client.h"
#include "chrome/browser/ui/content_settings/content_setting_bubble_model_delegate.h"
#include "chrome/grit/generated_resources.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "ui/base/l10n/l10n_util.h"

BraveContentSettingPluginBubbleModel::BraveContentSettingPluginBubbleModel(
    Delegate* delegate, content::WebContents* web_contents)
    : ContentSettingSimpleBubbleModel(delegate, web_contents,
        ContentSettingsType::PLUGINS) {
  content_settings::SettingInfo info;
  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(GetProfile());
  GURL url = web_contents->GetURL();
  std::unique_ptr<base::Value> value =
      map->GetWebsiteSetting(url, url, ContentSettingsType::PLUGINS,
          std::string(), &info);

  set_show_learn_more(true);

  // Do not show "Run flash this time" and "Manage" button in Tor profile.
  if (brave::IsTorProfile(GetProfile())) {
    set_manage_text_style(ContentSettingBubbleModel::ManageTextStyle::kNone);
    return;
  }

  // If the setting is not managed by the user, hide the "Manage" button.
  if (info.source != content_settings::SETTING_SOURCE_USER)
    set_manage_text_style(ContentSettingBubbleModel::ManageTextStyle::kNone);

  set_custom_link(l10n_util::GetStringUTF16(IDS_BLOCKED_PLUGINS_LOAD_ALL));
  set_custom_link_enabled(
      web_contents && content_settings::TabSpecificContentSettings::GetForFrame(
                          web_contents->GetMainFrame())
                          ->load_plugins_link_enabled());
}

void BraveContentSettingPluginBubbleModel::OnLearnMoreClicked() {
  if (delegate())
    delegate()->ShowLearnMorePage(ContentSettingsType::PLUGINS);
}

void BraveContentSettingPluginBubbleModel::OnCustomLinkClicked() {
  RunPluginsOnPage();
}

void BraveContentSettingPluginBubbleModel::RunPluginsOnPage() {
  // Web contents can be NULL if the tab was closed while the plugins
  // settings bubble is visible.
  if (!web_contents())
    return;

  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(GetProfile());
  map->SetContentSettingDefaultScope(
      web_contents()->GetURL(),
      GURL(),
      ContentSettingsType::PLUGINS,
      std::string(),
      CONTENT_SETTING_DETECT_IMPORTANT_CONTENT);

  ChromeSubresourceFilterClient::FromWebContents(web_contents())
        ->OnReloadRequested();
}
