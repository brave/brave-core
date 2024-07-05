// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import <memory>

#import "base/memory/raw_ptr.h"
#import "base/notimplemented.h"
#import "base/threading/thread_restrictions.h"
#import "components/keyed_service/core/service_access_type.h"
#import "components/keyed_service/ios/browser_state_dependency_manager.h"
#import "components/password_manager/core/browser/password_store/password_store_interface.h"
#import "ios/web/public/browser_state.h"
#import "ios/web_view/internal/app/application_context.h"
#import "ios/web_view/internal/autofill/cwv_autofill_data_manager_internal.h"
#import "ios/web_view/internal/autofill/web_view_personal_data_manager_factory.h"
#import "ios/web_view/internal/browser_state_keyed_service_factories.h"
#import "ios/web_view/internal/cwv_preferences_internal.h"
#import "ios/web_view/internal/cwv_user_content_controller_internal.h"
#import "ios/web_view/internal/cwv_web_view_configuration_internal.h"
#import "ios/web_view/internal/cwv_web_view_internal.h"
#import "ios/web_view/internal/passwords/web_view_account_password_store_factory.h"
#import "ios/web_view/internal/web_view_global_state_util.h"

@interface CWVWebViewConfiguration () {
  // The BrowserState for this configuration.
  raw_ptr<web::BrowserState> _browserState;

  // Holds all CWVWebViews created with this class. Weak references.
  NSHashTable* _webViews;
}
@end

@implementation CWVWebViewConfiguration

+ (void)shutDown {
}

+ (instancetype)defaultConfiguration {
  NOTIMPLEMENTED();
  return nil;
}

+ (instancetype)incognitoConfiguration {
  NOTIMPLEMENTED();
  return nil;
}

+ (CWVWebViewConfiguration*)nonPersistentConfiguration {
  NOTIMPLEMENTED();
  return nil;
}

- (instancetype)initWithBrowserState:(web::BrowserState*)browserState {
  self = [super init];
  if (self) {
    _browserState = browserState;
    _webViews = [NSHashTable weakObjectsHashTable];
  }
  return self;
}

#pragma mark - Unused Services

- (CWVPreferences*)preferences {
  // This property is not nullable, so will crash anyways on the Swift side if
  // accessed.
  NOTIMPLEMENTED();
  return nil;
}

- (CWVUserContentController*)userContentController {
  // This property is not nullable, so will crash anyways on the Swift side if
  // accessed.
  NOTIMPLEMENTED();
  return nil;
}

- (CWVAutofillDataManager*)autofillDataManager {
  return nil;
}

- (CWVSyncController*)syncController {
  return nil;
}

- (CWVLeakCheckService*)leakCheckService {
  return nil;
}

- (CWVReuseCheckService*)reuseCheckService {
  return nil;
}

#pragma mark - Public Methods

- (BOOL)isPersistent {
  return !_browserState->IsOffTheRecord();
}

#pragma mark - Private Methods

- (web::BrowserState*)browserState {
  return _browserState;
}

- (void)registerWebView:(CWVWebView*)webView {
  [_webViews addObject:webView];
}

- (void)shutDown {
  for (CWVWebView* webView in _webViews) {
    [webView shutDown];
  }
}

@end
