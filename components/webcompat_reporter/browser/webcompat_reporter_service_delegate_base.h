/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_BASE_H_
#define BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_BASE_H_

#include <string>
#include <vector>

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

class Profile;

namespace content {
    class BrowserContext;
}  // namespace content

namespace webcompat_reporter {

using WebCompatServiceDelegate = WebcompatReporterService::Delegate;
using ComponentInfo = WebcompatReporterService::Delegate::ComponentInfo;
class WebcompatReporterServiceDelegateBase : public WebCompatServiceDelegate {
 public:
  explicit WebcompatReporterServiceDelegateBase(
      component_updater::ComponentUpdateService* component_update_service, HostContentSettingsMap* host_content_settings_map,
    scoped_refptr<content_settings::CookieSettings> content_settings);
  WebcompatReporterServiceDelegateBase(
      const WebcompatReporterServiceDelegateBase&) = delete;
  WebcompatReporterServiceDelegateBase& operator=(
      const WebcompatReporterServiceDelegateBase&) = delete;
  ~WebcompatReporterServiceDelegateBase() override;

  std::optional<std::vector<ComponentInfo>> GetComponentInfos() const override;
  std::optional<std::string> GetCookiePolicy() const override;

 private:
  const raw_ptr<component_updater::ComponentUpdateService>
      component_update_service_;
  const base::raw_ptr<HostContentSettingsMap> host_content_settings_map_;
  scoped_refptr<content_settings::CookieSettings> cookie_settings_;

};

}  // namespace webcompat_reporter

#endif  // BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_BASE_H_
