/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/webcompat_reporter/webcompat_reporter_service_delegate.h"

#include "brave/common/brave_channel_info.h"
#include "brave/components/brave_shields/content/browser/ad_block_service.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_service_manager.h"

namespace webcompat_reporter {

WebcompatReporterServiceDelegateImpl::WebcompatReporterServiceDelegateImpl(
    brave_shields::AdBlockService* adblock_service)
    : adblock_service_(adblock_service) {}

WebcompatReporterServiceDelegateImpl::~WebcompatReporterServiceDelegateImpl() =
    default;

std::optional<std::vector<std::string>>
WebcompatReporterServiceDelegateImpl::GetAdblockFilterListNames() const {
  if (!adblock_service_) {
    return std::nullopt;
  }

  std::vector<std::string> ad_block_list_names;
  brave_shields::AdBlockComponentServiceManager* service_manager =
      ad_block_service->component_service_manager();
  CHECK(service_manager);
  for (const brave_shields::FilterListCatalogEntry& entry :
       service_manager->GetFilterListCatalog()) {
    if (service_manager->IsFilterListEnabled(entry.uuid)) {
      ad_block_list_names.push_back(entry.title);
    }
  }

  return ad_block_list_names.empty() ? std::nullopt : ad_block_list_names;
}

std::string WebcompatReporterServiceDelegateImpl::GetChannelName() const {
  return brave::GetChannelName();
}
}  // namespace webcompat_reporter
