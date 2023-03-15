/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/renderer/worker_content_settings_client.h"

#include "brave/components/brave_shields/common/brave_shield_utils.h"
#include "components/content_settings/renderer/content_settings_agent_impl.h"
#include "net/base/features.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

BraveFarblingLevel WorkerContentSettingsClient::GetBraveFarblingLevel() {
  ContentSetting setting = CONTENT_SETTING_DEFAULT;
  if (content_setting_rules_) {
    const GURL& primary_url = top_frame_origin_.GetURL();
    for (const auto& rule : content_setting_rules_->brave_shields_rules) {
      if (rule.primary_pattern.Matches(primary_url)) {
        setting = rule.GetContentSetting();
        break;
      }
    }
    if (setting == CONTENT_SETTING_BLOCK) {
      // Brave Shields is down
      setting = CONTENT_SETTING_ALLOW;
    } else {
      // Brave Shields is up, so check fingerprinting rules
      setting = brave_shields::GetBraveFPContentSettingFromRules(
          content_setting_rules_->fingerprinting_rules, primary_url);
    }
  }
  if (setting == CONTENT_SETTING_BLOCK) {
    return BraveFarblingLevel::MAXIMUM;
  } else if (setting == CONTENT_SETTING_ALLOW) {
    return BraveFarblingLevel::OFF;
  } else {
    return BraveFarblingLevel::BALANCED;
  }
}

blink::WebSecurityOrigin
WorkerContentSettingsClient::GetEphemeralStorageOriginSync() {
  if (!base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage))
    return {};

  if (is_unique_origin_)
    return {};

  // If first party ephemeral storage is enabled, we should always ask the
  // browser if a worker should use ephemeral storage or not.
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveFirstPartyEphemeralStorage) &&
      net::registry_controlled_domains::SameDomainOrHost(
          top_frame_origin_, document_origin_,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    return {};
  }

  EnsureContentSettingsManager();

  absl::optional<url::Origin> optional_ephemeral_storage_origin;
  content_settings_manager_->AllowEphemeralStorageAccess(
      render_frame_id_, document_origin_, site_for_cookies_, top_frame_origin_,
      &optional_ephemeral_storage_origin);
  // Don't cache the value intentionally as other WorkerContentSettingsClient
  // methods do.
  return blink::WebSecurityOrigin(
      optional_ephemeral_storage_origin
          ? blink::WebSecurityOrigin(*optional_ephemeral_storage_origin)
          : blink::WebSecurityOrigin());
}

bool WorkerContentSettingsClient::HasContentSettingsRules() const {
  return content_setting_rules_.get();
}

#include "src/chrome/renderer/worker_content_settings_client.cc"
