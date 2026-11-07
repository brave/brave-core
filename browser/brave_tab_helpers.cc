/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_tab_helpers.h"

#include <memory>

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/content_settings/page_specific_content_settings_delegate.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "content/public/browser/web_contents.h"

namespace brave {

void AttachPrivacySensitiveTabHelpers(content::WebContents* web_contents) {
  // Profiles without content settings, such as the shared Top Chrome WebUI
  // profile, host no web pages for these helpers to protect.
  if (!HostContentSettingsMapFactory::GetForProfile(
          web_contents->GetBrowserContext())) {
    return;
  }
  if (!content_settings::PageSpecificContentSettings::GetDelegateForWebContents(
          web_contents)) {
    content_settings::PageSpecificContentSettings::CreateForWebContents(
        web_contents,
        std::make_unique<PageSpecificContentSettingsDelegate>(web_contents));
  }
  brave_shields::BraveShieldsWebContentsObserver::CreateForWebContents(
      web_contents);
  if (auto* ephemeral_storage_service =
          EphemeralStorageServiceFactory::GetForContext(
              web_contents->GetBrowserContext())) {
    ephemeral_storage::EphemeralStorageTabHelper::CreateForWebContents(
        web_contents, ephemeral_storage_service);
  }
}

}  // namespace brave
