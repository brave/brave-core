/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_BASE_H_
#define BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_BASE_H_

#include <vector>

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace webcompat_reporter {

using WebCompatServiceDelegate = WebcompatReporterService::Delegate;
using ComponentInfo = WebcompatReporterService::Delegate::ComponentInfo;
class WebcompatReporterServiceDelegateBase : public WebCompatServiceDelegate {
 public:
  explicit WebcompatReporterServiceDelegateBase(
      component_updater::ComponentUpdateService* component_update_service);
  WebcompatReporterServiceDelegateBase(
      const WebcompatReporterServiceDelegateBase&) = delete;
  WebcompatReporterServiceDelegateBase& operator=(
      const WebcompatReporterServiceDelegateBase&) = delete;
  ~WebcompatReporterServiceDelegateBase() override;

  std::optional<std::vector<ComponentInfo>> GetComponentInfos() const override;

 private:
  const raw_ptr<component_updater::ComponentUpdateService>
      component_update_service_;
};

}  // namespace webcompat_reporter

#endif  // BRAVE_COMPONENTS_WEBCOMPAT_REPORTER_BROWSER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_BASE_H_
