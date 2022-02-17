/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/content_settings/brave_autoplay_content_setting_bubble_model.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/brave_browser_content_setting_bubble_model_delegate.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/url_formatter/elide_url.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

using content_settings::SettingInfo;
using content_settings::SettingSource;
using content_settings::SETTING_SOURCE_USER;
using content_settings::SETTING_SOURCE_NONE;

BraveAutoplayContentSettingBubbleModel::BraveAutoplayContentSettingBubbleModel(
    Delegate* delegate,
    WebContents* web_contents)
    : ContentSettingSimpleBubbleModel(delegate,
                                      web_contents,
                                      ContentSettingsType::AUTOPLAY),
      block_setting_(CONTENT_SETTING_BLOCK) {
  SetTitle();
  SetRadioGroup();
}

BraveAutoplayContentSettingBubbleModel::
~BraveAutoplayContentSettingBubbleModel() {}

void BraveAutoplayContentSettingBubbleModel::CommitChanges() {
  if (settings_changed()) {
    ContentSetting setting = selected_item() == kAllowButtonIndex
                                 ? CONTENT_SETTING_ALLOW
                                 : block_setting_;
    SetNarrowestContentSetting(setting);
    web_contents()->GetController().Reload(content::ReloadType::NORMAL, true);
  }
}

bool BraveAutoplayContentSettingBubbleModel::settings_changed() const {
  return selected_item() != bubble_content().radio_group.default_item;
}

void BraveAutoplayContentSettingBubbleModel::SetTitle() {
  set_title(l10n_util::GetStringUTF16(IDS_BLOCKED_AUTOPLAY_TITLE));
}

void BraveAutoplayContentSettingBubbleModel::SetRadioGroup() {
  GURL url = web_contents()->GetURL();
  std::u16string display_host = url_formatter::FormatUrlForSecurityDisplay(url);
  if (display_host.empty())
    display_host = base::ASCIIToUTF16(url.spec());

  content_settings::PageSpecificContentSettings* content_settings =
      content_settings::PageSpecificContentSettings::GetForFrame(
          web_contents()->GetMainFrame());
  bool allowed = !content_settings->IsContentBlocked(content_type());
  DCHECK(!allowed || content_settings->IsContentAllowed(content_type()));

  RadioGroup radio_group;
  radio_group.url = url;
  std::u16string radio_allow_label =
      l10n_util::GetStringFUTF16(IDS_BLOCKED_AUTOPLAY_UNBLOCK, display_host);
  std::u16string radio_block_label =
      l10n_util::GetStringUTF16(IDS_BLOCKED_AUTOPLAY_NO_ACTION);
  radio_group.radio_items.push_back(radio_allow_label);
  radio_group.radio_items.push_back(radio_block_label);

  ContentSetting setting;
  SettingSource setting_source = SETTING_SOURCE_NONE;

  SettingInfo info;
  HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(GetProfile());
  base::Value value = map->GetWebsiteSetting(url, url, content_type(), &info);
  setting = content_settings::ValueToContentSetting(value);
  setting_source = info.source;

  if (setting == CONTENT_SETTING_ALLOW) {
    radio_group.default_item = kAllowButtonIndex;
    // |block_setting_| is already set to |CONTENT_SETTING_BLOCK|.
  } else {
    radio_group.default_item = 1;
    block_setting_ = setting;
  }

  // Prevent creation of content settings for illegal urls like about:blank
  bool is_valid = map->CanSetNarrowestContentSetting(url, url, content_type());
  radio_group.user_managed = is_valid && setting_source == SETTING_SOURCE_USER;
  set_radio_group(radio_group);
}

void BraveAutoplayContentSettingBubbleModel::SetNarrowestContentSetting(
    ContentSetting setting) {
  if (!GetProfile())
    return;

  auto* map = HostContentSettingsMapFactory::GetForProfile(GetProfile());
  map->SetNarrowestContentSetting(bubble_content().radio_group.url,
                                  bubble_content().radio_group.url,
                                  content_type(), setting);
}
