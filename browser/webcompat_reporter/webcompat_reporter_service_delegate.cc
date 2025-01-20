/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/webcompat_reporter/webcompat_reporter_service_delegate.h"

#include <optional>

#include "brave/common/brave_channel_info.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"
#include "brave/components/brave_shields/core/browser/filter_list_catalog_entry.h"
#include "brave/components/webcompat_reporter/browser/webcompat_reporter_utils.h"
#include "components/component_updater/component_updater_service.h"

namespace webcompat_reporter {

WebcompatReporterServiceDelegateImpl::WebcompatReporterServiceDelegateImpl(
    component_updater::ComponentUpdateService* component_update_service,
    brave_shields::AdBlockService* adblock_service,
    HostContentSettingsMap* host_content_settings_map,
    scoped_refptr<content_settings::CookieSettings> content_settings)
    : WebcompatReporterServiceDelegateBase(component_update_service),
      adblock_service_(adblock_service),
      host_content_settings_map_(host_content_settings_map),
      cookie_settings_(content_settings) {}

WebcompatReporterServiceDelegateImpl::~WebcompatReporterServiceDelegateImpl() =
    default;

std::optional<std::vector<std::string>>
WebcompatReporterServiceDelegateImpl::GetAdblockFilterListNames() const {
  if (!adblock_service_) {
    return std::nullopt;
  }

  std::vector<std::string> ad_block_list_names;
  brave_shields::AdBlockComponentServiceManager* service_manager =
      adblock_service_->component_service_manager();
  CHECK(service_manager);
  for (const brave_shields::FilterListCatalogEntry& entry :
       service_manager->GetFilterListCatalog()) {
    if (service_manager->IsFilterListEnabled(entry.uuid)) {
      ad_block_list_names.push_back(entry.title);
    }
  }

  if (ad_block_list_names.empty()) {
    return std::nullopt;
  }

  return ad_block_list_names;
}

std::optional<std::string>
WebcompatReporterServiceDelegateImpl::GetChannelName() const {
  return brave::GetChannelName();
}

std::optional<std::string>
WebcompatReporterServiceDelegateImpl::GetCookiePolicy(
    const std::optional<std::string>& current_url) const {
  DCHECK(host_content_settings_map_);
  DCHECK(cookie_settings_);
  if (!host_content_settings_map_ || !cookie_settings_ || !current_url) {
    return std::nullopt;
  }

  return brave_shields::ControlTypeToString(brave_shields::GetCookieControlType(
      host_content_settings_map_, cookie_settings_.get(),
      GURL(current_url.value())));
}

std::optional<std::string>
WebcompatReporterServiceDelegateImpl::GetScriptBlockingFlag(
    const std::optional<std::string>& current_url) const {
  DCHECK(host_content_settings_map_);
  DCHECK(cookie_settings_);
  if (!host_content_settings_map_ || !cookie_settings_ || !current_url) {
    return std::nullopt;
  }

  return BoolToString(
      brave_shields::GetNoScriptControlType(host_content_settings_map_,
                                            GURL(current_url.value())) ==
      brave_shields::ControlType::BLOCK);
}

}  // namespace webcompat_reporter
