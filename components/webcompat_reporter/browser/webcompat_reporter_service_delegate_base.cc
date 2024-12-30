/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service_delegate_base.h"

#include <optional>

#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/webcompat_reporter/browser/webcompat_reporter_utils.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/component_updater/component_updater_service.h"

namespace webcompat_reporter {

WebcompatReporterServiceDelegateBase::WebcompatReporterServiceDelegateBase(
    component_updater::ComponentUpdateService* component_update_service,
    HostContentSettingsMap* host_content_settings_map,
    scoped_refptr<content_settings::CookieSettings> content_settings)
    : component_update_service_(component_update_service),
      host_content_settings_map_(host_content_settings_map),
      cookie_settings_(content_settings) {}

WebcompatReporterServiceDelegateBase::~WebcompatReporterServiceDelegateBase() =
    default;

std::optional<std::vector<ComponentInfo>>
WebcompatReporterServiceDelegateBase::GetComponentInfos() const {
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

std::optional<std::string>
WebcompatReporterServiceDelegateBase::GetCookiePolicy() const {
  DCHECK(host_content_settings_map_);
  DCHECK(cookie_settings_);
  if (!host_content_settings_map_ || !cookie_settings_) {
    return std::nullopt;
  }

  return brave_shields::ControlTypeToString(brave_shields::GetCookieControlType(
      host_content_settings_map_, cookie_settings_.get(), GURL()));
}

}  // namespace webcompat_reporter
