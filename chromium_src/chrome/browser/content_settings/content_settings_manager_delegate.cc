/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom-shared.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "components/user_prefs/user_prefs.h"

#include <chrome/browser/content_settings/content_settings_manager_delegate.cc>

namespace {

brave_shields::mojom::ShieldsSettingsPtr GetBraveShieldsSettingsOnUI(
    const content::GlobalRenderFrameHostToken& frame_token) {
  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromFrameToken(frame_token);
  if (!rfh) {
    return brave_shields::mojom::ShieldsSettings::New();
  }
  content::RenderFrameHost* top_frame_rfh = rfh->GetOutermostMainFrame();
  if (!top_frame_rfh) {
    return brave_shields::mojom::ShieldsSettings::New();
  }

  const GURL& top_frame_url = top_frame_rfh->GetLastCommittedURL();

  content::BrowserContext* browser_context = rfh->GetBrowserContext();
  const brave_shields::mojom::FarblingLevel farbling_level =
      brave_shields::GetFarblingLevel(
          HostContentSettingsMapFactory::GetForProfile(browser_context),
          top_frame_url);
  const base::Token farbling_token =
      farbling_level != brave_shields::mojom::FarblingLevel::OFF
          ? brave_shields::GetFarblingToken(
                HostContentSettingsMapFactory::GetForProfile(browser_context),
                top_frame_url)
          : base::Token();
  const auto scripts_blocked_by_extension =
      brave_shields::GetScriptBlockedByExtensionStatus(
          HostContentSettingsMapFactory::GetForProfile(browser_context),
          top_frame_url);

  PrefService* pref_service = user_prefs::UserPrefs::Get(browser_context);

  return brave_shields::mojom::ShieldsSettings::New(
      farbling_level, farbling_token, std::vector<std::string>(),
      brave_shields::IsReduceLanguageEnabledForProfile(pref_service),
      scripts_blocked_by_extension);
}

}  // namespace

void ContentSettingsManagerDelegate::GetBraveShieldsSettings(
    const content::GlobalRenderFrameHostToken& frame_token,
    content_settings::mojom::ContentSettingsManager::
        GetBraveShieldsSettingsCallback callback) {
  content::GetUIThreadTaskRunner({})->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&GetBraveShieldsSettingsOnUI, frame_token),
      std::move(callback));
}
