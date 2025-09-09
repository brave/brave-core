/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_SKUS_SKUS_SERVICE_FACTORY_H_
#define BRAVE_IOS_BROWSER_SKUS_SKUS_SERVICE_FACTORY_H_

#include <memory>

#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

class ProfileIOS;
class KeyedService;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace web {
class BrowserState;
}  // namespace web

namespace skus {

class SkusService;

class SkusServiceFactory : public ProfileKeyedServiceFactoryIOS {
 public:
  // Creates the service if it doesn't exist already for |profile|.
  static mojo::PendingRemote<mojom::SkusService> GetForProfile(
      ProfileIOS* profile);

  static SkusServiceFactory* GetInstance();

 private:
  friend base::NoDestructor<SkusServiceFactory>;

  SkusServiceFactory();
  ~SkusServiceFactory() override;

  // ProfileKeyedServiceFactoryIOS implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      web::BrowserState* context) const override;
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  SkusServiceFactory(const SkusServiceFactory&) = delete;
  SkusServiceFactory& operator=(const SkusServiceFactory&) = delete;
};

}  // namespace skus

#endif  // BRAVE_IOS_BROWSER_SKUS_SKUS_SERVICE_FACTORY_H_
