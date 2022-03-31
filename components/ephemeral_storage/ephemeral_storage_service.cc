/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"

#include <utility>

#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/ephemeral_storage/url_storage_checker.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/site_instance.h"
#include "net/base/features.h"

namespace ephemeral_storage {

EphemeralStorageService::EphemeralStorageService(
    content::BrowserContext* context,
    HostContentSettingsMap* host_content_settings_map)
    : context_(context), host_content_settings_map_(host_content_settings_map) {
  DCHECK(base::FeatureList::IsEnabled(
      net::features::kBraveFirstPartyEphemeralStorage));
  DCHECK(context_);
  DCHECK(host_content_settings_map_);
}

EphemeralStorageService::~EphemeralStorageService() = default;

void EphemeralStorageService::Shutdown() {
  for (const auto& pattern : patterns_to_cleanup_on_shutdown_) {
    host_content_settings_map_->SetContentSettingCustomScope(
        pattern, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::COOKIES, CONTENT_SETTING_DEFAULT);
  }
}

void EphemeralStorageService::CanEnable1PESForUrl(
    const GURL& url,
    base::OnceCallback<void(bool can_enable_1pes)> callback) const {
  if (!IsDefaultCookieSetting(url)) {
    std::move(callback).Run(false);
    return;
  }

  auto site_instance = content::SiteInstance::CreateForURL(context_, url);
  auto* storage_partition = context_->GetStoragePartition(site_instance.get());
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&UrlStorageChecker::StartCheck,
                     base::MakeRefCounted<UrlStorageChecker>(
                         storage_partition, url, std::move(callback))));
}

void EphemeralStorageService::Set1PESEnabledForUrl(const GURL& url,
                                                   bool enable) {
  auto pattern = ContentSettingsPattern::FromURLNoWildcard(url);
  if (enable) {
    patterns_to_cleanup_on_shutdown_.insert(pattern);
  } else {
    patterns_to_cleanup_on_shutdown_.erase(pattern);
  }
  host_content_settings_map_->SetContentSettingCustomScope(
      pattern, ContentSettingsPattern::Wildcard(), ContentSettingsType::COOKIES,
      enable ? CONTENT_SETTING_SESSION_ONLY : CONTENT_SETTING_DEFAULT);
}

bool EphemeralStorageService::Is1PESEnabledForUrl(const GURL& url) const {
  return host_content_settings_map_->GetContentSetting(
             url, url, ContentSettingsType::COOKIES) ==
         CONTENT_SETTING_SESSION_ONLY;
}

void EphemeralStorageService::Enable1PESForUrlIfPossible(
    const GURL& url,
    base::OnceCallback<void(bool)> on_ready) {
  CanEnable1PESForUrl(
      url,
      base::BindOnce(&EphemeralStorageService::OnCanEnable1PESForUrl,
                     weak_ptr_factory_.GetWeakPtr(), url, std::move(on_ready)));
}

void EphemeralStorageService::OnCanEnable1PESForUrl(
    const GURL& url,
    base::OnceCallback<void(bool)> on_ready,
    bool can_enable_1pes) {
  if (can_enable_1pes) {
    Set1PESEnabledForUrl(url, true);
  }
  std::move(on_ready).Run(can_enable_1pes);
}

bool EphemeralStorageService::IsDefaultCookieSetting(const GURL& url) const {
  ContentSettingsForOneType settings;
  host_content_settings_map_->GetSettingsForOneType(
      ContentSettingsType::COOKIES, &settings);

  for (const auto& setting : settings) {
    if (setting.primary_pattern.Matches(url) &&
        setting.secondary_pattern.Matches(url)) {
      return setting.source == "default";
    }
  }

  return true;
}

}  // namespace ephemeral_storage
