/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_tab_helpers.h"

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_tab_helper.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/content_settings/page_specific_content_settings_delegate.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "extensions/buildflags/buildflags.h"
#include "net/base/features.h"

namespace brave {

void AttachTabHelpers(content::WebContents* web_contents) {
  // If your TabHelper is related to privacy consider whether it belongs in this
  // method, so it can also be attached to background WebContents.
  AttachPrivacySensitiveTabHelpers(web_contents);
}

void AttachPrivacySensitiveTabHelpers(content::WebContents* web_contents) {
  content_settings::PageSpecificContentSettings::CreateForWebContents(
      web_contents,
      std::make_unique<PageSpecificContentSettingsDelegate>(web_contents));
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
