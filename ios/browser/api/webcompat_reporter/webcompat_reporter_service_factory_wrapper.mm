/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/webcompat_reporter/webcompat_reporter_service_factory_wrapper.h"

#include "brave/components/webcompat_reporter/common/ios/webcompat_reporter.mojom.objc+private.h"
#include "brave/ios/browser/api/webcompat_reporter/webcompat_reporter_service_factory.h"
#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

@implementation WebcompatReporterServiceFactory
+ (nullable id)serviceForBrowserState:(ProfileIOS*)profile {
  auto handler =
      webcompat_reporter::WebcompatReporterServiceFactory::GetHandlerForContext(
          profile);
  if (!handler) {
    return nil;
  }
  return [[WebcompatReporterWebcompatReporterHandlerMojoImpl alloc]
      initWithWebcompatReporterHandler:std::move(handler)];
}
@end
