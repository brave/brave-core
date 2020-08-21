/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/autoplay/autoplay_permission_context.h"

#include <utility>

#include "brave/browser/brave_browser_process_impl.h"
#include "chrome/common/chrome_features.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/browser/page_specific_content_settings.h"
#include "components/permissions/permission_request_id.h"
#include "content/public/browser/browser_context.h"
#include "third_party/blink/public/mojom/feature_policy/feature_policy.mojom.h"

AutoplayPermissionContext::AutoplayPermissionContext(
    content::BrowserContext* browser_context)
    : permissions::PermissionContextBase(
          browser_context,
          ContentSettingsType::AUTOPLAY,
          blink::mojom::FeaturePolicyFeature::kAutoplay) {}

AutoplayPermissionContext::~AutoplayPermissionContext() = default;

void AutoplayPermissionContext::UpdateTabContext(
    const permissions::PermissionRequestID& id,
    const GURL& requesting_frame,
    bool allowed) {
  content_settings::PageSpecificContentSettings* content_settings =
      content_settings::PageSpecificContentSettings::GetForFrame(
          id.render_process_id(), id.render_frame_id());
  if (!content_settings)
    return;

  if (!allowed) {
    content_settings->OnContentBlocked(ContentSettingsType::AUTOPLAY);
  }
}

void AutoplayPermissionContext::NotifyPermissionSet(
    const permissions::PermissionRequestID& id,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    permissions::BrowserPermissionCallback callback,
    bool persist,
    ContentSetting content_setting) {
  permissions::PermissionContextBase::NotifyPermissionSet(
      id, requesting_origin, embedding_origin, std::move(callback), persist,
      content_setting);
  // Ask -> Allow
  if (persist && content_setting == CONTENT_SETTING_ALLOW) {
    content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(
        content::RenderFrameHost::FromID(id.render_process_id(),
                                         id.render_frame_id()));
     web_contents->GetController().Reload(content::ReloadType::NORMAL, false);
  }
}

bool AutoplayPermissionContext::IsRestrictedToSecureOrigins() const {
  return false;
}
