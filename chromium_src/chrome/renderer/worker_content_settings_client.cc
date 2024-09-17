/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/renderer/worker_content_settings_client.h"

#include <optional>

#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl.h"
#include "components/content_settings/renderer/content_settings_agent_impl.h"
#include "net/base/features.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

#define WorkerContentSettingsClient WorkerContentSettingsClient_ChromiumImpl

#include "src/chrome/renderer/worker_content_settings_client.cc"

#undef WorkerContentSettingsClient

WorkerContentSettingsClient_BraveImpl::WorkerContentSettingsClient_BraveImpl(
    content::RenderFrame* render_frame)
    : WorkerContentSettingsClient_ChromiumImpl(render_frame) {
  content_settings::ContentSettingsAgentImpl* agent =
      content_settings::ContentSettingsAgentImpl::Get(render_frame);

  if (const auto& shields_settings =
          static_cast<content_settings::BraveContentSettingsAgentImpl*>(agent)
              ->shields_settings()) {
    shields_settings_ = shields_settings->Clone();
  }
}

WorkerContentSettingsClient_BraveImpl::
    ~WorkerContentSettingsClient_BraveImpl() = default;

WorkerContentSettingsClient_BraveImpl::WorkerContentSettingsClient_BraveImpl(
    const WorkerContentSettingsClient_BraveImpl& other)
    : WorkerContentSettingsClient_ChromiumImpl(other) {
  if (other.shields_settings_) {
    shields_settings_ = other.shields_settings_->Clone();
  }
}

std::unique_ptr<blink::WebContentSettingsClient>
WorkerContentSettingsClient_BraveImpl::Clone() {
  return base::WrapUnique(new WorkerContentSettingsClient_BraveImpl(*this));
}

brave_shields::mojom::ShieldsSettingsPtr
WorkerContentSettingsClient_BraveImpl::GetBraveShieldsSettings(
    ContentSettingsType webcompat_settings_type) {
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
    if (setting != CONTENT_SETTING_ALLOW) {
      auto webcompat_setting =
          brave_shields::GetBraveWebcompatContentSettingFromRules(
              content_setting_rules_->webcompat_rules, primary_url,
              webcompat_settings_type);
      if (webcompat_setting == CONTENT_SETTING_ALLOW) {
        setting = CONTENT_SETTING_ALLOW;
      }
    }
  }

  brave_shields::mojom::FarblingLevel farbling_level =
      brave_shields::mojom::FarblingLevel::BALANCED;
  if (setting == CONTENT_SETTING_BLOCK) {
    farbling_level = brave_shields::mojom::FarblingLevel::MAXIMUM;
  } else if (setting == CONTENT_SETTING_ALLOW) {
    farbling_level = brave_shields::mojom::FarblingLevel::OFF;
  } else {
    farbling_level = brave_shields::mojom::FarblingLevel::BALANCED;
  }

  if (shields_settings_) {
    auto shields_settings = shields_settings_.Clone();
    shields_settings->farbling_level = farbling_level;
    return shields_settings;
  } else {
    DCHECK(!HasContentSettingsRules());
    return brave_shields::mojom::ShieldsSettings::New(
        farbling_level, std::vector<std::string>(), false);
  }
}

blink::WebSecurityOrigin
WorkerContentSettingsClient_BraveImpl::GetEphemeralStorageOriginSync() {
  if (!base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage)) {
    return {};
  }

  if (is_unique_origin_) {
    return {};
  }

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

  std::optional<url::Origin> optional_ephemeral_storage_origin;
  content_settings_manager_->AllowEphemeralStorageAccess(
      frame_token_, document_origin_, site_for_cookies_, top_frame_origin_,
      &optional_ephemeral_storage_origin);
  // Don't cache the value intentionally as other
  // WorkerContentSettingsClient_BraveImpl methods do.
  return blink::WebSecurityOrigin(
      optional_ephemeral_storage_origin
          ? blink::WebSecurityOrigin(*optional_ephemeral_storage_origin)
          : blink::WebSecurityOrigin());
}

bool WorkerContentSettingsClient_BraveImpl::HasContentSettingsRules() const {
  return content_setting_rules_.get();
}
