// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/webcompat_reporter/webcompat_reporter_service_delegate.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/components/webcompat_reporter/common/webcompat_reporter_utils.h"
#include "components/component_updater/component_updater_service.h"

namespace webcompat_reporter {

WebcompatReporterServiceDelegateImpl::WebcompatReporterServiceDelegateImpl(
    component_updater::ComponentUpdateService* cus)
    : component_update_service_(cus) {}

WebcompatReporterServiceDelegateImpl::~WebcompatReporterServiceDelegateImpl() =
    default;

std::optional<std::vector<std::string>>
WebcompatReporterServiceDelegateImpl::GetAdblockFilterListNames() const {
  return std::nullopt;
}

std::optional<std::string>
WebcompatReporterServiceDelegateImpl::GetChannelName() const {
  return std::nullopt;
}

std::optional<std::vector<ComponentInfo>>
WebcompatReporterServiceDelegateImpl::GetComponentInfos() const {
  if (!component_update_service_) {
    return std::nullopt;
  }

  std::vector<ComponentInfo> result;
  auto components(component_update_service_->GetComponents());
  for (const auto& ci : components) {
    if (!NeedsToGetComponentInfo(ci.id)) {
      continue;
    }

    result.emplace_back(ComponentInfo{ci.id, base::UTF16ToUTF8(ci.name),
                                      ci.version.GetString()});
  }

  if (result.empty()) {
    return std::nullopt;
  }

  return result;
}

}  // namespace webcompat_reporter
