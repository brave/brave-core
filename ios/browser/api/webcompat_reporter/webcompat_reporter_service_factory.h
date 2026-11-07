// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"
#include "brave/components/webcompat_reporter/common/webcompat_reporter.mojom.h"
#include "ios/chrome/browser/shared/model/profile/typed_profile_keyed_service_factory_ios.h"

class ProfileIOS;

namespace webcompat_reporter {

class WebcompatReporterServiceFactory
    : public TypedProfileKeyedServiceFactoryIOS<WebcompatReporterServiceFactory,
                                                WebcompatReporterService> {
 public:
  static mojo::PendingRemote<mojom::WebcompatReporterHandler>
  GetHandlerForContext(ProfileIOS* profile);
  explicit WebcompatReporterServiceFactory(PassKey key);

  WebcompatReporterServiceFactory(const WebcompatReporterServiceFactory&) =
      delete;
  WebcompatReporterServiceFactory& operator=(
      const WebcompatReporterServiceFactory&) = delete;

 private:
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  // TypedProfileKeyedServiceFactoryIOS implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};

}  // namespace webcompat_reporter

#endif  // BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_SERVICE_FACTORY_H_
