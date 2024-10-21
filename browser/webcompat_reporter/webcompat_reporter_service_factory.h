/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_FACTORY_H_

#include "brave/components/webcompat_reporter/common/webcompat_reporter.mojom.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace webcompat_reporter {

class WebcompatReporterService;

class WebcompatReporterServiceFactory : public ProfileKeyedServiceFactory {
 public:
  WebcompatReporterServiceFactory(const WebcompatReporterServiceFactory&) =
      delete;
  WebcompatReporterServiceFactory& operator=(
      const WebcompatReporterServiceFactory&) = delete;

  static mojo::PendingRemote<mojom::WebcompatReporterHandler>
  GetHandlerForContext(content::BrowserContext* context);
  static WebcompatReporterService* GetServiceForContext(
      content::BrowserContext* context);
  static WebcompatReporterServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<WebcompatReporterServiceFactory>;

  WebcompatReporterServiceFactory();
  ~WebcompatReporterServiceFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};

}  // namespace webcompat_reporter

#endif  // BRAVE_BROWSER_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_FACTORY_H_
