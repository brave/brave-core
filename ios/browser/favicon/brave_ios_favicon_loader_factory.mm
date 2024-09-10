/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/favicon/brave_ios_favicon_loader_factory.h"

#include "base/no_destructor.h"
#import "brave/ios/browser/favicon/brave_ios_favicon_loader.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/favicon/model/favicon_service_factory.h"
#import "ios/chrome/browser/favicon/model/ios_chrome_large_icon_service_factory.h"
#include "ios/chrome/browser/shared/model/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

std::unique_ptr<KeyedService> BuildFaviconLoader(web::BrowserState* context) {
  ChromeBrowserState* browser_state =
      ChromeBrowserState::FromBrowserState(context);
  return std::make_unique<brave_favicon::BraveFaviconLoader>(
      ios::FaviconServiceFactory::GetForBrowserState(
          browser_state, ServiceAccessType::EXPLICIT_ACCESS));
}

}  // namespace

namespace brave_favicon {
BraveFaviconLoader* BraveIOSFaviconLoaderFactory::GetForBrowserState(
    ChromeBrowserState* browser_state) {
  return static_cast<BraveFaviconLoader*>(
      GetInstance()->GetServiceForBrowserState(browser_state, true));
}

BraveFaviconLoader* BraveIOSFaviconLoaderFactory::GetForBrowserStateIfExists(
    ChromeBrowserState* browser_state) {
  return static_cast<BraveFaviconLoader*>(
      GetInstance()->GetServiceForBrowserState(browser_state, false));
}

BraveIOSFaviconLoaderFactory* BraveIOSFaviconLoaderFactory::GetInstance() {
  static base::NoDestructor<BraveIOSFaviconLoaderFactory> instance;
  return instance.get();
}

// static
BrowserStateKeyedServiceFactory::TestingFactory
BraveIOSFaviconLoaderFactory::GetDefaultFactory() {
  return base::BindRepeating(&BuildFaviconLoader);
}

BraveIOSFaviconLoaderFactory::BraveIOSFaviconLoaderFactory()
    : BrowserStateKeyedServiceFactory(
          "BraveFaviconLoader",
          BrowserStateDependencyManager::GetInstance()) {
  DependsOn(ios::FaviconServiceFactory::GetInstance());
  DependsOn(IOSChromeLargeIconServiceFactory::GetInstance());
}

BraveIOSFaviconLoaderFactory::~BraveIOSFaviconLoaderFactory() {}

std::unique_ptr<KeyedService>
BraveIOSFaviconLoaderFactory::BuildServiceInstanceFor(
    web::BrowserState* context) const {
  return BuildFaviconLoader(context);
}

web::BrowserState* BraveIOSFaviconLoaderFactory::GetBrowserStateToUse(
    web::BrowserState* context) const {
  return GetBrowserStateRedirectedInIncognito(context);
}

bool BraveIOSFaviconLoaderFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
}  // namespace brave_favicon
