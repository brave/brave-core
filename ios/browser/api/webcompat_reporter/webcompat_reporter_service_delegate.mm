// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/webcompat_reporter/webcompat_reporter_service_delegate.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/components/webcompat_reporter/browser/webcompat_reporter_utils.h"
#include "components/component_updater/component_updater_service.h"

namespace webcompat_reporter {

WebcompatReporterServiceDelegateImpl::WebcompatReporterServiceDelegateImpl(
    component_updater::ComponentUpdateService* cus)
    : WebcompatReporterServiceDelegateBase(cus) {}

WebcompatReporterServiceDelegateImpl::~WebcompatReporterServiceDelegateImpl() =
    default;

std::optional<std::vector<std::string>>
WebcompatReporterServiceDelegateImpl::GetAdblockFilterListNames() const {
  // we don't need to implement it for iOS, as we get it from the front-end part
  return std::nullopt;
}

std::optional<std::string>
WebcompatReporterServiceDelegateImpl::GetChannelName() const {
  // we don't need to implement it for iOS, as we get it from the front-end part
  return std::nullopt;
}

std::optional<std::string>
WebcompatReporterServiceDelegateImpl::GetCookiePolicy(
    const std::optional<std::string>& current_url) const {
  // we don't need to implement it for iOS, as we get it from the front-end part
  return std::nullopt;
}

std::optional<std::string>
WebcompatReporterServiceDelegateImpl::GetScriptBlockingFlag(
    const std::optional<std::string>& current_url) const {
  // we don't need to implement it for iOS, as we get it from the front-end part
  return std::nullopt;
}
}  // namespace webcompat_reporter
