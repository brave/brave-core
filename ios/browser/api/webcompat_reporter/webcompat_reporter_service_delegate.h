// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_H_
#define BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_H_

#include <string>
#include <vector>

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service_delegate_base.h"

namespace webcompat_reporter {

class WebcompatReporterServiceDelegateImpl
    : public WebcompatReporterServiceDelegateBase {
 public:
  explicit WebcompatReporterServiceDelegateImpl(
      component_updater::ComponentUpdateService* cus);
  WebcompatReporterServiceDelegateImpl(
      const WebcompatReporterServiceDelegateImpl&) = delete;
  WebcompatReporterServiceDelegateImpl& operator=(
      const WebcompatReporterServiceDelegateImpl&) = delete;
  ~WebcompatReporterServiceDelegateImpl() override;

  std::optional<std::vector<std::string>> GetAdblockFilterListNames()
      const override;
  std::optional<std::string> GetChannelName() const override;
  std::optional<std::string> GetCookiePolicy() const override;
};

}  // namespace webcompat_reporter

#endif  // BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_H_
