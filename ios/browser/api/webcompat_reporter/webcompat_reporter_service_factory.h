// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "brave/components/webcompat_reporter/common/webcompat_reporter.mojom.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

class ChromeBrowserState;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace webcompat_reporter {

class WebcompatReporterServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static mojo::PendingRemote<mojom::WebcompatReporterHandler>
  GetHandlerForContext(ChromeBrowserState* browser_state);
  static WebcompatReporterServiceFactory* GetInstance();

  WebcompatReporterServiceFactory(const WebcompatReporterServiceFactory&) =
      delete;
  WebcompatReporterServiceFactory& operator=(
      const WebcompatReporterServiceFactory&) = delete;

 private:
  friend class base::NoDestructor<WebcompatReporterServiceFactory>;

  WebcompatReporterServiceFactory();
  ~WebcompatReporterServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
};

}  // namespace webcompat_reporter

#endif  // BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_FACTORY_H_
