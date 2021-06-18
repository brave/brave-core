/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/app/brave_core_main.h"

#import <UIKit/UIKit.h>

#include "base/compiler_specific.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/ios/app/brave_main_delegate.h"
#include "brave/ios/browser/api/history/brave_history_api+private.h"
#include "brave/ios/browser/api/sync/driver/brave_sync_profile_service+private.h"
#include "brave/ios/browser/brave_web_client.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "ios/chrome/app/startup/provider_registration.h"
#include "ios/chrome/app/startup_tasks.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/history/web_history_service_factory.h"
#include "ios/chrome/browser/sync/profile_sync_service_factory.h"
#include "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#include "ios/web/public/init/web_main.h"

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#import "brave/ios/browser/api/wallet/brave_wallet_api+private.h"
#import "brave/ios/browser/api/wallet/brave_wallet_service_factory.h"
#endif

@interface BraveCoreMain () {
  std::unique_ptr<BraveWebClient> _webClient;
  std::unique_ptr<BraveMainDelegate> _delegate;
  std::unique_ptr<web::WebMain> _webMain;
  ChromeBrowserState* _mainBrowserState;
}

@property(nonatomic, readwrite) BraveHistoryAPI* historyAPI;
@property(nonatomic, readwrite) BraveSyncProfileServiceIOS* syncProfileService;

@property(nullable, nonatomic, readwrite) BraveWalletAPI* wallet;
@end

@implementation BraveCoreMain

- (instancetype)init {
  return [self initWithSyncServiceURL:@""];
}

- (instancetype)initWithSyncServiceURL:(NSString*)syncServiceURL {
  if ((self = [super init])) {
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(onAppEnterBackground:)
               name:UIApplicationDidEnterBackgroundNotification
             object:nil];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(onAppEnterForeground:)
               name:UIApplicationWillEnterForegroundNotification
             object:nil];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(onAppWillTerminate:)
               name:UIApplicationWillTerminateNotification
             object:nil];

    // Register all providers before calling any Chromium code.
    [ProviderRegistration registerProviders];

    _webClient.reset(new BraveWebClient());
    web::SetWebClient(_webClient.get());

    _delegate.reset(new BraveMainDelegate());
    _delegate->SetSyncServiceURL(base::SysNSStringToUTF8(syncServiceURL));

    web::WebMainParams params(_delegate.get());
    _webMain = std::make_unique<web::WebMain>(std::move(params));

    ios::GetChromeBrowserProvider()->Initialize();

    ios::ChromeBrowserStateManager* browserStateManager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    ChromeBrowserState* chromeBrowserState =
        browserStateManager->GetLastUsedBrowserState();
    _mainBrowserState = chromeBrowserState;
  }
  return self;
}

- (void)onAppEnterBackground:(NSNotification*)notification {
  auto* context = GetApplicationContext();
  if (context)
    context->OnAppEnterBackground();
}

- (void)onAppEnterForeground:(NSNotification*)notification {
  auto* context = GetApplicationContext();
  if (context)
    context->OnAppEnterForeground();
}

- (void)onAppWillTerminate:(NSNotification*)notification {
  _webMain.reset();
}

- (void)scheduleLowPriorityStartupTasks {
  [StartupTasks scheduleDeferredBrowserStateInitialization:_mainBrowserState];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  _mainBrowserState = nullptr;
  _webMain.reset();
  _delegate.reset();
  _webClient.reset();
}

- (void)setUserAgent:(NSString*)userAgent {
  _webClient->SetUserAgent(base::SysNSStringToUTF8(userAgent));
}

- (BraveHistoryAPI*)historyAPI {
  if (!_historyAPI) {
    history::HistoryService* history_service_ =
        ios::HistoryServiceFactory::GetForBrowserState(
            _mainBrowserState, ServiceAccessType::EXPLICIT_ACCESS);
    history::WebHistoryService* web_history_service_ =
        ios::WebHistoryServiceFactory::GetForBrowserState(_mainBrowserState);

    _historyAPI =
        [[BraveHistoryAPI alloc] initWithHistoryService:history_service_
                                      webHistoryService:web_history_service_];
  }
  return _historyAPI;
}

- (BraveSyncProfileServiceIOS*)syncProfileService {
  if (!_syncProfileService) {
    syncer::SyncService* sync_service_ =
        ProfileSyncServiceFactory::GetForBrowserState(_mainBrowserState);
    _syncProfileService = [[BraveSyncProfileServiceIOS alloc]
        initWithProfileSyncService:sync_service_];
  }
  return _syncProfileService;
}

- (nullable BraveWalletAPI*)wallet {
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  if (!_wallet) {
    auto* service =
        BraveWalletServiceFactory::GetForBrowserState(_mainBrowserState);
    _wallet = [[BraveWalletAPI alloc] initWithWalletService:service];
  }
  return _wallet;
#else
  return nil;
#endif
}

@end
