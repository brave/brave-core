/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/components/keyed_service/keyed_service_factory_wrapper.h"

#include "base/notreached.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/browser_state_otr_helper.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"

@implementation KeyedServiceFactoryWrapper

+ (nullable id)getForPrivateMode:(bool)isPrivateBrowsing {
  auto* browserStateManager =
      GetApplicationContext()->GetChromeBrowserStateManager();
  auto* browserState = browserStateManager->GetLastUsedBrowserState();
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
