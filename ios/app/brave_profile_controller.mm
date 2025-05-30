// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/app/brave_profile_controller.h"

#include "base/memory/raw_ptr.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/ios/browser/api/ai_chat/ai_chat+private.h"
#include "brave/ios/browser/api/ai_chat/ai_chat_delegate.h"
#include "brave/ios/browser/api/bookmarks/brave_bookmarks_api+private.h"
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
#include "brave/ios/browser/api/password/brave_password_api+private.h"
#include "brave/ios/browser/api/sync/brave_sync_api+private.h"
#include "brave/ios/browser/api/sync/driver/brave_sync_profile_service+private.h"
#include "brave/ios/browser/api/web_image/web_image+private.h"
#include "brave/ios/browser/application_context/brave_application_context_impl.h"
#include "brave/ios/browser/brave_ads/ads_service_factory_ios.h"
#include "brave/ios/browser/brave_ads/ads_service_impl_ios.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_store/password_store.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/send_tab_to_self/send_tab_to_self_sync_service.h"
#include "components/sync/base/features.h"
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
#include "ios/chrome/browser/shared/model/prefs/pref_names.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"
#include "ios/chrome/browser/shared/model/profile/scoped_profile_keep_alive_ios.h"
#include "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#include "ios/chrome/browser/shared/public/commands/command_dispatcher.h"
#include "ios/chrome/browser/sync/model/send_tab_to_self_sync_service_factory.h"
#include "ios/chrome/browser/sync/model/session_sync_service_factory.h"
#include "ios/chrome/browser/sync/model/sync_service_factory.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "ios/web_view/internal/cwv_web_view_configuration_internal.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#include "ios/web_view/internal/web_view_download_manager.h"

#if BUILDFLAG(IOS_CREDENTIAL_PROVIDER_ENABLED)
#include "ios/chrome/browser/credential_provider/model/credential_provider_service_factory.h"
#include "ios/chrome/browser/credential_provider/model/credential_provider_util.h"
#endif

@interface BraveProfileController () {
  ScopedProfileKeepAliveIOS _profileKeepAlive;
  raw_ptr<ProfileIOS> _profile;
  std::unique_ptr<Browser> _browser;
  std::unique_ptr<Browser> _otr_browser;
  raw_ptr<BrowserList> _browserList;
  raw_ptr<BrowserList> _otr_browserList;
  std::unique_ptr<ios_web_view::WebViewDownloadManager> _downloadManager;
  std::unique_ptr<ios_web_view::WebViewDownloadManager> _otrDownloadManager;
}
@property(nonatomic) BraveBookmarksAPI* bookmarksAPI;
@property(nonatomic) BraveHistoryAPI* historyAPI;
@property(nonatomic) BravePasswordAPI* passwordAPI;
@property(nonatomic) BraveOpenTabsAPI* openTabsAPI;
@property(nonatomic) BraveSendTabAPI* sendTabAPI;
@property(nonatomic) BraveSyncAPI* syncAPI;
@property(nonatomic) BraveSyncProfileServiceIOS* syncProfileService;
@property(nonatomic) BraveTabGeneratorAPI* tabGeneratorAPI;
@property(nonatomic) BraveWalletAPI* braveWalletAPI;
@property(nonatomic) DeAmpPrefs* deAmpPrefs;
@property(nonatomic) IpfsAPIImpl* ipfsAPI;
@property(nonatomic) WebImageDownloader* webImageDownloader;
@property(nonatomic) NTPBackgroundImagesService* backgroundImagesService;
@property(nonatomic) DefaultHostContentSettings* defaultHostContentSettings;
@property(nonatomic) CWVWebViewConfiguration* defaultWebViewConfiguration;
@property(nonatomic) CWVWebViewConfiguration* nonPersistentWebViewConfiguration;
@end

@implementation BraveProfileController

- (instancetype)initWithProfileKeepAlive:
    (ScopedProfileKeepAliveIOS)profileKeepAlive {
  if ((self = [super init])) {
    _profileKeepAlive = std::move(profileKeepAlive);
    _profile = _profileKeepAlive.profile();

    // Disable Safe-Browsing via Prefs
    _profile->GetPrefs()->SetBoolean(prefs::kSafeBrowsingEnabled, false);

    // Setup main browser
    _browserList = BrowserListFactory::GetForProfile(_profile);
    _browser = Browser::Create(_profile, {});
    _browserList->AddBrowser(_browser.get());

    // Setup otr browser
    ProfileIOS* otr_last_used_profile = _profile->GetOffTheRecordProfile();
    _otr_browserList = BrowserListFactory::GetForProfile(otr_last_used_profile);
    _otr_browser = Browser::Create(otr_last_used_profile, {});
    _otr_browserList->AddBrowser(_otr_browser.get());

#if BUILDFLAG(IOS_CREDENTIAL_PROVIDER_ENABLED)
    [self performFaviconsCleanup];
    CredentialProviderServiceFactory::GetForProfile(_profile);
#endif

    // Setup download managers for CWVWebView
    _downloadManager =
        std::make_unique<ios_web_view::WebViewDownloadManager>(_profile);
    _otrDownloadManager =
        std::make_unique<ios_web_view::WebViewDownloadManager>(
            _profile->GetOffTheRecordProfile());

    _backgroundImagesService = [[NTPBackgroundImagesService alloc]
        initWithBackgroundImagesService:
            std::make_unique<ntp_background_images::NTPBackgroundImagesService>(
                GetApplicationContext()->GetComponentUpdateService(),
                GetApplicationContext()->GetLocalState())
                            ads_service:brave_ads::AdsServiceFactoryIOS::
                                            GetForProfile(_profile)];
  }
  return self;
}

- (void)dealloc {
  [_nonPersistentWebViewConfiguration shutDown];
  [_defaultWebViewConfiguration shutDown];

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

  _profile = nil;
  _profileKeepAlive.Reset();
}

#if BUILDFLAG(IOS_CREDENTIAL_PROVIDER_ENABLED)
- (void)performFaviconsCleanup {
  ProfileIOS* browserState = _profile;
  if (!browserState) {
    return;
  }
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&UpdateFaviconsStorageForProfile,
                                browserState->AsWeakPtr(),
                                /*fallback_to_google_server=*/false));
}
#endif

- (BraveBookmarksAPI*)bookmarksAPI {
  if (!_bookmarksAPI) {
    ProfileIOS* profile = ProfileIOS::FromBrowserState(_profile);
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
    _historyAPI = [[BraveHistoryAPI alloc] initWithBrowserState:_profile];
  }
  return _historyAPI;
}

- (BraveOpenTabsAPI*)openTabsAPI {
  if (!_openTabsAPI) {
    syncer::SyncService* sync_service_ =
        SyncServiceFactory::GetForProfile(_profile);

    sync_sessions::SessionSyncService* session_sync_service_ =
        SessionSyncServiceFactory::GetForProfile(_profile);

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
            _profile, ServiceAccessType::EXPLICIT_ACCESS)
            .get();

    _passwordAPI =
        [[BravePasswordAPI alloc] initWithPasswordStore:password_store_];
  }
  return _passwordAPI;
}

- (BraveSendTabAPI*)sendTabAPI {
  if (!_sendTabAPI) {
    send_tab_to_self::SendTabToSelfSyncService* sync_service_ =
        SendTabToSelfSyncServiceFactory::GetForProfile(_profile);

    _sendTabAPI = [[BraveSendTabAPI alloc] initWithSyncService:sync_service_];
  }
  return _sendTabAPI;
}

- (BraveSyncAPI*)syncAPI {
  if (!_syncAPI) {
    _syncAPI = [[BraveSyncAPI alloc] initWithBrowserState:_profile];
  }
  return _syncAPI;
}

- (BraveSyncProfileServiceIOS*)syncProfileService {
  if (!_syncProfileService) {
    syncer::SyncService* sync_service_ =
        SyncServiceFactory::GetForProfile(_profile);
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

- (BraveStats*)braveStats {
  return [[BraveStats alloc] initWithBrowserState:_profile];
}

- (id<IpfsAPI>)ipfsAPI {
  if (!_ipfsAPI) {
    _ipfsAPI = [[IpfsAPIImpl alloc] initWithBrowserState:_profile];
  }
  return _ipfsAPI;
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
    _braveWalletAPI = [[BraveWalletAPI alloc] initWithBrowserState:_profile];
  }
  return _braveWalletAPI;
}

- (DeAmpPrefs*)deAmpPrefs {
  if (!_deAmpPrefs) {
    _deAmpPrefs =
        [[DeAmpPrefs alloc] initWithProfileState:_profile->GetPrefs()];
  }
  return _deAmpPrefs;
}

- (AIChat*)aiChatAPIWithDelegate:(id<AIChatDelegate>)delegate {
  return [[AIChat alloc] initWithProfileIOS:_profile delegate:delegate];
}

- (DefaultHostContentSettings*)defaultHostContentSettings {
  if (!_defaultHostContentSettings) {
    HostContentSettingsMap* map =
        ios::HostContentSettingsMapFactory::GetForProfile(_profile);
    _defaultHostContentSettings =
        [[DefaultHostContentSettings alloc] initWithSettingsMap:map];
  }
  return _defaultHostContentSettings;
}

- (CWVWebViewConfiguration*)defaultWebViewConfiguration {
  if (!_defaultWebViewConfiguration) {
    _defaultWebViewConfiguration = [[CWVWebViewConfiguration alloc]
        initWithBrowserState:ios_web_view::WebViewBrowserState::
                                 FromBrowserState(_profile)];
  }
  return _defaultWebViewConfiguration;
}

- (CWVWebViewConfiguration*)nonPersistentWebViewConfiguration {
  if (!_nonPersistentWebViewConfiguration) {
    _nonPersistentWebViewConfiguration = [[CWVWebViewConfiguration alloc]
        initWithBrowserState:ios_web_view::WebViewBrowserState::
                                 FromBrowserState(
                                     _profile->GetOffTheRecordProfile())];
  }
  return _nonPersistentWebViewConfiguration;
}

#pragma mark - Handling of destroying the incognito BrowserState

// The incognito BrowserState should be closed when the last incognito tab is
// closed (i.e. if there are other incognito tabs open in another Scene, the
// BrowserState must not be destroyed).
- (BOOL)shouldDestroyAndRebuildIncognitoProfile {
  return _profile->HasOffTheRecordProfile();
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
  DCHECK(_profile->HasOffTheRecordProfile());
  _nonPersistentWebViewConfiguration = nil;

  ProfileIOS* otrProfile = _profile->GetOffTheRecordProfile();

  BrowsingDataRemover* browsingDataRemover =
      BrowsingDataRemoverFactory::GetForProfile(otrProfile);
  browsingDataRemover->Remove(browsing_data::TimePeriod::ALL_TIME,
                              BrowsingDataRemoveMask::REMOVE_ALL,
                              base::DoNothing());

  [self cleanupBrowser:_otr_browser.get()];
  _otr_browser.reset();

  // Destroy and recreate the off-the-record BrowserState.
  _profile->DestroyOffTheRecordProfile();

  otrProfile = _profile->GetOffTheRecordProfile();
  _otr_browser = Browser::Create(otrProfile, {});

  BrowserList* browserList = BrowserListFactory::GetForProfile(otrProfile);
  browserList->AddBrowser(_otr_browser.get());
}

@end
