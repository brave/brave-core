/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_H_

#include <string>
#include <vector>

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"

namespace brave_shields {
class AdBlockService;
}  // namespace brave_shields

namespace webcompat_reporter {

class WebcompatReporterServiceDelegateImpl
    : public WebcompatReporterService::WebCompatServiceDelegate {
 public:
  WebcompatReporterServiceDelegateImpl(
      brave_shields::AdBlockService* adblock_service);
  WebcompatReporterServiceDelegateImpl(
      const WebcompatReporterServiceDelegateImpl&) = delete;
  WebcompatReporterServiceDelegateImpl& operator=(
      const WebcompatReporterServiceDelegateImpl&) = delete;
  ~WebcompatReporterServiceDelegateImpl() override;

  std::optional<std::vector<std::string>> GetAdblockFilterListNames()
      override const;
  std::string GetChannelName() override const;

 private:
  raw_ptr<brave_shields::AdBlockService> adblock_service_;
};

}  // namespace webcompat_reporter

#endif  // BRAVE_BROWSER_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_DELEGATE_H_
