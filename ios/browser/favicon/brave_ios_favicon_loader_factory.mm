/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/favicon/brave_ios_favicon_loader_factory.h"

#import "brave/ios/browser/favicon/brave_ios_favicon_loader.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"
#include "ios/chrome/browser/favicon/model/favicon_service_factory.h"
#import "ios/chrome/browser/favicon/model/ios_chrome_large_icon_service_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

std::unique_ptr<KeyedService> BuildFaviconLoader(ProfileIOS* profile) {
  return std::make_unique<brave_favicon::BraveFaviconLoader>(
      ios::FaviconServiceFactory::GetForProfile(
          profile, ServiceAccessType::EXPLICIT_ACCESS));
}

}  // namespace

namespace brave_favicon {
// static
BraveIOSFaviconLoaderFactory::TestingFactory
BraveIOSFaviconLoaderFactory::GetDefaultFactory() {
  return base::BindRepeating(&BuildFaviconLoader);
}

BraveIOSFaviconLoaderFactory::BraveIOSFaviconLoaderFactory(PassKey key)
    : TypedProfileKeyedServiceFactoryIOS(
          std::move(key),
          "BraveFaviconLoader",
          ProfileSelection::kRedirectedInIncognito,
          ServiceCreation::kCreateLazily,
          TestingCreation::kNoServiceForTests) {
  DependsOn(ios::FaviconServiceFactory::GetInstance());
  DependsOn(IOSChromeLargeIconServiceFactory::GetInstance());
}

std::unique_ptr<KeyedService>
BraveIOSFaviconLoaderFactory::BuildServiceInstanceFor(
    ProfileIOS* profile) const {
  return BuildFaviconLoader(profile);
}
}  // namespace brave_favicon
