/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/app/brave_core_main.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "base/allocator/partition_alloc_support.h"
#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/i18n/icu_util.h"
#include "base/ios/scoped_critical_action.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/types/fixed_array.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/p3a/buildflags.h"
#include "brave/components/p3a/histograms_braveizer.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/p3a_service.h"
#include "brave/ios/app/brave_ios_main.h"
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
#include "brave/ios/browser/api/https_upgrade_exceptions/https_upgrade_exceptions_service+private.h"
#include "brave/ios/browser/api/ipfs/ipfs_api+private.h"
#include "brave/ios/browser/api/ntp_background_images/ntp_background_images_service_ios+private.h"
#include "brave/ios/browser/api/opentabs/brave_opentabs_api+private.h"
#include "brave/ios/browser/api/opentabs/brave_sendtab_api+private.h"
#include "brave/ios/browser/api/opentabs/brave_tabgenerator_api+private.h"
#include "brave/ios/browser/api/p3a/brave_p3a_utils+private.h"
#include "brave/ios/browser/api/password/brave_password_api+private.h"
#include "brave/ios/browser/api/sync/brave_sync_api+private.h"
#include "brave/ios/browser/api/sync/driver/brave_sync_profile_service+private.h"
#include "brave/ios/browser/api/web_image/web_image+private.h"
#include "brave/ios/browser/ui/webui/brave_web_ui_controller_factory.h"
#include "brave/ios/browser/web/brave_web_client.h"
#import "build/blink_buildflags.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/download/public/background_service/background_download_service.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_store/password_store.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/send_tab_to_self/send_tab_to_self_sync_service.h"
#include "components/sync/base/features.h"
#include "ios/chrome/app/application_delegate/app_state.h"
#include "ios/chrome/app/application_delegate/memory_warning_helper.h"
#include "ios/chrome/app/main_controller.h"
#include "ios/chrome/app/startup/provider_registration.h"
#include "ios/chrome/browser/bookmarks/model/bookmark_model_factory.h"
#include "ios/chrome/browser/bookmarks/model/bookmark_undo_service_factory.h"
#include "ios/chrome/browser/browsing_data/model/browsing_data_remover.h"
#include "ios/chrome/browser/browsing_data/model/browsing_data_remover_factory.h"
#include "ios/chrome/browser/content_settings/model/host_content_settings_map_factory.h"
#include "ios/chrome/browser/credential_provider/model/credential_provider_buildflags.h"
#include "ios/chrome/browser/download/model/background_service/background_download_service_factory.h"
#include "ios/chrome/browser/history/model/history_service_factory.h"
#include "ios/chrome/browser/history/model/web_history_service_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_profile_password_store_factory.h"
#include "ios/chrome/browser/sessions/model/session_restoration_service.h"
#include "ios/chrome/browser/sessions/model/session_restoration_service_factory.h"
#include "ios/chrome/browser/sessions/model/session_util.h"
#include "ios/chrome/browser/shared/coordinator/scene/scene_state.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser/browser.h"
#include "ios/chrome/browser/shared/model/browser/browser_list.h"
#include "ios/chrome/browser/shared/model/browser/browser_list_factory.h"
#include "ios/chrome/browser/shared/model/browser/browser_provider.h"
#include "ios/chrome/browser/shared/model/browser/browser_provider_interface.h"
#include "ios/chrome/browser/shared/model/paths/paths.h"
#include "ios/chrome/browser/shared/model/prefs/pref_names.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"
#include "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#include "ios/chrome/browser/shared/public/commands/command_dispatcher.h"
#include "ios/chrome/browser/sync/model/send_tab_to_self_sync_service_factory.h"
#include "ios/chrome/browser/sync/model/session_sync_service_factory.h"
#include "ios/chrome/browser/sync/model/sync_service_factory.h"
#include "ios/chrome/browser/tabs/model/inactive_tabs/utils.h"
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
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "brave/ios/app/brave_app_state.h"
#include "ios/chrome/browser/shared/coordinator/scene/scene_state.h"
#include "ios/chrome/browser/shared/coordinator/scene/scene_delegate.h"
#include "ios/chrome/app/startup/ios_chrome_main.h"

#if BUILDFLAG(IOS_CREDENTIAL_PROVIDER_ENABLED)
#include "ios/chrome/browser/credential_provider/model/credential_provider_service_factory.h"
#include "ios/chrome/browser/credential_provider/model/credential_provider_util.h"
#endif

class ScopedAllowBlockingForProfile : public base::ScopedAllowBlocking {};

namespace brave {
ProfileIOS* CreateMainProfileIOS() {
  // Initialize and set the main browser state.
  auto* localState = GetApplicationContext()->GetLocalState();
  auto* profileManager = GetApplicationContext()->GetProfileManager();
  std::string profileName =
      "Default";  // kIOSChromeInitialProfile which is now removed
  // Set this as the last used profile always so that its saved for the future
  // where we may have multiple profile support and need to read it from local
  // state before creating the profile
  localState->SetString(prefs::kLastUsedProfile, profileName);
  ScopedAllowBlockingForProfile allow_blocking;
  return profileManager->CreateProfile(profileName);
}
}  // namespace brave

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

namespace {
// The time delay after firstSceneWillEnterForeground: before checking for main
// intent signals.
constexpr base::TimeDelta kMainIntentCheckDelay = base::Seconds(1);
}  // namespace

@interface BraveCoreMain () {
  std::unique_ptr<base::AtExitManager> _exit_manager;
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
@property(nonatomic) MemoryWarningHelper* memoryHelper;
@property(nonatomic) MainController* mainController;
@property(nonatomic) BraveAppState* appState;
@property(nonatomic) bool didFinishLaunching;

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
@property(nonatomic)
    HTTPSUpgradeExceptionsService* httpsUpgradeExceptionsService;
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
    BraveIOSMain::InitStartTime();
    _exit_manager = std::make_unique<base::AtExitManager>();

    @autoreleasepool {
      ios::RegisterPathProvider();

      // Bundled components are not supported on ios, so DIR_USER_DATA is passed
      // for all three arguments.
      component_updater::RegisterPathProvider(
          ios::DIR_USER_DATA, ios::DIR_USER_DATA, ios::DIR_USER_DATA);
    }

#if PA_BUILDFLAG(USE_PARTITION_ALLOC) && !BUILDFLAG(USE_BLINK)
    // ContentMainRunnerImpl::Initialize calls this when USE_BLINK is true.
    base::allocator::PartitionAllocSupport::Get()->ReconfigureEarlyish("");
#endif  // PA_BUILDFLAG(USE_PARTITION_ALLOC) && !BUILDFLAG(USE_BLINK)

    NSBundle* baseBundle = base::apple::OuterBundle();
    base::apple::SetBaseBundleID(
        base::SysNSStringToUTF8([baseBundle bundleIdentifier]).c_str());

    // Register all providers before calling any Chromium code.
    [ProviderRegistration registerProviders];

    // Setup WebClient ([ClientRegistration registerClients])
    web::SetWebClient(new BraveWebClient());
    if (userAgent != nil) {
      reinterpret_cast<BraveWebClient*>(web::GetWebClient())->SetLegacyUserAgent(base::SysNSStringToUTF8(userAgent));
    }
    

    // Parse Switches, Features, Arguments (Command-Line Arguments)
    NSMutableArray* arguments =
        [[[NSProcessInfo processInfo] arguments] mutableCopy];
    for (BraveCoreSwitch* sv in additionalSwitches) {
      if (!sv.value) {
        [arguments addObject:[NSString stringWithFormat:@"--%@", sv.key]];
      } else {
        [arguments
            addObject:[NSString stringWithFormat:@"--%@=%@", sv.key, sv.value]];
      }
    }

    base::FixedArray<const char*> argv([arguments count]);
    std::vector<std::string> argv_store([arguments count]);

    for (NSUInteger i = 0; i < [arguments count]; ++i) {
      argv_store[i] = base::SysNSStringToUTF8([arguments objectAtIndex:i]);
      argv[i] = argv_store[i].c_str();
    }

    // Initialize Command-Line BEFORE anything else
    base::CommandLine::Init([arguments count], argv.data());

    // Setup Chromium
    _memoryHelper = [[MemoryWarningHelper alloc] init];
    _mainController = [[MainController alloc] init];
    _appState = [[BraveAppState alloc] initWithStartupInformation:_mainController];
    [_mainController setAppState:_appState];
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

  VLOG(1) << "Terminated Brave-Core";
}

- (void)profileLoaded:(ProfileIOS*)profile {
  _main_profile = profile;

  // Disable Safe-Browsing via Prefs
  profile->GetPrefs()->SetBoolean(prefs::kSafeBrowsingEnabled, false);

  // Setup main browser
  _browserList = BrowserListFactory::GetForProfile(profile);
  _browser = Browser::Create(_main_profile, {});
  _browserList->AddBrowser(_browser.get());
  [self setSessionIDForBrowser:_browser.get()];

  // Setup inactive browser
  auto* inactiveBrowser = _browser->CreateInactiveBrowser();
  _browserList->AddBrowser(inactiveBrowser);
  [self setSessionIDForBrowser:inactiveBrowser];

  // Setup otr browser
  ProfileIOS* otr_profile = profile->GetOffTheRecordProfile();
  _otr_browserList = BrowserListFactory::GetForProfile(otr_profile);
  _otr_browser = Browser::Create(otr_profile, {});
  _otr_browserList->AddBrowser(_otr_browser.get());
  [self setSessionIDForBrowser:_otr_browser.get()];

  // Setup download managers for CWVWebView
  _downloadManager =
      std::make_unique<ios_web_view::WebViewDownloadManager>(profile);
  _otrDownloadManager =
      std::make_unique<ios_web_view::WebViewDownloadManager>(otr_profile);

#if BUILDFLAG(IOS_CREDENTIAL_PROVIDER_ENABLED)
  CredentialProviderServiceFactory::GetForProfile(profile);
#endif
}

// Configures the BrowserAgent with the session identifier for `browser`.
- (void)setSessionIDForBrowser:(Browser*)browser {
  const std::string identifier = session_util::GetSessionIdentifier(browser);

  //  SnapshotBrowserAgent::FromBrowser(browser)->SetSessionID(identifier);

  ProfileIOS* profile = browser->GetProfile();
  SessionRestorationServiceFactory::GetForProfile(profile)->SetSessionID(
      browser, identifier);
}

- (void)restoreSession {
  Browser* inactiveBrowser = _browser->GetInactiveBrowser();

  // loadSessionForBrowser
  SessionRestorationServiceFactory::GetForProfile(_browser->GetProfile())
      ->LoadSession(_browser.get());
  SessionRestorationServiceFactory::GetForProfile(inactiveBrowser->GetProfile())
      ->LoadSession(inactiveBrowser);
  SessionRestorationServiceFactory::GetForProfile(_otr_browser->GetProfile())
      ->LoadSession(_otr_browser.get());

  // https://source.chromium.org/chromium/chromium/src/+/main:ios/chrome/browser/main/ui_bundled/browser_view_wrangler.mm
  // MoveTabsFromInactiveToActive(inactiveBrowser, _mainBrowser.get());
  // MoveTabsFromActiveToInactive(_mainBrowser.get(), inactiveBrowser);

  RestoreAllInactiveTabs(inactiveBrowser, _browser.get());
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
        [[BraveTabGeneratorAPI alloc] initWithBrowser:_browser.get()
                                           otrBrowser:_otr_browser.get()];
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

- (HTTPSUpgradeExceptionsService*)httpsUpgradeExceptionsService {
  if (!_httpsUpgradeExceptionsService) {
    _httpsUpgradeExceptionsService =
        [[HTTPSUpgradeExceptionsService alloc] init];
  }
  return _httpsUpgradeExceptionsService;
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
                      installationDate:(NSDate*)installDate {
#if BUILDFLAG(BRAVE_P3A_ENABLED)
  _p3a_service = base::MakeRefCounted<p3a::P3AService>(
      *GetApplicationContext()->GetLocalState(),
      base::SysNSStringToUTF8(channel), base::Time::FromNSDate(installDate),
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

- (AIChat*)aiChatAPIWithDelegate:(id<AIChatDelegate>)delegate {
  return [[AIChat alloc] initWithProfileIOS:_main_profile delegate:delegate];
}

+ (bool)initializeICUForTesting {
  NSBundle* bundle = [NSBundle bundleForClass:self];
  base::apple::SetOverrideOuterBundle(bundle);
  base::apple::SetOverrideFrameworkBundle(bundle);
  return base::i18n::InitializeICU();
}

+ (void)initializeResourceBundleForTesting {
  @autoreleasepool {
    ios::RegisterPathProvider();
    ui::RegisterPathProvider();
  }

  base::AtExitManager exit_manager;
  base::CommandLine::Init(0, nullptr);

  [BraveCoreMain initializeICUForTesting];

  NSBundle* baseBundle = base::apple::OuterBundle();
  base::apple::SetBaseBundleID(
      base::SysNSStringToUTF8([baseBundle bundleIdentifier]).c_str());

  // Register all providers before calling any Chromium code.
  [ProviderRegistration registerProviders];

  ui::ResourceBundle::InitSharedInstanceWithLocale(
      "en-US", nullptr, ui::ResourceBundle::LOAD_COMMON_RESOURCES);

  // Add Brave Resource Pack
  base::FilePath brave_pack_path;
  base::PathService::Get(base::DIR_ASSETS, &brave_pack_path);
  brave_pack_path = brave_pack_path.AppendASCII("brave_resources.pak");
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
      brave_pack_path, ui::kScaleFactorNone);
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
        ios::HostContentSettingsMapFactory::GetForProfile(_main_profile);
    _defaultHostContentSettings =
        [[DefaultHostContentSettings alloc] initWithSettingsMap:map];
  }
  return _defaultHostContentSettings;
}

- (CWVWebViewConfiguration*)defaultWebViewConfiguration {
  if (!_defaultWebViewConfiguration) {
    _defaultWebViewConfiguration = [[CWVWebViewConfiguration alloc]
        initWithBrowserState:ios_web_view::WebViewBrowserState::
                                 FromBrowserState(_main_profile)];
  }
  return _defaultWebViewConfiguration;
}

- (CWVWebViewConfiguration*)nonPersistentWebViewConfiguration {
  if (!_nonPersistentWebViewConfiguration) {
    _nonPersistentWebViewConfiguration = [[CWVWebViewConfiguration alloc]
        initWithBrowserState:ios_web_view::WebViewBrowserState::
                                 FromBrowserState(
                                     _main_profile->GetOffTheRecordProfile())];
  }
  return _nonPersistentWebViewConfiguration;
}

#pragma mark - Handling of destroying the incognito BrowserState

// The incognito BrowserState should be closed when the last incognito tab is
// closed (i.e. if there are other incognito tabs open in another Scene, the
// BrowserState must not be destroyed).
- (BOOL)shouldDestroyAndRebuildIncognitoProfile {
  return _main_profile->HasOffTheRecordProfile();
}

// Matches lastIncognitoTabClosed from Chrome's SceneController
- (void)notifyLastPrivateTabClosed {
  // If no other window has incognito tab, then destroy and rebuild the
  // BrowserState. Otherwise, just do the state transition animation.
  if ([self shouldDestroyAndRebuildIncognitoProfile]) {
    // Incognito browser state cannot be deleted before all the requests are
    // deleted. Queue empty task on IO thread and destroy the BrowserState
    // when the task has executed, again verifying that no incognito tabs are
    // present. When an incognito tab is moved between browsers, there is
    // a point where the tab isn't attached to any web state list. However, when
    // this queued cleanup step executes, the moved tab will be attached, so
    // the cleanup shouldn't proceed.

    auto cleanup = ^{
      if ([self shouldDestroyAndRebuildIncognitoProfile]) {
        [self destroyAndRebuildIncognitoProfile];
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

  // Stop serializing the state of `browser`.
  SessionRestorationServiceFactory::GetForProfile(profile)->Disconnect(browser);

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

- (void)destroyAndRebuildIncognitoProfile {
  DCHECK(_main_profile->HasOffTheRecordProfile());
  _nonPersistentWebViewConfiguration = nil;

  ProfileIOS* otrProfile = _main_profile->GetOffTheRecordProfile();

  BrowsingDataRemover* browsingDataRemover =
      BrowsingDataRemoverFactory::GetForProfile(otrProfile);
  browsingDataRemover->Remove(browsing_data::TimePeriod::ALL_TIME,
                              BrowsingDataRemoveMask::REMOVE_ALL,
                              base::DoNothing());

  [self cleanupBrowser:_otr_browser.get()];
  _otr_browser.reset();

  // Destroy and recreate the off-the-record BrowserState.
  _main_profile->DestroyOffTheRecordProfile();

  otrProfile = _main_profile->GetOffTheRecordProfile();
  _otr_browser = Browser::Create(otrProfile, {});

  BrowserList* browserList = BrowserListFactory::GetForProfile(otrProfile);
  browserList->AddBrowser(_otr_browser.get());
}

+ (Class)sceneDelegateClass {
  return [SceneDelegate class];
}

- (NSObject*)getInternalAppState {
  return self.appState;
}
@end

@implementation BraveCoreMain (AppDelegate)
- (BOOL)application:(UIApplication*)application
    didFinishLaunchingWithOptions:(NSDictionary*)launchOptions {
  // Initialize Chromium
  _appState.startupInformation.didFinishLaunchingTime = base::TimeTicks::Now();
  [_appState startInitialization];

  // Setup Chromium Notifications
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(sceneWillConnect:)
             name:UISceneWillConnectNotification
           object:nil];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(lastSceneWillEnterBackground:)
             name:UIApplicationWillResignActiveNotification
           object:nil];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(lastSceneDidEnterBackground:)
             name:UIApplicationDidEnterBackgroundNotification
           object:nil];

  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(firstSceneWillEnterForeground:)
             name:UIApplicationWillEnterForegroundNotification
           object:nil];
  
//  Browser* mainBrowser =
//        self.sceneState.browserProviderInterface.mainBrowserProvider.browser

  //  // Setup Profiles
//  ProfileIOS* profile = brave::CreateMainProfileIOS();
//  [self profileLoaded:profile];
//
//  // Initialize the provider UI global state.
//  ios::provider::InitializeUI();
//
//  // Setup WebUI (Sync Internals and other WebViews)
//  web::WebUIIOSControllerFactory::RegisterFactory(
//      BraveWebUIControllerFactory::GetInstance());
//
//  // TODO(darkdh): move _adblockService and _backgroundImageService to
//  // BraveWebMainParts::PreMainMessageLoopRun
//  // https://github.com/brave/brave-browser/issues/40567
//  component_updater::ComponentUpdateService* cus =
//      GetApplicationContext()->GetComponentUpdateService();
//
//  _adblockService = [[AdblockService alloc] initWithComponentUpdater:cus];
//
//  _backgroundImagesService = [[NTPBackgroundImagesService alloc]
//      initWithBackgroundImagesService:
//          std::make_unique<ntp_background_images::NTPBackgroundImagesService>(
//              cus, GetApplicationContext()->GetLocalState())];

  return YES;
}

- (void)applicationWillTerminate:(UIApplication*)application {
  base::ios::ScopedCriticalAction::ApplicationWillTerminate();

  // If `self.didFinishLaunching` is NO, that indicates that the app was
  // terminated before startup could be run. In this situation, skip running
  // shutdown, since the app was never fully started.
  if (!self.didFinishLaunching) {
    return;
  }

  if (_appState.initStage <= AppInitStage::kSafeMode) {
    return;
  }

  // Instead of adding code here, consider if it could be handled by listening
  // for  UIApplicationWillterminate.
  [_mainController applicationWillTerminate:application];
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application {
  if (_appState.initStage <= AppInitStage::kSafeMode) {
    return;
  }

  [_memoryHelper handleMemoryPressure];
}

- (void)application:(UIApplication*)application
    didDiscardSceneSessions:(NSSet<UISceneSession*>*)sceneSessions {
  [_mainController application:application
       didDiscardSceneSessions:sceneSessions];
}

- (void)application:(UIApplication*)application
    handleEventsForBackgroundURLSession:(NSString*)identifier
                      completionHandler:(void (^)())completionHandler {
  if (![identifier
          hasPrefix:base::SysUTF8ToNSString(
                        download::kBackgroundDownloadIdentifierPrefix)]) {
    completionHandler();
    return;
  }
  // TODO(crbug.com/325613461) Remove this Browser dependency, ideally by
  // refactoring into a dedicated agent.
  Browser* browser = _mainController.browserProviderInterfaceDoNotUse
                         .mainBrowserProvider.browser;
  if (!browser) {
    // TODO(crbug.com/40240359): We should store the completionHandler and wait
    // for mainBrowserProvider creation.
    completionHandler();
    return;
  }
  // TODO(crbug.com/325613461): Associate downloads with a specific file path to
  // determine which profile / download service to use here.
  download::BackgroundDownloadService* download_service =
      BackgroundDownloadServiceFactory::GetForProfile(browser->GetProfile());
  if (download_service) {
    download_service->HandleEventsForBackgroundURLSession(
        base::BindOnce(completionHandler));
    return;
  }
  completionHandler();
}

#pragma mark - Scenes lifecycle

- (NSInteger)foregroundSceneCount {
  NSInteger foregroundSceneCount = 0;
  for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
    if ((scene.activationState == UISceneActivationStateForegroundInactive) ||
        (scene.activationState == UISceneActivationStateForegroundActive)) {
      foregroundSceneCount++;
    }
  }
  return foregroundSceneCount;
}

- (void)sceneWillConnect:(NSNotification*)notification {
  UIWindowScene* scene =
      base::apple::ObjCCastStrict<UIWindowScene>(notification.object);
  id<UISceneDelegate> sceneDelegate = scene.delegate;

  // Under some iOS 15 betas, Chrome gets scene connection events for some
  // system scene connections. To handle this, early return if the connecting
  // scene doesn't have a valid delegate. (See crbug.com/1217461)
  if (!sceneDelegate ||
      ![sceneDelegate conformsToProtocol:@protocol(UIWindowSceneDelegate)]) {
    return;
  }

  // TODO(crbug.com/40679152): This should be called later, or this flow should
  // be changed completely.
  if (self.foregroundSceneCount == 0) {
    [_mainController
        applicationWillEnterForeground:UIApplication.sharedApplication
                          memoryHelper:_memoryHelper];
  }
}

- (void)lastSceneWillEnterBackground:(NSNotification*)notification {
  [_mainController applicationWillResignActive:UIApplication.sharedApplication];
}

- (void)lastSceneDidEnterBackground:(NSNotification*)notification {
  [_mainController applicationDidEnterBackground:UIApplication.sharedApplication
                                    memoryHelper:_memoryHelper];
}

- (void)firstSceneWillEnterForeground:(NSNotification*)notification {
  // This method may be invoked really early in the application lifetime
  // even before the creation of the main loop. Thus it is not possible
  // to use PostTask API here, and we have to use dispatch_async(...).
  __weak BraveCoreMain* weakSelf = self;
  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW, kMainIntentCheckDelay.InNanoseconds()),
      dispatch_get_main_queue(), ^{
        [weakSelf firstSceneDidEnterForeground];
      });

  [_mainController
      applicationWillEnterForeground:UIApplication.sharedApplication
                        memoryHelper:_memoryHelper];
}

#pragma mark - Private

// Returns whether the application was started via an external intent or
// directly (i.e. by tapping on the app button directly).
- (BOOL)appStartupFromExternalIntent {
  for (SceneState* scene in self.appState.connectedScenes) {
    if (scene.startupHadExternalIntent) {
      return YES;
    }
  }

  return NO;
}

// Invoked on the main sequence after -firstSceneWillEnterForeground: is
// called, after a small delay. The delay is there to give time for the
// intents to be received by the application (as they are not guaranteed
// to happen before -firstSceneWillEnterForeground:).
- (void)firstSceneDidEnterForeground {
}
@end
