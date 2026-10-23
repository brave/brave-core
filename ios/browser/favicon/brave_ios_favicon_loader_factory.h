/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_FAVICON_BRAVE_IOS_FAVICON_LOADER_FACTORY_H_
#define BRAVE_IOS_BROWSER_FAVICON_BRAVE_IOS_FAVICON_LOADER_FACTORY_H_

#include <memory>

#include "brave/ios/browser/favicon/brave_ios_favicon_loader.h"
#include "ios/chrome/browser/shared/model/profile/typed_profile_keyed_service_factory_ios.h"

class ProfileIOS;

namespace brave_favicon {

// Singleton that owns all FaviconLoaders and associates them with
// ProfileIOS.
class BraveIOSFaviconLoaderFactory
    : public TypedProfileKeyedServiceFactoryIOS<BraveIOSFaviconLoaderFactory,
                                                BraveFaviconLoader> {
 public:
  static TestingFactory GetDefaultFactory();

  explicit BraveIOSFaviconLoaderFactory(PassKey key);

  BraveIOSFaviconLoaderFactory(const BraveIOSFaviconLoaderFactory&) = delete;
  BraveIOSFaviconLoaderFactory& operator=(const BraveIOSFaviconLoaderFactory&) =
      delete;

 private:
  // TypedProfileKeyedServiceFactoryIOS implementation.
  std::unique_ptr<KeyedService> BuildServiceInstanceFor(
      ProfileIOS* profile) const override;
};
}  // namespace brave_favicon

#endif  // BRAVE_IOS_BROWSER_FAVICON_BRAVE_IOS_FAVICON_LOADER_FACTORY_H_
