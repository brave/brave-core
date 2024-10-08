/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/app/brave_core_main.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include <memory>

#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/i18n/icu_util.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/p3a/buildflags.h"
#include "brave/components/p3a/histograms_braveizer.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/p3a_service.h"
#include "brave/ios/app/brave_main_delegate.h"
#include "brave/ios/browser/api/ai_chat/ai_chat+private.h"
#include "brave/ios/browser/api/ai_chat/ai_chat_delegate.h"
#include "brave/ios/browser/api/bookmarks/brave_bookmarks_api+private.h"
#include "brave/ios/browser/api/brave_shields/adblock_service+private.h"
#include "brave/ios/browser/api/brave_stats/brave_stats+private.h"
#include "brave/ios/browser/api/brave_wallet/brave_wallet_api+private.h"
#include "brave/ios/browser/api/content_settings/default_host_content_settings.h"
#include "brave/ios/browser/api/content_settings/default_host_content_settings_internal.h"
#include "brave/ios/browser/api/de_amp/de_amp_prefs+private.h"
#include "brave/ios/browser/api/history/brave_history_api+private.h"
#include "brave/ios/browser/api/ipfs/ipfs_api+private.h"
#include "brave/ios/browser/api/ntp_background_images/ntp_background_images_service_ios+private.h"
#include "brave/ios/browser/api/opentabs/brave_opentabs_api+private.h"
#include "brave/ios/browser/api/opentabs/brave_sendtab_api+private.h"
#include "brave/ios/browser/api/opentabs/brave_tabgenerator_api+private.h"
#include "brave/ios/browser/api/p3a/brave_p3a_utils+private.h"
#include "brave/ios/browser/api/password/brave_password_api+private.h"
#include "brave/ios/browser/api/prefs/browser_prefs+private.h"
#include "brave/ios/browser/api/sync/brave_sync_api+private.h"
#include "brave/ios/browser/api/sync/driver/brave_sync_profile_service+private.h"
#include "brave/ios/browser/api/web_image/web_image+private.h"
#include "brave/ios/browser/ui/webui/brave_web_ui_controller_factory.h"
#include "brave/ios/browser/web/brave_web_client.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_store/password_store.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/send_tab_to_self/send_tab_to_self_sync_service.h"
#include "components/sync/base/features.h"
#include "ios/chrome/app/startup/provider_registration.h"
#include "ios/chrome/browser/bookmarks/model/bookmark_model_factory.h"
#include "ios/chrome/browser/bookmarks/model/bookmark_undo_service_factory.h"
#include "ios/chrome/browser/browsing_data/model/browsing_data_remover.h"
#include "ios/chrome/browser/browsing_data/model/browsing_data_remover_factory.h"
#include "ios/chrome/browser/content_settings/model/host_content_settings_map_factory.h"
#include "ios/chrome/browser/credential_provider/model/credential_provider_buildflags.h"
#include "ios/chrome/browser/history/model/history_service_factory.h"
#include "ios/chrome/browser/history/model/web_history_service_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_profile_password_store_factory.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser/browser.h"
#include "ios/chrome/browser/shared/model/browser/browser_list.h"
#include "ios/chrome/browser/shared/model/browser/browser_list_factory.h"
#include "ios/chrome/browser/shared/model/paths/paths.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"
#include "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#include "ios/chrome/browser/shared/public/commands/command_dispatcher.h"
#include "ios/chrome/browser/sync/model/send_tab_to_self_sync_service_factory.h"
#include "ios/chrome/browser/sync/model/session_sync_service_factory.h"
#include "ios/chrome/browser/sync/model/sync_service_factory.h"
#include "ios/chrome/browser/webui/ui_bundled/chrome_web_ui_ios_controller_factory.h"
#include "ios/public/provider/chrome/browser/overrides/overrides_api.h"
#include "ios/public/provider/chrome/browser/ui_utils/ui_utils_api.h"
#include "ios/web/public/init/web_main.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "ios/web_view/internal/cwv_web_view_configuration_internal.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#include "ios/web_view/internal/web_view_download_manager.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

#if BUILDFLAG(IOS_CREDENTIAL_PROVIDER_ENABLED)
#include "ios/chrome/browser/credential_provider/model/credential_provider_service_factory.h"
#include "ios/chrome/browser/credential_provider/model/credential_provider_support.h"
#include "ios/chrome/browser/credential_provider/model/credential_provider_util.h"
#endif

// Chromium logging is global, therefore we cannot link this to the instance in
// question
static BraveCoreLogHandler _Nullable _logHandler = nil;

const BraveCoreLogSeverity BraveCoreLogSeverityFatal = logging::LOGGING_FATAL;
const BraveCoreLogSeverity BraveCoreLogSeverityError = logging::LOGGING_ERROR;
const BraveCoreLogSeverity BraveCoreLogSeverityWarning =
    logging::LOGGING_WARNING;
const BraveCoreLogSeverity BraveCoreLogSeverityInfo = logging::LOGGING_INFO;
const BraveCoreLogSeverity BraveCoreLogSeverityVerbose =
    logging::LOGGING_VERBOSE;

@interface BraveCoreMain () {
  std::unique_ptr<BraveWebClient> _webClient;
  std::unique_ptr<BraveMainDelegate> _delegate;
  std::vector<std::string> _argv_store;
  std::unique_ptr<const char*[]> _raw_args;
  std::unique_ptr<web::WebMain> _webMain;
  std::unique_ptr<Browser> _browser;
  std::unique_ptr<Browser> _otr_browser;
  std::unique_ptr<ios_web_view::WebViewDownloadManager> _downloadManager;
  std::unique_ptr<ios_web_view::WebViewDownloadManager> _otrDownloadManager;
  raw_ptr<BrowserList> _browserList;
  raw_ptr<BrowserList> _otr_browserList;
  raw_ptr<ProfileIOS> _main_profile;
  scoped_refptr<p3a::P3AService> _p3a_service;
  scoped_refptr<p3a::HistogramsBraveizer> _histogram_braveizer;
}
@property(nonatomic) BraveBookmarksAPI* bookmarksAPI;
@property(nonatomic) BraveHistoryAPI* historyAPI;
@property(nonatomic) BravePasswordAPI* passwordAPI;
@property(nonatomic) BraveOpenTabsAPI* openTabsAPI;
@property(nonatomic) BraveSendTabAPI* sendTabAPI;
@property(nonatomic) BraveSyncAPI* syncAPI;
@property(nonatomic) BraveSyncProfileServiceIOS* syncProfileService;
@property(nonatomic) BraveTabGeneratorAPI* tabGeneratorAPI;
@property(nonatomic) WebImageDownloader* webImageDownloader;
@property(nonatomic) BraveWalletAPI* braveWalletAPI;
@property(nonatomic) IpfsAPIImpl* ipfsAPI;
@property(nonatomic) BraveP3AUtils* p3aUtils;
@property(nonatomic) DeAmpPrefs* deAmpPrefs;
@property(nonatomic) NTPBackgroundImagesService* backgroundImagesService;
@property(nonatomic) DefaultHostContentSettings* defaultHostContentSettings;
@property(nonatomic) CWVWebViewConfiguration* defaultWebViewConfiguration;
@property(nonatomic) CWVWebViewConfiguration* nonPersistentWebViewConfiguration;
@end

@implementation BraveCoreMain

- (instancetype)initWithUserAgent:(NSString*)userAgent {
  return [self initWithUserAgent:userAgent additionalSwitches:@[]];
}

- (instancetype)initWithUserAgent:(NSString*)userAgent
               additionalSwitches:
                   (NSArray<BraveCoreSwitch*>*)additionalSwitches {
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

    @autoreleasepool {
      ios::RegisterPathProvider();

      // Bundled components are not supported on ios, so DIR_USER_DATA is passed
      // for all three arguments.
      component_updater::RegisterPathProvider(
          ios::DIR_USER_DATA, ios::DIR_USER_DATA, ios::DIR_USER_DATA);
    }

    NSBundle* baseBundle = base::apple::OuterBundle();
    base::apple::SetBaseBundleID(
        base::SysNSStringToUTF8([baseBundle bundleIdentifier]).c_str());

    // Register all providers before calling any Chromium code.
    [ProviderRegistration registerProviders];

    // Setup WebClient ([ClientRegistration registerClients])
    _webClient.reset(new BraveWebClient());
    web::SetWebClient(_webClient.get());

    _delegate.reset(new BraveMainDelegate());

    // Start Main ([ChromeMainStarter startChromeMain])
    web::WebMainParams params(_delegate.get());

    // Parse Switches, Features, Arguments (Command-Line Arguments)
    NSMutableArray* arguments =
        [[[NSProcessInfo processInfo] arguments] mutableCopy];
    NSMutableArray* switches = [[NSMutableArray alloc] init];
    for (BraveCoreSwitch* sv in additionalSwitches) {
      if (!sv.value) {
        [switches addObject:[NSString stringWithFormat:@"--%@", sv.key]];
      } else {
        [switches
            addObject:[NSString stringWithFormat:@"--%@=%@", sv.key, sv.value]];
      }
    }
    [arguments addObjectsFromArray:switches];
    params.argc = [arguments count];

    // Allocate memory to convert from iOS arguments to Native arguments
    _raw_args.reset(new const char*[params.argc]);
    _argv_store.resize([arguments count]);

    for (NSUInteger i = 0; i < [arguments count]; i++) {
      _argv_store[i] = base::SysNSStringToUTF8([arguments objectAtIndex:i]);
      _raw_args[i] = _argv_store[i].c_str();
    }
    params.argv = _raw_args.get();

    // Setup WebMain
    _webMain = std::make_unique<web::WebMain>(std::move(params));

    // Initialize and set the main browser state.
    std::vector<ProfileIOS*> profiles =
        GetApplicationContext()->GetProfileManager()->GetLoadedProfiles();
    ProfileIOS* last_used_profile = profiles.at(0);
    _main_profile = last_used_profile;

    // Disable Safe-Browsing via Prefs
    last_used_profile->GetPrefs()->SetBoolean(prefs::kSafeBrowsingEnabled,
                                              false);

    // Setup main browser
    _browserList = BrowserListFactory::GetForProfile(_main_profile);
    _browser = Browser::Create(_main_profile, {});
    _browserList->AddBrowser(_browser.get());

    // Setup otr browser
    ProfileIOS* otr_last_used_profile =
        last_used_profile->GetOffTheRecordProfile();
    _otr_browserList = BrowserListFactory::GetForProfile(otr_last_used_profile);
    _otr_browser = Browser::Create(otr_last_used_profile, {});
    _otr_browserList->AddBrowser(_otr_browser.get());

    // Setup download managers for CWVWebView
    _downloadManager = std::make_unique<ios_web_view::WebViewDownloadManager>(
        _mainBrowserState);
    _otrDownloadManager =
        std::make_unique<ios_web_view::WebViewDownloadManager>(
            otrChromeBrowserState);

    // Initialize the provider UI global state.
    ios::provider::InitializeUI();

    // Setup WebUI (Sync Internals and other WebViews)
    web::WebUIIOSControllerFactory::RegisterFactory(
        BraveWebUIControllerFactory::GetInstance());

    // TODO(darkdh): move _adblockService and _backgroundImageService to
    // BraveWebMainParts::PreMainMessageLoopRun
    // https://github.com/brave/brave-browser/issues/40567
    component_updater::ComponentUpdateService* cus =
        GetApplicationContext()->GetComponentUpdateService();

    _adblockService = [[AdblockService alloc] initWithComponentUpdater:cus];

    _backgroundImagesService = [[NTPBackgroundImagesService alloc]
        initWithBackgroundImagesService:
            std::make_unique<ntp_background_images::NTPBackgroundImagesService>(
                cus, GetApplicationContext()->GetLocalState())];

#if BUILDFLAG(IOS_CREDENTIAL_PROVIDER_ENABLED)
    if (IsCredentialProviderExtensionSupported()) {
      CredentialProviderServiceFactory::GetForProfile(_main_profile);
    }
#endif
  }
  return self;
}

- (void)dealloc {
  _backgroundImagesService = nil;
  _adblockService = nil;
  _bookmarksAPI = nil;
  _historyAPI = nil;
  _openTabsAPI = nil;
  _passwordAPI = nil;
  _sendTabAPI = nil;
  _syncProfileService = nil;
  _syncAPI = nil;
  _tabGeneratorAPI = nil;
  _webImageDownloader = nil;

  [_nonPersistentWebViewConfiguration shutDown];
  [_defaultWebViewConfiguration shutDown];

  _nonPersistentWebViewConfiguration = nil;
  _defaultWebViewConfiguration = nil;

  _downloadManager.reset();
  _otrDownloadManager.reset();

  _otr_browserList =
      BrowserListFactory::GetForProfile(_otr_browser->GetProfile());
  [_otr_browser->GetCommandDispatcher() prepareForShutdown];
  _otr_browserList->RemoveBrowser(_otr_browser.get());
  CloseAllWebStates(*_otr_browser->GetWebStateList(),
                    WebStateList::CLOSE_NO_FLAGS);
  _otr_browser.reset();

  _browserList = BrowserListFactory::GetForProfile(_browser->GetProfile());
  [_browser->GetCommandDispatcher() prepareForShutdown];
  _browserList->RemoveBrowser(_browser.get());
  CloseAllWebStates(*_browser->GetWebStateList(), WebStateList::CLOSE_NO_FLAGS);
  _browser.reset();

  _main_profile = nullptr;
  _webMain.reset();
  _raw_args.reset();
  _argv_store = {};
  _delegate.reset();
  _webClient.reset();

  VLOG(1) << "Terminated Brave-Core";
}

- (void)onAppEnterBackground:(NSNotification*)notification {
  auto* context = GetApplicationContext();
  if (context) {
    context->OnAppEnterBackground();
    // Since we don't use the WebViewWebMainParts, local state is never commited
    // on app background
    context->GetLocalState()->CommitPendingWrite();
  }
}

- (void)onAppEnterForeground:(NSNotification*)notification {
  auto* context = GetApplicationContext();
  if (context)
    context->OnAppEnterForeground();
}

- (void)onAppWillTerminate:(NSNotification*)notification {
  // ApplicationContextImpl doesn't get teardown call at the moment because we
  // cannot dealloc this class yet without crashing.
  GetApplicationContext()->GetLocalState()->CommitPendingWrite();
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)scheduleLowPriorityStartupTasks {
  // Install overrides
  ios::provider::InstallOverrides();

  // Make sure the system url request getter is called at least once during
  // startup in case cleanup is done early before first network request
  GetApplicationContext()->GetSystemURLRequestContext();

#if BUILDFLAG(IOS_CREDENTIAL_PROVIDER_ENABLED)
  [self performFaviconsCleanup];
#endif
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
  if (severity > logging::LOGGING_VERBOSE ||
      severity <= logging::GetVlogLevelHelper(file, strlen(file))) {
    return _logHandler(severity, base::SysUTF8ToNSString(file), line,
                       message_start, base::SysUTF8ToNSString(str));
  }
  return true;
}

#pragma mark -

- (BraveBookmarksAPI*)bookmarksAPI {
  if (!_bookmarksAPI) {
    ProfileIOS* profile = ProfileIOS::FromBrowserState(_main_profile);
    bookmarks::BookmarkModel* bookmark_model =
        ios::BookmarkModelFactory::GetForProfile(profile);
    BookmarkUndoService* bookmark_undo_service =
        ios::BookmarkUndoServiceFactory::GetForProfile(profile);

    _bookmarksAPI =
        [[BraveBookmarksAPI alloc] initWithBookmarkModel:bookmark_model
                                     bookmarkUndoService:bookmark_undo_service];
  }
  return _bookmarksAPI;
}

- (BraveHistoryAPI*)historyAPI {
  if (!_historyAPI) {
    _historyAPI = [[BraveHistoryAPI alloc] initWithBrowserState:_main_profile];
  }
  return _historyAPI;
}

- (BraveOpenTabsAPI*)openTabsAPI {
  if (!_openTabsAPI) {
    syncer::SyncService* sync_service_ =
        SyncServiceFactory::GetForProfile(_main_profile);

    sync_sessions::SessionSyncService* session_sync_service_ =
        SessionSyncServiceFactory::GetForProfile(_main_profile);

    _openTabsAPI =
        [[BraveOpenTabsAPI alloc] initWithSyncService:sync_service_
                                   sessionSyncService:session_sync_service_];
  }
  return _openTabsAPI;
}

- (BravePasswordAPI*)passwordAPI {
  if (!_passwordAPI) {
    scoped_refptr<password_manager::PasswordStoreInterface> password_store_ =
        IOSChromeProfilePasswordStoreFactory::GetForProfile(
            _main_profile, ServiceAccessType::EXPLICIT_ACCESS)
            .get();

    _passwordAPI =
        [[BravePasswordAPI alloc] initWithPasswordStore:password_store_];
  }
  return _passwordAPI;
}

- (BraveSendTabAPI*)sendTabAPI {
  if (!_sendTabAPI) {
    send_tab_to_self::SendTabToSelfSyncService* sync_service_ =
        SendTabToSelfSyncServiceFactory::GetForProfile(_main_profile);

    _sendTabAPI = [[BraveSendTabAPI alloc] initWithSyncService:sync_service_];
  }
  return _sendTabAPI;
}

- (BraveSyncAPI*)syncAPI {
  if (!_syncAPI) {
    _syncAPI = [[BraveSyncAPI alloc] initWithBrowserState:_main_profile];
  }
  return _syncAPI;
}

- (BraveSyncProfileServiceIOS*)syncProfileService {
  if (!_syncProfileService) {
    syncer::SyncService* sync_service_ =
        SyncServiceFactory::GetForProfile(_main_profile);
    _syncProfileService = [[BraveSyncProfileServiceIOS alloc]
        initWithProfileSyncService:sync_service_];
  }
  return _syncProfileService;
}

- (BraveTabGeneratorAPI*)tabGeneratorAPI {
  if (!_tabGeneratorAPI) {
    _tabGeneratorAPI =
        [[BraveTabGeneratorAPI alloc] initWithBrowser:_browser.get()];
  }
  return _tabGeneratorAPI;
}

- (WebImageDownloader*)webImageDownloader {
  if (!_webImageDownloader) {
    _webImageDownloader = [[WebImageDownloader alloc]
        initWithBrowserState:_otr_browser->GetProfile()];
  }
  return _webImageDownloader;
}

- (BraveWalletAPI*)braveWalletAPI {
  if (!_braveWalletAPI) {
    _braveWalletAPI =
        [[BraveWalletAPI alloc] initWithBrowserState:_main_profile];
  }
  return _braveWalletAPI;
}

- (BraveStats*)braveStats {
  return [[BraveStats alloc] initWithBrowserState:_main_profile];
}

- (id<IpfsAPI>)ipfsAPI {
  if (!_ipfsAPI) {
    _ipfsAPI = [[IpfsAPIImpl alloc] initWithBrowserState:_main_profile];
  }
  return _ipfsAPI;
}

- (void)initializeP3AServiceForChannel:(NSString*)channel
                         weekOfInstall:(NSString*)weekOfInstall {
#if BUILDFLAG(BRAVE_P3A_ENABLED)
  _p3a_service = base::MakeRefCounted<p3a::P3AService>(
      *GetApplicationContext()->GetLocalState(),
      base::SysNSStringToUTF8(channel), base::SysNSStringToUTF8(weekOfInstall),
      p3a::P3AConfig::LoadFromCommandLine());
  _p3a_service->InitCallbacks();
  _p3a_service->Init(GetApplicationContext()->GetSharedURLLoaderFactory());
  _histogram_braveizer = p3a::HistogramsBraveizer::Create();
#endif  // BUILDFLAG(BRAVE_P3A_ENABLED)
}

- (BraveP3AUtils*)p3aUtils {
  if (!_p3aUtils) {
    _p3aUtils = [[BraveP3AUtils alloc]
        initWithLocalState:GetApplicationContext()->GetLocalState()
                p3aService:_p3a_service];
  }
  return _p3aUtils;
}

- (DeAmpPrefs*)deAmpPrefs {
  if (!_deAmpPrefs) {
    _deAmpPrefs =
        [[DeAmpPrefs alloc] initWithProfileState:_main_profile->GetPrefs()];
  }
  return _deAmpPrefs;
}

- (BrowserPrefs*)browserPrefs {
  return
      [[BrowserPrefs alloc] initWithPrefService:_mainBrowserState->GetPrefs()];
}

- (AIChat*)aiChatAPIWithDelegate:(id<AIChatDelegate>)delegate {
  return [[AIChat alloc] initWithProfileIOS:_main_profile delegate:delegate];
}

+ (bool)initializeICUForTesting {
  NSBundle* bundle = [NSBundle bundleForClass:self];
  base::apple::SetOverrideOuterBundle(bundle);
  base::apple::SetOverrideFrameworkBundle(bundle);
  return base::i18n::InitializeICU();
}

#if BUILDFLAG(IOS_CREDENTIAL_PROVIDER_ENABLED)
- (void)performFaviconsCleanup {
  ProfileIOS* browserState = _main_profile;
  if (!browserState) {
    return;
  }
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&UpdateFaviconsStorageForProfile,
                                browserState->AsWeakPtr(),
                                /*fallback_to_google_server=*/false));
}
#endif

- (DefaultHostContentSettings*)defaultHostContentSettings {
  if (!_defaultHostContentSettings) {
    HostContentSettingsMap* map =
        ios::HostContentSettingsMapFactory::GetForBrowserState(
            _mainBrowserState);
    _defaultHostContentSettings =
        [[DefaultHostContentSettings alloc] initWithSettingsMap:map];
  }
  return _defaultHostContentSettings;
}

- (CWVWebViewConfiguration*)defaultWebViewConfiguration {
  if (!_defaultWebViewConfiguration) {
    _defaultWebViewConfiguration = [[CWVWebViewConfiguration alloc]
        initWithBrowserState:ios_web_view::WebViewBrowserState::
                                 FromBrowserState(_mainBrowserState)];
  }
  return _defaultWebViewConfiguration;
}

- (CWVWebViewConfiguration*)nonPersistentWebViewConfiguration {
  if (!_nonPersistentWebViewConfiguration) {
    _nonPersistentWebViewConfiguration = [[CWVWebViewConfiguration alloc]
        initWithBrowserState:
            ios_web_view::WebViewBrowserState::FromBrowserState(
                _mainBrowserState->GetOffTheRecordChromeBrowserState())];
  }
  return _nonPersistentWebViewConfiguration;
}

#pragma mark - Handling of destroying the incognito BrowserState

// The incognito BrowserState should be closed when the last incognito tab is
// closed (i.e. if there are other incognito tabs open in another Scene, the
// BrowserState must not be destroyed).
- (BOOL)shouldDestroyAndRebuildIncognitoBrowserState {
  return _mainBrowserState->HasOffTheRecordChromeBrowserState();
}

// Matches lastIncognitoTabClosed from Chrome's SceneController
- (void)notifyLastPrivateTabClosed {
  // If no other window has incognito tab, then destroy and rebuild the
  // BrowserState. Otherwise, just do the state transition animation.
  if ([self shouldDestroyAndRebuildIncognitoBrowserState]) {
    // Incognito browser state cannot be deleted before all the requests are
    // deleted. Queue empty task on IO thread and destroy the BrowserState
    // when the task has executed, again verifying that no incognito tabs are
    // present. When an incognito tab is moved between browsers, there is
    // a point where the tab isn't attached to any web state list. However, when
    // this queued cleanup step executes, the moved tab will be attached, so
    // the cleanup shouldn't proceed.

    auto cleanup = ^{
      if ([self shouldDestroyAndRebuildIncognitoBrowserState]) {
        [self destroyAndRebuildIncognitoBrowserState];
      }
    };

    web::GetIOThreadTaskRunner({})->PostTaskAndReply(
        FROM_HERE, base::DoNothing(), base::BindRepeating(cleanup));
  }
}

// Matches cleanupBrowser from Chrome's BrowserViewWrangler
- (void)cleanupBrowser:(Browser*)browser {
  DCHECK(browser);

  // Remove the Browser from the browser list. The browser itself is still
  // alive during this call, so any observer can act on it.
  ProfileIOS* profile = browser->GetProfile();
  BrowserList* browserList = BrowserListFactory::GetForProfile(profile);
  browserList->RemoveBrowser(browser);

  WebStateList* webStateList = browser->GetWebStateList();
  // Close all webstates in `webStateList`. Do this in an @autoreleasepool as
  // WebStateList observers will be notified (they are unregistered later). As
  // some of them may be implemented in Objective-C and unregister themselves
  // in their -dealloc method, ensure the -autorelease introduced by ARC are
  // processed before the WebStateList destructor is called.
  @autoreleasepool {
    CloseAllWebStates(*webStateList, WebStateList::CLOSE_NO_FLAGS);
  }
}

- (void)destroyAndRebuildIncognitoBrowserState {
  DCHECK(_mainBrowserState->HasOffTheRecordChromeBrowserState());
  _nonPersistentWebViewConfiguration = nil;

  ChromeBrowserState* otrBrowserState =
      _mainBrowserState->GetOffTheRecordChromeBrowserState();

  BrowsingDataRemover* browsingDataRemover =
      BrowsingDataRemoverFactory::GetForBrowserState(otrBrowserState);
  browsingDataRemover->Remove(browsing_data::TimePeriod::ALL_TIME,
                              BrowsingDataRemoveMask::REMOVE_ALL,
                              base::DoNothing());

  [self cleanupBrowser:_otr_browser.get()];
  _otr_browser.reset();

  // Destroy and recreate the off-the-record BrowserState.
  _mainBrowserState->DestroyOffTheRecordChromeBrowserState();

  otrBrowserState = _mainBrowserState->GetOffTheRecordChromeBrowserState();
  _otr_browser = Browser::Create(otrBrowserState, {});

  BrowserList* browserList =
      BrowserListFactory::GetForBrowserState(otrBrowserState);
  browserList->AddBrowser(_otr_browser.get());
}

@end
