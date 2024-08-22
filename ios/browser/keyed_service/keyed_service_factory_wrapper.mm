/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper.h"

#include "base/notreached.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"
#include "ios/web/public/thread/web_thread.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation KeyedServiceFactoryWrapper

+ (nullable id)getForPrivateMode:(bool)isPrivateBrowsing {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  auto* browserStateManager =
      GetApplicationContext()->GetChromeBrowserStateManager();
  auto* browserState =
      browserStateManager->GetLastUsedBrowserStateDeprecatedDoNotUse();
  if (isPrivateBrowsing) {
    browserState = browserState->GetOffTheRecordChromeBrowserState();
  }
  return [self serviceForBrowserState:browserState];
}

+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  NOTIMPLEMENTED();
  return nil;
}

@end
