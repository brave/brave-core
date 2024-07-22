// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <memory>

#import "base/threading/thread_restrictions.h"
#import "brave/ios/web_view/internal/cwv_web_view_configuration_internal.h"
#import "components/keyed_service/core/service_access_type.h"
#import "ios/chrome/browser/shared/model/application_context/application_context.h"
#import "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/shared/model/browser_state/chrome_browser_state_manager.h"
#import "ios/web_view/internal/cwv_preferences_internal.h"
#import "ios/web_view/internal/cwv_user_content_controller_internal.h"
#import "ios/web_view/internal/cwv_web_view_internal.h"

namespace {
CWVWebViewConfiguration* gDefaultConfiguration = nil;
CWVWebViewConfiguration* gIncognitoConfiguration = nil;
NSHashTable<CWVWebViewConfiguration*>* gNonPersistentConfigurations = nil;
}  // namespace

@interface CWVWebViewConfiguration () {
  // The BrowserState for this configuration.
  ChromeBrowserState* _browserState;

  // Holds all CWVWebViews created with this class. Weak references.
  NSHashTable* _webViews;
}

@end

@implementation CWVWebViewConfiguration

@synthesize autofillDataManager = _autofillDataManager;
@synthesize leakCheckService = _leakCheckService;
@synthesize reuseCheckService = _reuseCheckService;
@synthesize preferences = _preferences;
@synthesize syncController = _syncController;
@synthesize userContentController = _userContentController;

+ (void)shutDown {
  // Non-persistent configurations should be shut down first because its browser
  // state holds on to the default configuration's browser state. This ensures
  // the non-persistent configurations will not reference a dangling pointer.
  for (CWVWebViewConfiguration* nonPersistentConfiguration in
           gNonPersistentConfigurations) {
    [nonPersistentConfiguration shutDown];
  }
  [gDefaultConfiguration shutDown];
}

+ (instancetype)defaultConfiguration {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    ios::ChromeBrowserStateManager* browserStateManager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    ChromeBrowserState* browserState =
        browserStateManager->GetLastUsedBrowserStateDeprecatedDoNotUse();
    gDefaultConfiguration =
        [[CWVWebViewConfiguration alloc] initWithBrowserState:browserState];
  });
  return gDefaultConfiguration;
}

+ (instancetype)incognitoConfiguration {
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    gIncognitoConfiguration = [self nonPersistentConfiguration];
  });
  return gIncognitoConfiguration;
}

+ (CWVWebViewConfiguration*)nonPersistentConfiguration {
  ios::ChromeBrowserStateManager* browserStateManager =
      GetApplicationContext()->GetChromeBrowserStateManager();
  ChromeBrowserState* browserState =
      browserStateManager->GetLastUsedBrowserStateDeprecatedDoNotUse()
          ->GetOffTheRecordChromeBrowserState();
  CWVWebViewConfiguration* nonPersistentConfiguration =
      [[CWVWebViewConfiguration alloc] initWithBrowserState:browserState];

  // Save a weak pointer to nonpersistent configurations so they may be shut
  // down later.
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    gNonPersistentConfigurations = [NSHashTable weakObjectsHashTable];
  });
  [gNonPersistentConfigurations addObject:nonPersistentConfiguration];

  return nonPersistentConfiguration;
}

- (instancetype)initWithBrowserState:(ChromeBrowserState*)browserState {
  self = [super init];
  if (self) {
    _browserState = browserState;

    _preferences =
        [[CWVPreferences alloc] initWithPrefService:_browserState->GetPrefs()];

    _userContentController =
        [[CWVUserContentController alloc] initWithConfiguration:self];

    _webViews = [NSHashTable weakObjectsHashTable];
  }
  return self;
}

#pragma mark - Autofill

- (CWVAutofillDataManager*)autofillDataManager {
  // if (!_autofillDataManager && self.persistent) {
  //   autofill::PersonalDataManager* personalDataManager =
  //       ios_web_view::WebViewPersonalDataManagerFactory::GetForBrowserState(
  //           self.browserState);
  //   scoped_refptr<password_manager::PasswordStoreInterface> passwordStore =
  //       ios_web_view::WebViewAccountPasswordStoreFactory::GetForBrowserState(
  //           self.browserState, ServiceAccessType::EXPLICIT_ACCESS);
  //   _autofillDataManager = [[CWVAutofillDataManager alloc]
  //       initWithPersonalDataManager:personalDataManager
  //                     passwordStore:passwordStore.get()];
  // }
  return _autofillDataManager;
}

#pragma mark - Sync

- (CWVSyncController*)syncController {
  // if (!_syncController && self.persistent) {
  //   syncer::SyncService* syncService =
  //       ios_web_view::WebViewSyncServiceFactory::GetForBrowserState(
  //           self.browserState);
  //   signin::IdentityManager* identityManager =
  //       ios_web_view::WebViewIdentityManagerFactory::GetForBrowserState(
  //           self.browserState);
  //   _syncController = [[CWVSyncController alloc]
  //       initWithSyncService:syncService
  //           identityManager:identityManager
  //               prefService:_browserState->GetPrefs()];
  // }
  return _syncController;
}

#pragma mark - LeakCheckService

- (CWVLeakCheckService*)leakCheckService {
  // if (!_leakCheckService && self.persistent) {
  //   password_manager::BulkLeakCheckServiceInterface* bulkLeakCheckService =
  //       ios_web_view::WebViewBulkLeakCheckServiceFactory::GetForBrowserState(
  //           self.browserState);
  //   _leakCheckService = [[CWVLeakCheckService alloc]
  //       initWithBulkLeakCheckService:bulkLeakCheckService];
  // }
  return _leakCheckService;
}

#pragma mark - ReuseCheckService

- (CWVReuseCheckService*)reuseCheckService {
  // if (!_reuseCheckService && self.persistent) {
  //   affiliations::AffiliationService* affiliation_service =
  //       ios_web_view::WebViewAffiliationServiceFactory::GetForBrowserState(
  //           static_cast<ChromeBrowserState*>(self.browserState));

  //   _reuseCheckService = [[CWVReuseCheckService alloc]
  //       initWithAffiliationService:affiliation_service];
  // }
  return _reuseCheckService;
}

#pragma mark - Public Methods

- (BOOL)isPersistent {
  return !_browserState->IsOffTheRecord();
}

#pragma mark - Private Methods

- (ChromeBrowserState*)browserState {
  return _browserState;
}

- (void)registerWebView:(CWVWebView*)webView {
  [_webViews addObject:webView];
}

- (void)shutDown {
  // [_autofillDataManager shutDown];
  for (CWVWebView* webView in _webViews) {
    [webView shutDown];
  }
  _browserState = nullptr;
}

@end
