// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/webcompat_reporter/common/webcompat_reporter.mojom.h"
#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"

class ProfileIOS;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace webcompat_reporter {

class WebcompatReporterServiceFactory : public ProfileKeyedServiceFactoryIOS {
 public:
  static mojo::PendingRemote<mojom::WebcompatReporterHandler>
  GetHandlerForContext(ProfileIOS* profile);
  static WebcompatReporterServiceFactory* GetInstance();

  WebcompatReporterServiceFactory(const WebcompatReporterServiceFactory&) =
      delete;
  WebcompatReporterServiceFactory& operator=(
      const WebcompatReporterServiceFactory&) = delete;

 private:
  friend class base::NoDestructor<WebcompatReporterServiceFactory>;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  WebcompatReporterServiceFactory();
  ~WebcompatReporterServiceFactory() override;

  // ProfileKeyedServiceFactoryIOS implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};

}  // namespace webcompat_reporter

#endif  // BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_FACTORY_H_
