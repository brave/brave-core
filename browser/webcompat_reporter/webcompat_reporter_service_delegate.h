/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_H_

#include <string>
#include <vector>

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service_delegate_base.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

namespace brave_shields {
class AdBlockService;
}  // namespace brave_shields

namespace webcompat_reporter {

class WebcompatReporterServiceDelegateImpl
    : public WebcompatReporterServiceDelegateBase {
 public:
  explicit WebcompatReporterServiceDelegateImpl(
      component_updater::ComponentUpdateService* component_update_service,
      brave_shields::AdBlockService* adblock_service,
      HostContentSettingsMap* host_content_settings_map,
      scoped_refptr<content_settings::CookieSettings> content_settings);
  WebcompatReporterServiceDelegateImpl(
      const WebcompatReporterServiceDelegateImpl&) = delete;
  WebcompatReporterServiceDelegateImpl& operator=(
      const WebcompatReporterServiceDelegateImpl&) = delete;
  ~WebcompatReporterServiceDelegateImpl() override;

  std::optional<std::vector<std::string>> GetAdblockFilterListNames()
      const override;
  std::optional<std::string> GetChannelName() const override;
  std::optional<std::string> GetCookiePolicy(
      const std::optional<std::string>& current_url) const override;
  std::optional<std::string> GetScriptBlockingFlag(
      const std::optional<std::string>& current_url) const override;

 private:
  const raw_ptr<brave_shields::AdBlockService> adblock_service_;
  const raw_ptr<HostContentSettingsMap> host_content_settings_map_;
  scoped_refptr<content_settings::CookieSettings> cookie_settings_;
};

}  // namespace webcompat_reporter

#endif  // BRAVE_BROWSER_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_H_
