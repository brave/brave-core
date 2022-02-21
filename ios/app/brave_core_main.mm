/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/app/brave_core_main.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "base/base_switches.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_impl.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "brave/components/brave_wallet/resources/grit/brave_wallet_script_generated.h"
#include "brave/components/skus/browser/switches.h"
#include "brave/ios/app/brave_main_delegate.h"
#include "brave/ios/browser/api/bookmarks/brave_bookmarks_api+private.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet.mojom.objc+private.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet_provider_delegate_ios+private.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet_provider_delegate_ios.h"
#include "brave/ios/browser/api/history/brave_history_api+private.h"
#include "brave/ios/browser/api/password/brave_password_api+private.h"
#include "brave/ios/browser/api/sync/brave_sync_api+private.h"
#include "brave/ios/browser/api/sync/driver/brave_sync_profile_service+private.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/ios/browser/brave_wallet/keyring_service_factory.h"
#include "brave/ios/browser/brave_wallet/tx_service_factory.h"
#include "brave/ios/browser/brave_web_client.h"
#include "brave/ios/browser/component_updater/component_updater_utils.h"
#include "components/component_updater/component_updater_switches.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/sync/base/command_line_switches.h"
#include "ios/chrome/app/startup/provider_registration.h"
#include "ios/chrome/app/startup_tasks.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/history/web_history_service_factory.h"
#include "ios/chrome/browser/passwords/ios_chrome_password_store_factory.h"
#include "ios/chrome/browser/sync/sync_service_factory.h"
#include "ios/chrome/browser/ui/webui/chrome_web_ui_ios_controller_factory.h"
#include "ios/chrome/browser/undo/bookmark_undo_service_factory.h"
#include "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#include "ios/public/provider/chrome/browser/ui_utils/ui_utils_api.h"
#include "ios/web/public/init/web_main.h"
#include "ui/base/resource/resource_bundle.h"

// Chromium logging is global, therefore we cannot link this to the instance in
// question
static BraveCoreLogHandler _Nullable _logHandler = nil;

const BraveCoreSwitch BraveCoreSwitchComponentUpdater =
    base::SysUTF8ToNSString(switches::kComponentUpdater);
const BraveCoreSwitch BraveCoreSwitchVModule =
    base::SysUTF8ToNSString(switches::kVModule);
const BraveCoreSwitch BraveCoreSwitchSyncURL =
    base::SysUTF8ToNSString(syncer::kSyncServiceURL);
const BraveCoreSwitch BraveCoreSwitchSkusEnvironment =
    base::SysUTF8ToNSString(skus::switches::kSkusEnv);

@interface BraveCoreMain () {
  std::unique_ptr<BraveWebClient> _webClient;
  std::unique_ptr<BraveMainDelegate> _delegate;
  std::unique_ptr<web::WebMain> _webMain;
  ChromeBrowserState* _mainBrowserState;
  NSString* _walletProviderJS;
}
@property(nonatomic) BraveBookmarksAPI* bookmarksAPI;
@property(nonatomic) BraveHistoryAPI* historyAPI;
@property(nonatomic) BravePasswordAPI* passwordAPI;
@property(nonatomic) BraveSyncAPI* syncAPI;
@property(nonatomic) BraveSyncProfileServiceIOS* syncProfileService;
@end

@implementation BraveCoreMain

- (instancetype)initWithUserAgent:(NSString*)userAgent {
  return [self initWithUserAgent:userAgent additionalSwitches:@{}];
}

- (instancetype)initWithUserAgent:(NSString*)userAgent
               additionalSwitches:(NSDictionary<BraveCoreSwitch, NSString*>*)
                                      additionalSwitches {
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
    _webClient->SetUserAgent(base::SysNSStringToUTF8(userAgent));
    web::SetWebClient(_webClient.get());

    _delegate.reset(new BraveMainDelegate());

    web::WebMainParams params(_delegate.get());
    NSMutableArray* arguments =
        [[[NSProcessInfo processInfo] arguments] mutableCopy];
    NSMutableArray* switches = [[NSMutableArray alloc] init];
    for (NSString* key in additionalSwitches) {
      if (![additionalSwitches[key] isKindOfClass:NSString.class]) {
        continue;
      }
      [switches
          addObject:[NSString stringWithFormat:@"--%@=%@", key,
                                               static_cast<NSString*>(
                                                   additionalSwitches[key])]];
    }
    [arguments addObjectsFromArray:switches];
    params.argc = [arguments count];
    const char* argv[params.argc];
    std::vector<std::string> argv_store;
    argv_store.resize([arguments count]);
    for (NSUInteger i = 0; i < [arguments count]; i++) {
      argv_store[i] = base::SysNSStringToUTF8([arguments objectAtIndex:i]);
      argv[i] = argv_store[i].c_str();
    }
    params.argv = argv;
    _webMain = std::make_unique<web::WebMain>(std::move(params));

    ios::provider::InitializeUI();

    web::WebUIIOSControllerFactory::RegisterFactory(
        ChromeWebUIIOSControllerFactory::GetInstance());

    [self registerComponentsForUpdate];

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
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  _mainBrowserState = nullptr;
  _webMain.reset();
  _delegate.reset();
  _webClient.reset();
}

- (void)scheduleLowPriorityStartupTasks {
  [StartupTasks scheduleDeferredBrowserStateInitialization:_mainBrowserState];
}

- (void)registerComponentsForUpdate {
  brave_component_updater::BraveOnDemandUpdater::GetInstance()
      ->RegisterOnDemandUpdateCallback(
          base::BindRepeating(&component_updater::BraveOnDemandUpdate));

  component_updater::ComponentUpdateService* cus =
      GetApplicationContext()->GetComponentUpdateService();
  DCHECK(cus);

  brave_wallet::RegisterWalletDataFilesComponent(cus);
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  _mainBrowserState = nullptr;
  _webMain.reset();
  _delegate.reset();
  _webClient.reset();
}

+ (void)setLogHandler:(BraveCoreLogHandler)logHandler {
  _logHandler = logHandler;
  logging::SetLogMessageHandler(&CustomLogHandler);
}

static bool CustomLogHandler(int severity,
                             const char* file,
                             int line,
                             size_t message_start,
                             const std::string& str) {
  if (!_logHandler) {
    return false;
  }
  const int vlog_level = logging::GetVlogLevelHelper(file, strlen(file));
  if (severity <= vlog_level) {
    return _logHandler(severity, base::SysUTF8ToNSString(file), line,
                       message_start, base::SysUTF8ToNSString(str));
  }
  return true;
}

#pragma mark -

- (BraveBookmarksAPI*)bookmarksAPI {
  if (!_bookmarksAPI) {
    bookmarks::BookmarkModel* bookmark_model_ =
        ios::BookmarkModelFactory::GetForBrowserState(_mainBrowserState);
    BookmarkUndoService* bookmark_undo_service_ =
        ios::BookmarkUndoServiceFactory::GetForBrowserState(_mainBrowserState);

    _bookmarksAPI = [[BraveBookmarksAPI alloc]
        initWithBookmarkModel:bookmark_model_
          bookmarkUndoService:bookmark_undo_service_];
  }
  return _bookmarksAPI;
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

- (BraveSyncAPI*)syncAPI {
  if (!_syncAPI) {
    _syncAPI = [[BraveSyncAPI alloc] initWithBrowserState:_mainBrowserState];
  }
  return _syncAPI;
}

- (BravePasswordAPI*)passwordAPI {
  if (!_passwordAPI) {
    scoped_refptr<password_manager::PasswordStoreInterface> password_store_ =
        IOSChromePasswordStoreFactory::GetForBrowserState(
            _mainBrowserState, ServiceAccessType::EXPLICIT_ACCESS)
            .get();

    _passwordAPI =
        [[BravePasswordAPI alloc] initWithPasswordStore:password_store_];
  }
  return _passwordAPI;
}

- (BraveSyncProfileServiceIOS*)syncProfileService {
  if (!_syncProfileService) {
    syncer::SyncService* sync_service_ =
        SyncServiceFactory::GetForBrowserState(_mainBrowserState);
    _syncProfileService = [[BraveSyncProfileServiceIOS alloc]
        initWithProfileSyncService:sync_service_];
  }
  return _syncProfileService;
}

+ (id<BraveWalletBlockchainRegistry>)blockchainRegistry {
  auto* registry = brave_wallet::BlockchainRegistry::GetInstance();
  return [[BraveWalletBlockchainRegistryImpl alloc]
      initWithBlockchainRegistry:registry];
}

- (nullable id<BraveWalletBraveWalletProvider>)
    walletProviderWithDelegate:(id<BraveWalletProviderDelegate>)delegate
             isPrivateBrowsing:(bool)isPrivateBrowsing {
  auto* browserState = _mainBrowserState;
  if (isPrivateBrowsing) {
    browserState = browserState->GetOffTheRecordChromeBrowserState();
  }

  auto* json_rpc_service =
      brave_wallet::JsonRpcServiceFactory::GetServiceForState(browserState);
  if (!json_rpc_service) {
    return nil;
  }

  auto* tx_service =
      brave_wallet::TxServiceFactory::GetServiceForState(browserState);
  if (!tx_service) {
    return nil;
  }

  auto* keyring_service =
      brave_wallet::KeyringServiceFactory::GetServiceForState(browserState);
  if (!keyring_service) {
    return nil;
  }

  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForState(browserState);
  if (!brave_wallet_service) {
    return nil;
  }

  auto* provider = new brave_wallet::BraveWalletProviderImpl(
      ios::HostContentSettingsMapFactory::GetForBrowserState(browserState),
      json_rpc_service, tx_service, keyring_service, brave_wallet_service,
      std::make_unique<brave_wallet::BraveWalletProviderDelegateBridge>(
          delegate),
      browserState->GetPrefs());
  return [[BraveWalletBraveWalletProviderImpl alloc]
      initWithBraveWalletProvider:provider];
}

- (NSString*)walletProviderJS {
  if (!_walletProviderJS) {
    auto resource_id = IDR_BRAVE_WALLET_SCRIPT_BRAVE_WALLET_SCRIPT_BUNDLE_JS;
    // The resource bundle is not available until after WebMainParts is setup
    auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
    std::string resource_string = "";
    if (resource_bundle.IsGzipped(resource_id)) {
      resource_string =
          std::string(resource_bundle.LoadDataResourceString(resource_id));
    } else {
      resource_string =
          std::string(resource_bundle.GetRawDataResource(resource_id));
    }
    _walletProviderJS = base::SysUTF8ToNSString(resource_string);
  }
  return _walletProviderJS;
}

@end
