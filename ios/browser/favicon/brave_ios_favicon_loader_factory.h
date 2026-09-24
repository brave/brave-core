/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_FAVICON_BRAVE_IOS_FAVICON_LOADER_FACTORY_H_
#define BRAVE_IOS_BROWSER_FAVICON_BRAVE_IOS_FAVICON_LOADER_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "ios/chrome/browser/shared/model/profile/profile_keyed_service_factory_ios.h"

class ProfileIOS;

namespace brave_favicon {
class BraveFaviconLoader;

// Singleton that owns all FaviconLoaders and associates them with
// ProfileIOS.
class BraveIOSFaviconLoaderFactory : public ProfileKeyedServiceFactoryIOS {
 public:
  static BraveFaviconLoader* GetForProfile(ProfileIOS* profile);
  static BraveFaviconLoader* GetForProfileIfExists(ProfileIOS* profile);
  static BraveIOSFaviconLoaderFactory* GetInstance();
  static TestingFactory GetDefaultFactory();

  BraveIOSFaviconLoaderFactory(const BraveIOSFaviconLoaderFactory&) = delete;
  BraveIOSFaviconLoaderFactory& operator=(const BraveIOSFaviconLoaderFactory&) =
      delete;

 private:
  friend class base::NoDestructor<BraveIOSFaviconLoaderFactory>;

  BraveIOSFaviconLoaderFactory();
  ~BraveIOSFaviconLoaderFactory() override;

  // ProfileKeyedServiceFactoryIOS implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};
}  // namespace brave_favicon

#endif  // BRAVE_IOS_BROWSER_FAVICON_BRAVE_IOS_FAVICON_LOADER_FACTORY_H_
