/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/content_settings/brave_widevine_content_setting_bubble_model.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ui/brave_browser_content_setting_bubble_model_delegate.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/plugins/plugin_utils.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/subresource_filter/chrome_subresource_filter_client.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
#include <string>

#include "base/bind.h"
#include "brave/browser/widevine/brave_widevine_bundle_manager.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#endif

namespace {
#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
void OnWidevineInstallDone(const std::string& error) {
  if (!error.empty()) {
    LOG(ERROR) << __func__ << ": " << error;
    return;
  }

  DVLOG(1) << __func__ << ": Widevine install success";
  if (Browser* browser = chrome::FindLastActive())
    browser->window()->UpdateToolbar(nullptr);
}
#endif
}

BraveWidevineContentSettingPluginBubbleModel::
BraveWidevineContentSettingPluginBubbleModel(
    ContentSettingBubbleModel::Delegate* delegate,
        content::WebContents* web_contents) :
        ContentSettingSimpleBubbleModel(delegate,
            web_contents,
            CONTENT_SETTINGS_TYPE_PLUGINS),
        brave_content_settings_delegate_(
            (BraveBrowserContentSettingBubbleModelDelegate*)delegate) {
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
  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  prefs->SetBoolean(kWidevineOptedIn, true);
  RegisterWidevineCdmComponent(g_brave_browser_process->component_updater());
  ChromeSubresourceFilterClient::FromWebContents(web_contents())
        ->OnReloadRequested();
#endif

#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  auto* manager = g_brave_browser_process->brave_widevine_bundle_manager();
  if (manager->needs_restart()) {
    manager->WillRestart();
    chrome::AttemptRelaunch();
    return;
  }
  // User can request install again because |kWidevineOptedIn| is set when
  // install is finished. In this case, just waiting previous install request.
  if (!manager->in_progress()) {
    manager->InstallWidevineBundle(base::BindOnce(&OnWidevineInstallDone),
                                   true);
  }
#endif
}

void BraveWidevineContentSettingPluginBubbleModel::SetTitle() {
  int message_id = IDS_NOT_INSTALLED_WIDEVINE_TITLE;
#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  auto* manager = g_brave_browser_process->brave_widevine_bundle_manager();
  message_id = manager->GetWidevineContentSettingsBubbleTitleText();
#endif
  set_title(l10n_util::GetStringUTF16(message_id));
}

void BraveWidevineContentSettingPluginBubbleModel::SetMessage() {
  set_message(l10n_util::GetStringUTF16(IDS_WIDEVINE_INSTALL_MESSAGE));
}

void BraveWidevineContentSettingPluginBubbleModel::SetCustomLink() {
  int message_id = IDS_INSTALL_AND_RUN_WIDEVINE;
#if BUILDFLAG(BUNDLE_WIDEVINE_CDM)
  auto* manager = g_brave_browser_process->brave_widevine_bundle_manager();
  message_id = manager->GetWidevineContentSettingsBubbleLinkText();
#endif
  set_custom_link(l10n_util::GetStringUTF16(message_id));
  set_custom_link_enabled(true);
}

void BraveWidevineContentSettingPluginBubbleModel::SetLearnMore() {
  set_show_learn_more(true);
}

void BraveWidevineContentSettingPluginBubbleModel::SetManageText() {
  set_manage_text_style(ContentSettingBubbleModel::ManageTextStyle::kNone);
}
