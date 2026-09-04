// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/search_engines/template_url_service_factory_bridge.h"

#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#include "brave/ios/browser/search_engines/template_url_service_bridge_impl.h"
#include "components/search_engines/template_url_service.h"
#include "ios/chrome/browser/search_engines/model/template_url_service_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

@implementation TemplateURLServiceFactoryBridge

+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  TemplateURLService* service =
      ios::TemplateURLServiceFactory::GetForProfile(profile);
  if (!service) {
    return nil;
  }
  return [[TemplateURLServiceBridgeImpl alloc]
      initWithTemplateURLService:service
                           prefs:profile->GetPrefs()];
}

@end
