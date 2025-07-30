// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BRAVE_SHIELDS_SERVICE_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BRAVE_SHIELDS_SERVICE_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom.h"
#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"

class ProfileIOS;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace brave_shields {

class BraveShieldsUtilsServiceFactory : public ProfileKeyedServiceFactoryIOS {
 public:
  static mojo::PendingRemote<mojom::BraveShieldsUtilsService>
  GetHandlerForContext(ProfileIOS* profile);
  static BraveShieldsUtilsServiceFactory* GetInstance();

  BraveShieldsUtilsServiceFactory(const BraveShieldsUtilsServiceFactory&) =
      delete;
  BraveShieldsUtilsServiceFactory& operator=(
      const BraveShieldsUtilsServiceFactory&) = delete;

 private:
  friend class base::NoDestructor<BraveShieldsUtilsServiceFactory>;
  void RegisterBrowserStatePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  BraveShieldsUtilsServiceFactory();
  ~BraveShieldsUtilsServiceFactory() override;

  // ProfileKeyedServiceFactoryIOS implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
};

}  // namespace brave_shields

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BRAVE_SHIELDS_SERVICE_SERVICE_FACTORY_H_
