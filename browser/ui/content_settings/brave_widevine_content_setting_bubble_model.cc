/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/content_settings/brave_widevine_content_setting_bubble_model.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ui/brave_browser_content_setting_bubble_model_delegate.h"
#include "brave/browser/widevine/widevine_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "ui/base/l10n/l10n_util.h"

BraveWidevineContentSettingPluginBubbleModel::
BraveWidevineContentSettingPluginBubbleModel(
    ContentSettingBubbleModel::Delegate* delegate,
        content::WebContents* web_contents) :
        ContentSettingSimpleBubbleModel(delegate,
            web_contents,
            CONTENT_SETTINGS_TYPE_PLUGINS),
        brave_content_settings_delegate_(
            static_cast<BraveBrowserContentSettingBubbleModelDelegate*>(
                delegate)) {
  SetTitle();
  SetLearnMore();
  SetMessage();
  SetCustomLink();
  SetManageText();
}

void BraveWidevineContentSettingPluginBubbleModel::OnLearnMoreClicked() {
  if (brave_content_settings_delegate()) {
    brave_content_settings_delegate()->ShowWidevineLearnMorePage();
  }
}

void BraveWidevineContentSettingPluginBubbleModel::OnCustomLinkClicked() {
  RunPluginsOnPage();
}

void BraveWidevineContentSettingPluginBubbleModel::RunPluginsOnPage() {
  // Web contents can be NULL if the tab was closed while the settings
  // bubble is visible.
  if (!web_contents())
    return;

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
  EnableWidevineCdmComponent(web_contents());
#endif

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  InstallBundleOrRestartBrowser();
#endif
}

void BraveWidevineContentSettingPluginBubbleModel::SetTitle() {
  set_title(l10n_util::GetStringUTF16(GetWidevineTitleTextResourceId()));
}

void BraveWidevineContentSettingPluginBubbleModel::SetMessage() {
  set_message(l10n_util::GetStringUTF16(IDS_WIDEVINE_INSTALL_MESSAGE));
}

void BraveWidevineContentSettingPluginBubbleModel::SetCustomLink() {
  set_custom_link(l10n_util::GetStringUTF16(
      GetWidevineLinkTextForContentSettingsBubbleResourceId()));
  set_custom_link_enabled(true);
}

void BraveWidevineContentSettingPluginBubbleModel::SetLearnMore() {
  set_show_learn_more(true);
}

void BraveWidevineContentSettingPluginBubbleModel::SetManageText() {
  set_manage_text_style(ContentSettingBubbleModel::ManageTextStyle::kNone);
}
