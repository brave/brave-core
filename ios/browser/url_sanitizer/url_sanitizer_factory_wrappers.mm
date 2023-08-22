// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/url_sanitizer/url_sanitizer_factory_wrappers.h"

#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"
#include "brave/ios/browser/api/url_sanitizer/url_sanitizer_service+private.h"
#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#include "brave/ios/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation URLSanitizerFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  // Create and start the local data file service and component installer
  brave::URLSanitizerService* urlSanitizer =
      brave::URLSanitizerServiceFactory::GetServiceForState(browserState);
  return [[URLSanitizerService alloc] initWithURLSanitizerService:urlSanitizer];
}
@end
