/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"

#include <utility>

#include "base/strings/strcat.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/ephemeral_storage/url_storage_checker.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
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

EphemeralStorageService::~EphemeralStorageService() {}

void EphemeralStorageService::CanEnable1PESForUrl(
    const GURL& url,
    base::OnceCallback<void(bool can_enable_1pes)> callback) const {
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
  host_content_settings_map_->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString(
          base::StrCat({"[*.]", url.host_piece(), ":*"})),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::COOKIES,
      enable ? CONTENT_SETTING_SESSION_ONLY : CONTENT_SETTING_DEFAULT);
}

bool EphemeralStorageService::Is1PESEnabledForUrl(const GURL& url) const {
  return host_content_settings_map_->GetContentSetting(
             url, url, ContentSettingsType::COOKIES) ==
         CONTENT_SETTING_SESSION_ONLY;
}

}  // namespace ephemeral_storage
