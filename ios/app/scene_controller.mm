// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "brave/ios/app/scene_controller.h"

#import "base/feature_list.h"
#import "base/functional/callback_helpers.h"
#import "base/i18n/message_formatter.h"
#import "base/ios/ios_util.h"
#import "base/logging.h"
#import "base/metrics/histogram_functions.h"
#import "base/metrics/histogram_macros.h"
#import "base/metrics/user_metrics.h"
#import "base/metrics/user_metrics_action.h"
#import "base/strings/sys_string_conversions.h"
#import "base/time/time.h"
#import "components/autofill/core/browser/data_model/autofill_profile.h"
#import "components/autofill/core/browser/data_model/credit_card.h"
#import "components/breadcrumbs/core/breadcrumbs_status.h"
#import "components/feature_engagement/public/event_constants.h"
#import "components/feature_engagement/public/tracker.h"
#import "components/infobars/core/infobar_manager.h"
#import "components/password_manager/core/browser/ui/credential_ui_entry.h"
#import "components/password_manager/core/browser/ui/password_check_referrer.h"
#import "components/policy/core/common/cloud/user_cloud_policy_manager.h"
#import "components/prefs/pref_service.h"
#import "components/previous_session_info/previous_session_info.h"
#import "components/signin/public/base/signin_metrics.h"
#import "components/signin/public/base/signin_pref_names.h"
#import "components/signin/public/identity_manager/identity_manager.h"
#import "components/supervised_user/core/browser/kids_management_api_fetcher.h"
#import "components/supervised_user/core/browser/proto/kidsmanagement_messages.pb.h"
#import "components/supervised_user/core/browser/proto_fetcher_status.h"
#import "components/supervised_user/core/browser/supervised_user_utils.h"
#import "components/url_formatter/url_formatter.h"
#import "components/version_info/version_info.h"
#import "components/web_resource/web_resource_pref_names.h"
#import "ios/chrome/app/application_delegate/app_state.h"
#import "ios/chrome/app/application_delegate/startup_information.h"
#import "ios/chrome/app/application_delegate/url_opener.h"
#import "ios/chrome/app/application_delegate/url_opener_params.h"
#import "ios/chrome/app/application_mode.h"
#import "ios/chrome/app/chrome_overlay_window.h"
#import "ios/chrome/app/deferred_initialization_runner.h"
#import "ios/chrome/app/deferred_initialization_task_names.h"
#import "ios/chrome/app/profile/profile_state.h"
#import "ios/chrome/app/profile/profile_state_observer.h"
#import "ios/chrome/app/tests_hook.h"
#import "ios/chrome/browser/ai_prototyping/coordinator/ai_prototyping_coordinator.h"
#import "ios/chrome/browser/app_store_rating/ui_bundled/app_store_rating_scene_agent.h"
#import "ios/chrome/browser/app_store_rating/ui_bundled/features.h"
#import "ios/chrome/browser/appearance/ui_bundled/appearance_customization.h"
#import "ios/chrome/browser/authentication/ui_bundled/account_menu/account_menu_coordinator.h"
#import "ios/chrome/browser/authentication/ui_bundled/signin/signin_constants.h"
#import "ios/chrome/browser/authentication/ui_bundled/signin/signin_coordinator.h"
#import "ios/chrome/browser/authentication/ui_bundled/signin/signin_utils.h"
#import "ios/chrome/browser/authentication/ui_bundled/signin_notification_infobar_delegate.h"
#import "ios/chrome/browser/browser_view/ui_bundled/browser_view_controller.h"
#import "ios/chrome/browser/browsing_data/model/browsing_data_remove_mask.h"
#import "ios/chrome/browser/browsing_data/model/browsing_data_remover.h"
#import "ios/chrome/browser/browsing_data/model/browsing_data_remover_factory.h"
#import "ios/chrome/browser/crash_report/model/breadcrumbs/breadcrumb_manager_browser_agent.h"
#import "ios/chrome/browser/crash_report/model/crash_keys_helper.h"
#import "ios/chrome/browser/crash_report/model/crash_loop_detection_util.h"
#import "ios/chrome/browser/crash_report/model/crash_report_helper.h"
#import "ios/chrome/browser/credential_provider_promo/ui_bundled/credential_provider_promo_scene_agent.h"
#import "ios/chrome/browser/default_browser/model/default_browser_interest_signals.h"
#import "ios/chrome/browser/default_browser/model/promo_source.h"
#import "ios/chrome/browser/default_browser/model/utils.h"
#import "ios/chrome/browser/enterprise/model/idle/idle_service.h"
#import "ios/chrome/browser/enterprise/model/idle/idle_service_factory.h"
#import "ios/chrome/browser/feature_engagement/model/tracker_factory.h"
#import "ios/chrome/browser/first_run/model/first_run.h"
#import "ios/chrome/browser/geolocation/model/geolocation_manager.h"
#import "ios/chrome/browser/history/ui_bundled/history_coordinator.h"
#import "ios/chrome/browser/history/ui_bundled/history_coordinator_delegate.h"
#import "ios/chrome/browser/incognito_interstitial/ui_bundled/incognito_interstitial_coordinator.h"
#import "ios/chrome/browser/incognito_interstitial/ui_bundled/incognito_interstitial_coordinator_delegate.h"
#import "ios/chrome/browser/incognito_reauth/ui_bundled/incognito_reauth_scene_agent.h"
#import "ios/chrome/browser/infobars/model/infobar_manager_impl.h"
#import "ios/chrome/browser/intents/user_activity_browser_agent.h"
#import "ios/chrome/browser/lens/ui_bundled/lens_entrypoint.h"
#import "ios/chrome/browser/lens_overlay/coordinator/lens_overlay_availability.h"
#import "ios/chrome/browser/lens_overlay/model/lens_overlay_tab_helper.h"
#import "ios/chrome/browser/mailto_handler/model/mailto_handler_service.h"
#import "ios/chrome/browser/mailto_handler/model/mailto_handler_service_factory.h"
#import "ios/chrome/browser/main/ui_bundled/browser_view_wrangler.h"
#import "ios/chrome/browser/main/ui_bundled/default_browser_promo_scene_agent.h"
#import "ios/chrome/browser/main/ui_bundled/incognito_blocker_scene_agent.h"
#import "ios/chrome/browser/main/ui_bundled/ui_blocker_scene_agent.h"
#import "ios/chrome/browser/main/ui_bundled/wrangled_browser.h"
#import "ios/chrome/browser/metrics/model/tab_usage_recorder_browser_agent.h"
#import "ios/chrome/browser/ntp/model/new_tab_page_tab_helper.h"
#import "ios/chrome/browser/ntp/ui_bundled/new_tab_page_feature.h"
#import "ios/chrome/browser/passwords/model/ios_chrome_password_check_manager.h"
#import "ios/chrome/browser/passwords/model/ios_chrome_password_check_manager_factory.h"
#import "ios/chrome/browser/passwords/model/password_checkup_utils.h"
#import "ios/chrome/browser/policy/model/cloud/user_policy_signin_service_factory.h"
#import "ios/chrome/browser/policy/model/policy_util.h"
#import "ios/chrome/browser/policy/model/policy_watcher_browser_agent.h"
#import "ios/chrome/browser/policy/model/policy_watcher_browser_agent_observer_bridge.h"
#import "ios/chrome/browser/policy/ui_bundled/idle/idle_timeout_policy_scene_agent.h"
#import "ios/chrome/browser/policy/ui_bundled/signin_policy_scene_agent.h"
#import "ios/chrome/browser/policy/ui_bundled/user_policy_scene_agent.h"
#import "ios/chrome/browser/policy/ui_bundled/user_policy_util.h"
#import "ios/chrome/browser/promos_manager/model/features.h"
#import "ios/chrome/browser/promos_manager/model/promos_manager_factory.h"
#import "ios/chrome/browser/promos_manager/ui_bundled/promos_manager_scene_agent.h"
#import "ios/chrome/browser/promos_manager/ui_bundled/utils.h"
#import "ios/chrome/browser/reading_list/model/reading_list_browser_agent.h"
#import "ios/chrome/browser/scoped_ui_blocker/ui_bundled/scoped_ui_blocker.h"
#import "ios/chrome/browser/screenshot/model/screenshot_delegate.h"
#import "ios/chrome/browser/sessions/model/session_restoration_service.h"
#import "ios/chrome/browser/sessions/model/session_restoration_service_factory.h"
#import "ios/chrome/browser/sessions/model/session_saving_scene_agent.h"
#import "ios/chrome/browser/settings/ui_bundled/clear_browsing_data/features.h"
#import "ios/chrome/browser/settings/ui_bundled/password/password_checkup/password_checkup_coordinator.h"
#import "ios/chrome/browser/settings/ui_bundled/password/passwords_coordinator.h"
#import "ios/chrome/browser/settings/ui_bundled/password/passwords_mediator.h"
#import "ios/chrome/browser/settings/ui_bundled/settings_navigation_controller.h"
#import "ios/chrome/browser/settings/ui_bundled/utils/password_utils.h"
#import "ios/chrome/browser/shared/coordinator/default_browser_promo/non_modal_default_browser_promo_scheduler_scene_agent.h"
#import "ios/chrome/browser/shared/coordinator/layout_guide/layout_guide_scene_agent.h"
#import "ios/chrome/browser/shared/coordinator/scene/scene_ui_provider.h"
#import "ios/chrome/browser/shared/model/application_context/application_context.h"
#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/model/browser/browser_list.h"
#import "ios/chrome/browser/shared/model/browser/browser_list_factory.h"
#import "ios/chrome/browser/shared/model/browser/browser_provider_interface.h"
#import "ios/chrome/browser/shared/model/prefs/pref_names.h"
#import "ios/chrome/browser/shared/model/profile/profile_attributes_storage_ios.h"
#import "ios/chrome/browser/shared/model/profile/profile_ios.h"
#import "ios/chrome/browser/shared/model/profile/profile_manager_ios.h"
#import "ios/chrome/browser/shared/model/url/chrome_url_constants.h"
#import "ios/chrome/browser/shared/model/url/url_util.h"
#import "ios/chrome/browser/shared/model/web_state_list/browser_util.h"
#import "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#import "ios/chrome/browser/shared/model/web_state_list/web_state_list_observer_bridge.h"
#import "ios/chrome/browser/shared/model/web_state_list/web_state_opener.h"
#import "ios/chrome/browser/shared/public/commands/application_commands.h"
#import "ios/chrome/browser/shared/public/commands/bookmarks_commands.h"
#import "ios/chrome/browser/shared/public/commands/browser_commands.h"
#import "ios/chrome/browser/shared/public/commands/browser_coordinator_commands.h"
#import "ios/chrome/browser/shared/public/commands/command_dispatcher.h"
#import "ios/chrome/browser/shared/public/commands/lens_commands.h"
#import "ios/chrome/browser/shared/public/commands/omnibox_commands.h"
#import "ios/chrome/browser/shared/public/commands/open_lens_input_selection_command.h"
#import "ios/chrome/browser/shared/public/commands/open_new_tab_command.h"
#import "ios/chrome/browser/shared/public/commands/policy_change_commands.h"
#import "ios/chrome/browser/shared/public/commands/qr_scanner_commands.h"
#import "ios/chrome/browser/shared/public/commands/quick_delete_commands.h"
#import "ios/chrome/browser/shared/public/commands/show_signin_command.h"
#import "ios/chrome/browser/shared/public/commands/snackbar_commands.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/chrome/browser/shared/ui/util/snackbar_util.h"
#import "ios/chrome/browser/shared/ui/util/top_view_controller.h"
#import "ios/chrome/browser/shared/ui/util/uikit_ui_util.h"
#import "ios/chrome/browser/signin/model/authentication_service.h"
#import "ios/chrome/browser/signin/model/authentication_service_factory.h"
#import "ios/chrome/browser/signin/model/constants.h"
#import "ios/chrome/browser/signin/model/identity_manager_factory.h"
#import "ios/chrome/browser/signin/model/system_identity_manager.h"
#import "ios/chrome/browser/snapshots/model/snapshot_tab_helper.h"
#import "ios/chrome/browser/start_surface/ui_bundled/start_surface_features.h"
#import "ios/chrome/browser/start_surface/ui_bundled/start_surface_recent_tab_browser_agent.h"
#import "ios/chrome/browser/start_surface/ui_bundled/start_surface_scene_agent.h"
#import "ios/chrome/browser/start_surface/ui_bundled/start_surface_util.h"
#import "ios/chrome/browser/tab_insertion/model/tab_insertion_browser_agent.h"
#import "ios/chrome/browser/tab_switcher/ui_bundled/tab_grid/tab_grid_coordinator.h"
#import "ios/chrome/browser/tab_switcher/ui_bundled/tab_grid/tab_grid_coordinator_delegate.h"
#import "ios/chrome/browser/tab_switcher/ui_bundled/tab_utils.h"
#import "ios/chrome/browser/ui/whats_new/promo/whats_new_scene_agent.h"
#import "ios/chrome/browser/url_loading/model/scene_url_loading_service.h"
#import "ios/chrome/browser/url_loading/model/url_loading_browser_agent.h"
#import "ios/chrome/browser/url_loading/model/url_loading_params.h"
#import "ios/chrome/browser/web/model/page_placeholder_browser_agent.h"
#import "ios/chrome/browser/web_state_list/model/session_metrics.h"
#import "ios/chrome/browser/web_state_list/model/web_usage_enabler/web_usage_enabler_browser_agent.h"
#import "ios/chrome/browser/window_activities/model/window_activity_helpers.h"
#import "ios/chrome/browser/youtube_incognito/coordinator/youtube_incognito_coordinator.h"
#import "ios/chrome/browser/youtube_incognito/coordinator/youtube_incognito_coordinator_delegate.h"
#import "ios/chrome/common/ui/reauthentication/reauthentication_module.h"
#import "ios/chrome/grit/ios_strings.h"
#import "ios/public/provider/chrome/browser/signin/choice_api.h"
#import "ios/public/provider/chrome/browser/ui_utils/ui_utils_api.h"
#import "ios/public/provider/chrome/browser/user_feedback/user_feedback_api.h"
#import "ios/public/provider/chrome/browser/user_feedback/user_feedback_data.h"
#import "ios/web/public/navigation/navigation_item.h"
#import "ios/web/public/navigation/navigation_manager.h"
#import "ios/web/public/navigation/navigation_util.h"
#import "ios/web/public/session/proto/storage.pb.h"
#import "ios/web/public/thread/web_task_traits.h"
#import "ios/web/public/thread/web_thread.h"
#import "ios/web/public/web_state.h"
#import "net/base/apple/url_conversions.h"
#import "services/network/public/cpp/shared_url_loader_factory.h"
#import "ui/base/l10n/l10n_util.h"

namespace {

// Killswitch, can be removed around February 2024. If enabled,
// createInitialUI will call makeKeyAndVisible before mainCoordinator start.
// When disabled, this fix resolves a flicker when starting the app in light
// mode
BASE_FEATURE(kMakeKeyAndVisibleBeforeMainCoordinatorStart,
             "MakeKeyAndVisibleBeforeMainCoordinatorStart",
             base::FEATURE_DISABLED_BY_DEFAULT);

// Feature to control whether Search Intents (Widgets, Application
// Shortcuts menu) forcibly open a new tab, rather than reusing an
// existing NTP. See http://crbug.com/1363375 for details.
BASE_FEATURE(kForceNewTabForIntentSearch,
             "ForceNewTabForIntentSearch",
             base::FEATURE_DISABLED_BY_DEFAULT);
}

@interface SceneController () <ProfileStateObserver,
                               WebStateListObserving> {
  
  std::unique_ptr<WebStateListObserverBridge> _webStateListForwardingObserver;
  
  std::unique_ptr<
      base::ScopedObservation<WebStateList, WebStateListObserverBridge>>
      _incognitoWebStateObserver;
  
  std::unique_ptr<
      base::ScopedObservation<WebStateList, WebStateListObserverBridge>>
      _mainWebStateObserver;

  std::unique_ptr<SceneUrlLoadingService> _sceneURLLoadingService;
}

@property(nonatomic, strong) TabGridCoordinator* mainCoordinator;

// YES while activating a new browser (often leading to dismissing the tab
// switcher.
@property(nonatomic, assign) BOOL activatingBrowser;

// YES if the scene has been backgrounded since it has last been
// SceneActivationLevelForegroundActive.
@property(nonatomic, assign) BOOL backgroundedSinceLastActivated;

// Wrangler to handle BVC and tab model creation, access, and related logic.
// Implements features exposed from this object through the
// BrowserViewInformation protocol.
@property(nonatomic, strong) BrowserViewWrangler* browserViewWrangler;

// The state of the scene controlled by this object.
@property(nonatomic, weak, readonly) SceneState* sceneState;
@end

@implementation SceneController
@synthesize startupParameters = _startupParameters;
@synthesize startupParametersAreBeingHandled =
    _startupParametersAreBeingHandled;

- (instancetype)initWithSceneState:(SceneState*)sceneState {
  self = [super init];
  if (self) {
    _sceneState = sceneState;
    [_sceneState addObserver:self];

//    _sceneURLLoadingService = std::make_unique<SceneUrlLoadingService>();
//    _sceneURLLoadingService->SetDelegate(self);

    _webStateListForwardingObserver =
        std::make_unique<WebStateListObserverBridge>(self);
  }
  return self;
}

- (void)setProfileState:(ProfileState*)profileState {
  DCHECK(!_sceneState.profileState);

  // Connect the ProfileState with the SceneState.
  _sceneState.profileState = profileState;
  [profileState sceneStateConnected:_sceneState];

  // Add agents. They may depend on the ProfileState, so they need to be
  // created after it has been connected to the SceneState.
  [_sceneState addAgent:[[SessionSavingSceneAgent alloc] init]];

  // Start observing the ProfileState. This needs to happen after the agents
  // as this may result in creation of the UI which can access to the agents.
  [profileState addObserver:self];
}

#pragma mark - Setters and getters

- (TabGridCoordinator*)mainCoordinator {
  if (!_mainCoordinator) {
    // Lazily create the main coordinator.
//    TabGridCoordinator* tabGridCoordinator = [[TabGridCoordinator alloc]
//        initWithApplicationCommandEndpoint:self
//                            regularBrowser:self.mainInterface.browser
//                           inactiveBrowser:self.mainInterface.inactiveBrowser
//                          incognitoBrowser:self.incognitoInterface.browser];
//    _mainCoordinator = tabGridCoordinator;
  }
  return _mainCoordinator;
}

- (WrangledBrowser*)mainInterface {
  return self.browserViewWrangler.mainInterface;
}

- (WrangledBrowser*)currentInterface {
  return self.browserViewWrangler.currentInterface;
}

- (WrangledBrowser*)incognitoInterface {
  return self.browserViewWrangler.incognitoInterface;
}

- (id<BrowserProviderInterface>)browserProviderInterface {
  return self.browserViewWrangler;
}

- (void)setStartupParameters:(AppStartupParameters*)parameters {
  _startupParameters = parameters;
  self.startupParametersAreBeingHandled = NO;
  
  Browser* mainBrowser =
  self.sceneState.browserProviderInterface.mainBrowserProvider.browser;
  if (!mainBrowser) {
    return;
  }
  
  ProfileIOS* profile = mainBrowser->GetProfile();
  if (!profile) {
    return;
  }
}


#pragma mark - SceneStateObserver

- (void)sceneState:(SceneState*)sceneState
    transitionedToActivationLevel:(SceneActivationLevel)level {
  ProfileState* profileState = self.sceneState.profileState;
  [self transitionToSceneActivationLevel:level
                        profileInitStage:profileState.initStage];
}

- (void)handleExternalIntents {
  if (![self canHandleIntents]) {
    return;
  }
  UserActivityBrowserAgent* userActivityBrowserAgent =
      UserActivityBrowserAgent::FromBrowser(self.currentInterface.browser);
  // Handle URL opening from
  // `UIWindowSceneDelegate scene:willConnectToSession:options:`.
  for (UIOpenURLContext* context in self.sceneState.connectionOptions
           .URLContexts) {
    URLOpenerParams* params =
        [[URLOpenerParams alloc] initWithUIOpenURLContext:context];
    [self openTabFromLaunchWithParams:params
                   startupInformation:self.sceneState.profileState.appState
                                          .startupInformation];
  }
  if (self.sceneState.connectionOptions.shortcutItem) {
    userActivityBrowserAgent->Handle3DTouchApplicationShortcuts(
        self.sceneState.connectionOptions.shortcutItem);
  }

  // See if this scene launched as part of a multiwindow URL opening.
  // If so, load that URL (this also creates a new tab to load the URL
  // in). No other UI will show in this case.
  NSUserActivity* activityWithCompletion;
  for (NSUserActivity* activity in self.sceneState.connectionOptions
           .userActivities) {
    if (ActivityIsURLLoad(activity)) {
      UrlLoadParams params = LoadParamsFromActivity(activity);
      ApplicationMode mode = params.in_incognito ? ApplicationMode::INCOGNITO
                                                 : ApplicationMode::NORMAL;
      [self openOrReuseTabInMode:mode
               withUrlLoadParams:params
             tabOpenedCompletion:nil];
    } else if (ActivityIsTabMove(activity)) {
      if ([self isTabActivityValid:activity]) {
        [self handleTabMoveActivity:activity];
      } else {
        // If the tab does not exist, open a new tab.
        UrlLoadParams params =
            UrlLoadParams::InNewTab(GURL(kChromeUINewTabURL));
        ApplicationMode mode = self.currentInterface.incognito
                                   ? ApplicationMode::INCOGNITO
                                   : ApplicationMode::NORMAL;
        [self openOrReuseTabInMode:mode
                 withUrlLoadParams:params
               tabOpenedCompletion:nil];
      }
    } else if (!activityWithCompletion) {
      // Completion involves user interaction.
      // Only one can be triggered.
      activityWithCompletion = activity;
    }
  }
  if (activityWithCompletion) {
    // This function is called when the scene is activated (or unblocked).
    // Consider the scene as still not active at this point as the handling
    // of startup parameters is not yet done (and will be later in this
    // function).
    userActivityBrowserAgent->ContinueUserActivity(activityWithCompletion, NO);
  }
  self.sceneState.connectionOptions = nil;

  if (self.startupParameters) {
    if ([self isIncognitoForced]) {
      [self.startupParameters
            setApplicationMode:ApplicationModeForTabOpening::INCOGNITO
          forceApplicationMode:YES];
    } else if ([self isIncognitoDisabled]) {
      [self.startupParameters
            setApplicationMode:ApplicationModeForTabOpening::NORMAL
          forceApplicationMode:YES];
    }

    userActivityBrowserAgent->RouteToCorrectTab();
  }
}

// Handles a tab move activity as part of an intent when launching a
// scene. This should only ever be an intent generated by Chrome.
- (void)handleTabMoveActivity:(NSUserActivity*)activity {
  DCHECK(ActivityIsTabMove(activity));
  BOOL incognito = GetIncognitoFromTabMoveActivity(activity);
  web::WebStateID tabID = GetTabIDFromActivity(activity);

  WrangledBrowser* interface = self.currentInterface;

  // It's expected that the current interface matches `incognito`.
  DCHECK(interface.incognito == incognito);

  // Move the tab to the current interface's browser.
  MoveTabToBrowser(tabID, interface.browser, /*destination_tab_index=*/0);
}

- (void)sceneState:(SceneState*)sceneState
    hasPendingURLs:(NSSet<UIOpenURLContext*>*)URLContexts {
  DCHECK(URLContexts);
  // It is necessary to reset the URLContextsToOpen after opening them.
  // Handle the opening asynchronously to avoid interfering with potential
  // other observers.
  dispatch_async(dispatch_get_main_queue(), ^{
    [self openURLContexts:sceneState.URLContextsToOpen];
    self.sceneState.URLContextsToOpen = nil;
  });
}

- (void)performActionForShortcutItem:(UIApplicationShortcutItem*)shortcutItem
                   completionHandler:
                       (void (^)(BOOL succeeded))completionHandler {
  if (self.sceneState.profileState.initStage <= ProfileInitStage::kUIReady ||
      !self.currentInterface.profile) {
    // Don't handle the intent if the browser UI objects aren't yet initialized.
    // This is the case when the app is in safe mode or may be the case when the
    // app is going through an odd sequence of lifecyle events (shouldn't happen
    // but happens somehow), see crbug.com/1211006 for more details.
    return;
  }

  self.sceneState.startupHadExternalIntent = YES;

  // Perform the action in incognito when only incognito mode is available.
  if ([self isIncognitoForced]) {
    [self.startupParameters
          setApplicationMode:ApplicationModeForTabOpening::INCOGNITO
        forceApplicationMode:YES];
  }
  UserActivityBrowserAgent* userActivityBrowserAgent =
      UserActivityBrowserAgent::FromBrowser(self.currentInterface.browser);
  BOOL handledShortcutItem =
      userActivityBrowserAgent->Handle3DTouchApplicationShortcuts(shortcutItem);
  if (completionHandler) {
    completionHandler(handledShortcutItem);
  }
}

- (void)sceneState:(SceneState*)sceneState
    receivedUserActivity:(NSUserActivity*)userActivity {
  if (!userActivity) {
    return;
  }

  if (self.sceneState.profileState.initStage <= ProfileInitStage::kUIReady ||
      !self.currentInterface.profile) {
    // Don't handle the intent if the browser UI objects aren't yet initialized.
    // This is the case when the app is in safe mode or may be the case when the
    // app is going through an odd sequence of lifecyle events (shouldn't happen
    // but happens somehow), see crbug.com/1211006 for more details.
    return;
  }

  BOOL sceneIsActive = [self canHandleIntents];
  self.sceneState.startupHadExternalIntent = YES;

  PrefService* prefs = self.currentInterface.profile->GetPrefs();
  UserActivityBrowserAgent* userActivityBrowserAgent =
      UserActivityBrowserAgent::FromBrowser(self.currentInterface.browser);
  if (IsIncognitoPolicyApplied(prefs) &&
      !userActivityBrowserAgent->ProceedWithUserActivity(userActivity)) {
    
    
  } else {
    userActivityBrowserAgent->ContinueUserActivity(userActivity, sceneIsActive);
  }

  if (sceneIsActive) {
    // It is necessary to reset the pendingUserActivity after handling it.
    // Handle the reset asynchronously to avoid interfering with other
    // observers.
    dispatch_async(dispatch_get_main_queue(), ^{
      self.sceneState.pendingUserActivity = nil;
    });
  }
}

#pragma mark - ProfileStateObserver

- (void)profileState:(ProfileState*)profileState
    didTransitionToInitStage:(ProfileInitStage)nextInitStage
               fromInitStage:(ProfileInitStage)fromInitStage {
  [self transitionToSceneActivationLevel:self.sceneState.activationLevel
                        profileInitStage:nextInitStage];
}

#pragma mark - private

// A sink for profileState:didTransitionFromInitStage: and
// sceneState:transitionedToActivationLevel: events.
//
// Discussion: the scene controller cares both about the profile and the scene
// init stages. This method is called from both observer callbacks and allows
// to handle all the transitions in one place.
- (void)transitionToSceneActivationLevel:(SceneActivationLevel)level
                        profileInitStage:(ProfileInitStage)profileInitStage {
  // Update `backgroundedSinceLastActivated` and, if the scene has just been
  // activated, mark its state before the current activation for future use.
  BOOL transitionedToForegroundActiveFromBackground =
      level == SceneActivationLevelForegroundActive &&
      self.backgroundedSinceLastActivated;
  if (level <= SceneActivationLevelBackground) {
    self.backgroundedSinceLastActivated = YES;
  } else if (level == SceneActivationLevelForegroundActive) {
    self.backgroundedSinceLastActivated = NO;
  }

  if (level == SceneActivationLevelDisconnected) {
    //  The scene may become disconnected at any time. In that case, any UI that
    //  was already set-up should be torn down.
    [self teardownUI];
  }
  if (profileInitStage < ProfileInitStage::kUIReady) {
    // Nothing else per-scene should happen before the app completes the global
    // setup, like executing Safe mode, or creating the main Profile.
    return;
  }

  BOOL initializingUIInColdStart =
      level > SceneActivationLevelBackground && !self.sceneState.UIEnabled;
  if (initializingUIInColdStart) {
    [self initializeUI];
    // Add the scene to the list of connected scene, to restore in case of
    // crashes.
    [[PreviousSessionInfo sharedInstance]
        addSceneSessionID:self.sceneState.sceneSessionID];
  }

  // When the scene transitions to inactive (such as when it's being shown in
  // the OS app-switcher), update the title for display on iPadOS.
  if (level == SceneActivationLevelForegroundInactive) {
    self.sceneState.scene.title = [self displayTitleForAppSwitcher];
  }

  if (level == SceneActivationLevelForegroundActive &&
      profileInitStage == ProfileInitStage::kFinal) {
    [self handleExternalIntents];

    if (!initializingUIInColdStart &&
        transitionedToForegroundActiveFromBackground &&
        self.mainCoordinator.isTabGridActive &&
        [self shouldOpenNTPTabOnActivationOfBrowser:self.currentInterface
                                                        .browser]) {
      DCHECK(!self.activatingBrowser);
      [self beginActivatingBrowser:self.mainInterface.browser focusOmnibox:NO];

      OpenNewTabCommand* command = [OpenNewTabCommand commandWithIncognito:NO];
      command.userInitiated = NO;
      Browser* browser = self.currentInterface.browser;
      id<ApplicationCommands> applicationHandler = HandlerForProtocol(
          browser->GetCommandDispatcher(), ApplicationCommands);
      [applicationHandler openURLInNewTab:command];
                                                          
                                                          
      // Display browser here
    }
  }

  if (self.sceneState.UIEnabled && level <= SceneActivationLevelDisconnected) {
    if (base::ios::IsMultipleScenesSupported()) {
      // If Multiple scenes are not supported, the session shouldn't be
      // removed as it can be used for normal restoration.
      [[PreviousSessionInfo sharedInstance]
          removeSceneSessionID:self.sceneState.sceneSessionID];
    }
  }
}

- (void)initializeUI {
  if (self.sceneState.UIEnabled) {
    return;
  }

  [self startUpChromeUI];
  self.sceneState.UIEnabled = YES;
}

// Starts up a single chrome window and its UI.
- (void)startUpChromeUI {
  DCHECK(!self.browserViewWrangler);
  DCHECK(_sceneURLLoadingService.get());
  DCHECK(self.sceneState.profileState.profile);

  SceneState* sceneState = self.sceneState;
  ProfileIOS* profile = sceneState.profileState.profile;

  self.browserViewWrangler =
      [[BrowserViewWrangler alloc] initWithProfile:profile
                                        sceneState:sceneState];

  // Create and start the BVC.
  [self.browserViewWrangler createMainCoordinatorAndInterface];

  [self activateBVCAndMakeCurrentBVCPrimary];
  [self.browserViewWrangler loadSession];
  [self createInitialUI:[self initialUIMode]];
}

// Determines the mode (normal or incognito) the initial UI should be in.
- (ApplicationMode)initialUIMode {
  // When only incognito mode is available.
  if ([self isIncognitoForced]) {
    return ApplicationMode::INCOGNITO;
  }

  // When only incognito mode is disabled.
  if ([self isIncognitoDisabled]) {
    return ApplicationMode::NORMAL;
  }

  // Check if the UI is being created from an intent; if it is, open in the
  // correct mode for that activity. Because all activities must be in the same
  // mode, as soon as any activity reports being in incognito, switch to that
  // mode.
  for (NSUserActivity* activity in self.sceneState.connectionOptions
           .userActivities) {
    if (ActivityIsTabMove(activity)) {
      return GetIncognitoFromTabMoveActivity(activity)
                 ? ApplicationMode::INCOGNITO
                 : ApplicationMode::NORMAL;
    }
  }

  // Launch in the mode that matches the state of the scene when the application
  // was terminated. If the scene was showing the incognito UI, but there are
  // no incognito tabs open (e.g. the tab switcher was active and user closed
  // the last tab), then instead show the regular UI.

  if (self.sceneState.incognitoContentVisible &&
      !self.incognitoInterface.browser->GetWebStateList()->empty()) {
    return ApplicationMode::INCOGNITO;
  }

  // In all other cases, default to normal mode.
  return ApplicationMode::NORMAL;
}

// Creates and displays the initial UI in `launchMode`, performing other
// setup and configuration as needed.
- (void)createInitialUI:(ApplicationMode)launchMode {
  
  _mainWebStateObserver = std::make_unique<
      base::ScopedObservation<WebStateList, WebStateListObserverBridge>>(
      _webStateListForwardingObserver.get());
  _mainWebStateObserver->Observe(self.mainInterface.browser->GetWebStateList());
  
  _incognitoWebStateObserver = std::make_unique<
      base::ScopedObservation<WebStateList, WebStateListObserverBridge>>(
      _webStateListForwardingObserver.get());
  
  _incognitoWebStateObserver->Observe(
      self.incognitoInterface.browser->GetWebStateList());

  if (base::FeatureList::IsEnabled(
          kMakeKeyAndVisibleBeforeMainCoordinatorStart)) {
    [self.sceneState setRootViewControllerKeyAndVisible];
  }

  // Lazy init of mainCoordinator.
  [self.mainCoordinator start];

  if (!base::FeatureList::IsEnabled(
          kMakeKeyAndVisibleBeforeMainCoordinatorStart)) {
    // Enables UI initializations to query the keyWindow's size. Do this after
    // `mainCoordinator start` as it sets self.window.rootViewController to work
    // around crbug.com/850387, causing a flicker if -makeKeyAndVisible has been
    // called.
    [self.sceneState setRootViewControllerKeyAndVisible];
  }

  Browser* browser = (launchMode == ApplicationMode::INCOGNITO)
                         ? self.incognitoInterface.browser
                         : self.mainInterface.browser;

  // Inject a NTP before setting the interface, which will trigger a load of
  // the current webState.
  if (self.sceneState.profileState.appState.postCrashAction ==
      PostCrashAction::kShowNTPWithReturnToTab) {
    InjectNTP(browser);
  }

  if (launchMode == ApplicationMode::INCOGNITO) {
    [self setCurrentInterfaceForMode:ApplicationMode::INCOGNITO];
  } else {
    [self setCurrentInterfaceForMode:ApplicationMode::NORMAL];
  }

  // Figure out what UI to show initially.

  if (self.mainCoordinator.isTabGridActive) {
    DCHECK(!self.activatingBrowser);
    [self beginActivatingBrowser:self.mainInterface.browser focusOmnibox:NO];
    [self finishActivatingBrowserDismissingTabSwitcher];
  }

  // If this web state list should have an NTP created when it activates, then
  // create that tab.
  if ([self shouldOpenNTPTabOnActivationOfBrowser:browser]) {
    OpenNewTabCommand* command = [OpenNewTabCommand
        commandWithIncognito:self.currentInterface.incognito];
    command.userInitiated = NO;
    Browser* currentBrowser = self.currentInterface.browser;
    id<ApplicationCommands> applicationHandler = HandlerForProtocol(
        currentBrowser->GetCommandDispatcher(), ApplicationCommands);
    [applicationHandler openURLInNewTab:command];
  }
}

- (void)teardownUI {
  [_mainCoordinator stop];
  _mainCoordinator = nil;

  _incognitoWebStateObserver.reset();
  _mainWebStateObserver.reset();

  self.sceneState.UIEnabled = NO;

  [[SessionSavingSceneAgent agentFromScene:self.sceneState]
      saveSessionsIfNeeded];
  [self.browserViewWrangler shutdown];
  self.browserViewWrangler = nil;

  [self.sceneState.profileState removeObserver:self];
  _sceneURLLoadingService.reset();
}

// Formats string for display on iPadOS application switcher with the
// domain of the foreground tab and the tab count. Assumes the scene is
// visible. Will return nil if there are no tabs.
- (NSString*)displayTitleForAppSwitcher {
  Browser* browser = self.currentInterface.browser;
  DCHECK(browser);

  if (browser->GetProfile()->IsOffTheRecord()) {
    return nil;
  }
  web::WebState* webState = browser->GetWebStateList()->GetActiveWebState();
  if (!webState) {
    return nil;
  }

  // At this point there is at least one tab.
  int numberOfTabs = browser->GetWebStateList()->count();
  DCHECK(numberOfTabs > 0);
  GURL url = webState->GetVisibleURL();
  std::u16string urlText = url_formatter::FormatUrl(
      url,
      url_formatter::kFormatUrlOmitDefaults |
          url_formatter::kFormatUrlOmitTrivialSubdomains |
          url_formatter::kFormatUrlOmitHTTPS |
          url_formatter::kFormatUrlTrimAfterHost,
      base::UnescapeRule::SPACES, nullptr, nullptr, nullptr);
  std::u16string pattern =
      l10n_util::GetStringUTF16(IDS_IOS_APP_SWITCHER_SCENE_TITLE);
  std::u16string formattedTitle =
      base::i18n::MessageFormatter::FormatWithNamedArgs(
          pattern, "domain", urlText, "count", numberOfTabs - 1);
  return base::SysUTF16ToNSString(formattedTitle);
}

- (BOOL)isIncognitoDisabled {
  return IsIncognitoModeDisabled(
      self.mainInterface.browser->GetProfile()->GetPrefs());
}

// YES if incognito mode is forced by enterprise policy.
- (BOOL)isIncognitoForced {
  return IsIncognitoModeForced(
      self.incognitoInterface.browser->GetProfile()->GetPrefs());
}

// Returns 'YES' if the tabID from the given `activity` is valid.
- (BOOL)isTabActivityValid:(NSUserActivity*)activity {
  web::WebStateID tabID = GetTabIDFromActivity(activity);

  ProfileIOS* profile = self.currentInterface.profile;
  BrowserList* browserList = BrowserListFactory::GetForProfile(profile);
  const BrowserList::BrowserType browser_types =
      self.currentInterface.incognito
          ? BrowserList::BrowserType::kIncognito
          : BrowserList::BrowserType::kRegularAndInactive;
  std::set<Browser*> browsers = browserList->BrowsersOfType(browser_types);

  BrowserAndIndex tabInfo = FindBrowserAndIndex(tabID, browsers);

  return tabInfo.tab_index != WebStateList::kInvalidIndex;
}

- (void)reconcileEulaAsAccepted {
  static dispatch_once_t once_token = 0;
  dispatch_once(&once_token, ^{
    PrefService* prefs = GetApplicationContext()->GetLocalState();
    if (!FirstRun::IsChromeFirstRun() &&
        !prefs->GetBoolean(prefs::kEulaAccepted)) {
      prefs->SetBoolean(prefs::kEulaAccepted, true);
      prefs->CommitPendingWrite();
      base::UmaHistogramBoolean("IOS.ReconcileEULAPref", true);
    }
  });
}

- (BOOL)canHandleIntents {
  if (self.sceneState.activationLevel < SceneActivationLevelForegroundActive) {
    return NO;
  }

  if (self.sceneState.profileState.initStage < ProfileInitStage::kFinal) {
    return NO;
  }

  if (self.sceneState.presentingModalOverlay) {
    return NO;
  }

  return YES;
}

#pragma mark - ApplicationCommands

- (void)openURLInNewTab:(OpenNewTabCommand*)command {
  UrlLoadParams params =
      UrlLoadParams::InNewTab(command.URL, command.virtualURL);
  params.SetInBackground(command.inBackground);
  params.web_params.referrer = command.referrer;
  params.in_incognito = command.inIncognito;
  params.append_to = command.appendTo;
  params.origin_point = command.originPoint;
  params.from_chrome = command.fromChrome;
  params.user_initiated = command.userInitiated;
  params.should_focus_omnibox = command.shouldFocusOmnibox;
  params.inherit_opener = !command.inBackground;
  _sceneURLLoadingService->LoadUrlInNewTab(params);
}

- (void)setIncognitoContentVisible:(BOOL)incognitoContentVisible {
  self.sceneState.incognitoContentVisible = incognitoContentVisible;
}

- (void)openNewWindowWithActivity:(NSUserActivity*)userActivity {
  if (!base::ios::IsMultipleScenesSupported()) {
    return;  // silent no-op.
  }

  UISceneActivationRequestOptions* options =
      [[UISceneActivationRequestOptions alloc] init];
  options.requestingScene = self.sceneState.scene;

  if (self.mainInterface) {
    PrefService* prefs = self.mainInterface.profile->GetPrefs();
    if (IsIncognitoModeForced(prefs)) {
      userActivity = AdaptUserActivityToIncognito(userActivity, true);
    } else if (IsIncognitoModeDisabled(prefs)) {
      userActivity = AdaptUserActivityToIncognito(userActivity, false);
    }

    [UIApplication.sharedApplication
        requestSceneSessionActivation:nil /* make a new scene */
                         userActivity:userActivity
                              options:options
                         errorHandler:nil];
  }
}

// Returns YES if the current Tab is available to present a view controller.
- (BOOL)isTabAvailableToPresentViewController {
  if (self.sceneState.profileState.initStage < ProfileInitStage::kFinal) {
    return NO;
  }
  return YES;
}

#pragma mark - TabGridCoordinatorDelegate

- (void)tabGrid:(TabGridCoordinator*)tabGrid
    shouldActivateBrowser:(Browser*)browser
             focusOmnibox:(BOOL)focusOmnibox {
  [self beginActivatingBrowser:browser focusOmnibox:focusOmnibox];
}

- (void)tabGridDismissTransitionDidEnd:(TabGridCoordinator*)tabGrid {
  if (!self.sceneState.UIEnabled) {
    return;
  }
  [self finishActivatingBrowserDismissingTabSwitcher];
}

// Begins the process of activating the given current model, switching which BVC
// is suspended if necessary. The omnibox will be focused after the tab switcher
// dismissal is completed if `focusOmnibox` is YES.
- (void)beginActivatingBrowser:(Browser*)browser
                  focusOmnibox:(BOOL)focusOmnibox {
  DCHECK(browser == self.mainInterface.browser ||
         browser == self.incognitoInterface.browser);

  self.activatingBrowser = YES;
  ApplicationMode mode = (browser == self.mainInterface.browser)
                             ? ApplicationMode::NORMAL
                             : ApplicationMode::INCOGNITO;
  [self setCurrentInterfaceForMode:mode];

  // The call to set currentBVC above does not actually display the BVC, because
  // _activatingBrowser is YES.  So: Force the BVC transition to start.
  [self displayCurrentBVCAndFocusOmnibox:focusOmnibox];
}

- (void)openLatestTab {
  WebStateList* webStateList = self.currentInterface.browser->GetWebStateList();
  web::WebState* webState = StartSurfaceRecentTabBrowserAgent::FromBrowser(
                                self.currentInterface.browser)
                                ->most_recent_tab();
  if (!webState) {
    return;
  }
  int index = webStateList->GetIndexOfWebState(webState);
  webStateList->ActivateWebStateAt(index);
}

#pragma mark - TabOpening implementation.

- (void)openTabFromLaunchWithParams:(URLOpenerParams*)params
                 startupInformation:(id<StartupInformation>)startupInformation {
  if (params) {
    [URLOpener handleLaunchOptions:params
                         tabOpener:self
             connectionInformation:self
                startupInformation:startupInformation
                       prefService:self.currentInterface.profile->GetPrefs()
                         initStage:self.sceneState.profileState.initStage];
  }
}

- (BOOL)URLIsOpenedInRegularMode:(const GURL&)URL {
  WebStateList* webStateList = self.mainInterface.browser->GetWebStateList();
  return webStateList && webStateList->GetIndexOfWebStateWithURL(URL) !=
                             WebStateList::kInvalidIndex;
}

- (BOOL)shouldOpenNTPTabOnActivationOfBrowser:(Browser*)browser {
  // Check if there are pending actions that would result in opening a new tab.
  // In that case, it is not useful to open another tab.
  for (NSUserActivity* activity in self.sceneState.connectionOptions
           .userActivities) {
    if (ActivityIsURLLoad(activity) || ActivityIsTabMove(activity)) {
      return NO;
    }
  }

  if (self.startupParameters) {
    return NO;
  }

  if (self.mainCoordinator.isTabGridActive) {
    Browser* mainBrowser = self.mainInterface.browser;
    Browser* otrBrowser = self.incognitoInterface.browser;
    // Only attempt to dismiss the tab switcher and open a new tab if:
    // - there are no tabs open in either tab model, and
    // - the tab switcher controller is not directly or indirectly presenting
    // another view controller.
    if (!(mainBrowser->GetWebStateList()->empty()) ||
        !(otrBrowser->GetWebStateList()->empty())) {
      return NO;
    }

    // If the tabSwitcher is contained, check if the parent container is
    // presenting another view controller.
    if ([self.mainCoordinator.baseViewController
                .parentViewController presentedViewController]) {
      return NO;
    }

    // Check if the tabSwitcher is directly presenting another view controller.
    if (self.mainCoordinator.baseViewController.presentedViewController) {
      return NO;
    }

    return YES;
  }

  return browser->GetWebStateList()->empty();
}

#pragma mark - SceneURLLoadingServiceDelegate

// Note that the current tab of `browserCoordinator`'s BVC will normally be
// reloaded by this method. If a new tab is about to be added, call
// expectNewForegroundTab on the BVC first to avoid extra work and possible page
// load side-effects for the tab being replaced.
- (void)setCurrentInterfaceForMode:(ApplicationMode)mode {
  DCHECK(self.browserViewWrangler);
  BOOL incognito = mode == ApplicationMode::INCOGNITO;
  WrangledBrowser* currentInterface = self.currentInterface;
  WrangledBrowser* newInterface =
      incognito ? self.incognitoInterface : self.mainInterface;
  if (currentInterface && currentInterface == newInterface) {
    return;
  }

  // Update the snapshot before switching another application mode.  This
  // ensures that the snapshot is correct when links are opened in a different
  // application mode.
  [self updateActiveWebStateSnapshot];

  self.browserViewWrangler.currentInterface = newInterface;

  if (!self.activatingBrowser) {
    [self displayCurrentBVCAndFocusOmnibox:NO];
  }

  // Tell the BVC that was made current that it can use the web.
  [self activateBVCAndMakeCurrentBVCPrimary];
}

- (void)openMultipleTabsWithURLs:(const std::vector<GURL>&)URLs
                 inIncognitoMode:(BOOL)openInIncognito
                      completion:(ProceduralBlock)completion {
  [self recursiveOpenURLs:URLs
          inIncognitoMode:openInIncognito
             currentIndex:0
               totalCount:URLs.size()
               completion:completion];
}

// Call `dismissModalsAndMaybeOpenSelectedTabInMode` recursively to open the
// list of URLs contained in `URLs`. Achieved through chaining
// `dismissModalsAndMaybeOpenSelectedTabInMode` in its completion handler.
- (void)recursiveOpenURLs:(const std::vector<GURL>&)URLs
          inIncognitoMode:(BOOL)incognitoMode
             currentIndex:(size_t)currentIndex
               totalCount:(size_t)totalCount
               completion:(ProceduralBlock)completion {
  if (currentIndex >= totalCount) {
    if (completion) {
      completion();
    }
    return;
  }

  GURL webpageGURL = URLs.at(currentIndex);

  __weak SceneController* weakSelf = self;

  if (!webpageGURL.is_valid()) {
    [self recursiveOpenURLs:URLs
            inIncognitoMode:incognitoMode
               currentIndex:(currentIndex + 1)
                 totalCount:totalCount
                 completion:completion];
    return;
  }

  UrlLoadParams param = UrlLoadParams::InNewTab(webpageGURL, webpageGURL);
  std::vector<GURL> copyURLs = URLs;

  ApplicationModeForTabOpening mode =
      incognitoMode ? ApplicationModeForTabOpening::INCOGNITO
                    : ApplicationModeForTabOpening::NORMAL;
  
  [weakSelf
      recursiveOpenURLs:copyURLs
        inIncognitoMode:incognitoMode
           currentIndex:(currentIndex + 1)
             totalCount:totalCount
             completion:completion];
}

- (void)expectNewForegroundTabForMode:(ApplicationMode)targetMode {
  WrangledBrowser* interface = targetMode == ApplicationMode::INCOGNITO
                                   ? self.incognitoInterface
                                   : self.mainInterface;
  DCHECK(interface);
  PagePlaceholderBrowserAgent* pagePlaceholderBrowserAgent =
      PagePlaceholderBrowserAgent::FromBrowser(interface.browser);
  pagePlaceholderBrowserAgent->ExpectNewForegroundTab();
}

- (void)openNewTabFromOriginPoint:(CGPoint)originPoint
                     focusOmnibox:(BOOL)focusOmnibox
                    inheritOpener:(BOOL)inheritOpener {
  [self.currentInterface.bvc openNewTabFromOriginPoint:originPoint
                                          focusOmnibox:focusOmnibox
                                         inheritOpener:inheritOpener];
}

- (Browser*)currentBrowserForURLLoading {
  return self.currentInterface.browser;
}

- (UrlLoadingBrowserAgent*)browserAgentForIncognito:(BOOL)incognito {
  if (incognito) {
    return UrlLoadingBrowserAgent::FromBrowser(self.incognitoInterface.browser);
  }
  return UrlLoadingBrowserAgent::FromBrowser(self.mainInterface.browser);
}

// Asks the respective Snapshot helper to update the snapshot for the active
// WebState.
- (void)updateActiveWebStateSnapshot {
  // Durinhg startup, there may be no current interface. Do nothing in that
  // case.
  if (!self.currentInterface) {
    return;
  }

  WebStateList* webStateList = self.currentInterface.browser->GetWebStateList();
  web::WebState* webState = webStateList->GetActiveWebState();
  if (webState) {
    SnapshotTabHelper::FromWebState(webState)->UpdateSnapshotWithCallback(nil);
  }
}

// Checks the target BVC's current tab's URL. If `urlLoadParams` has an empty
// URL, no new tab will be opened and `tabOpenedCompletion` will be run. If this
// URL is chrome://newtab, loads `urlLoadParams` in this tab. Otherwise, open
// `urlLoadParams` in a new tab in the target BVC. `tabOpenedCompletion` will be
// called on the new tab (if not nil).
- (void)openOrReuseTabInMode:(ApplicationMode)targetMode
           withUrlLoadParams:(const UrlLoadParams&)urlLoadParams
         tabOpenedCompletion:(ProceduralBlock)tabOpenedCompletion {
  WrangledBrowser* targetInterface = targetMode == ApplicationMode::NORMAL
                                         ? self.mainInterface
                                         : self.incognitoInterface;
  // If the url to load is empty, create a new tab if no tabs are open and run
  // the completion.
  if (urlLoadParams.web_params.url.is_empty()) {
    if (tabOpenedCompletion) {
      tabOpenedCompletion();
    }
    return;
  }

  BrowserViewController* targetBVC = targetInterface.bvc;
  web::WebState* currentWebState =
      targetInterface.browser->GetWebStateList()->GetActiveWebState();

  // Refrain from reusing the same tab for Lens Overlay initiated requests.
  BOOL initiatedByLensOverlay = false;
  if (IsLensOverlayAvailable() && currentWebState) {
    if (LensOverlayTabHelper* lensOverlayTabHelper =
            LensOverlayTabHelper::FromWebState(currentWebState)) {
      initiatedByLensOverlay =
          lensOverlayTabHelper->IsLensOverlayUIAttachedAndAlive();
    }
  }

  BOOL forceNewTabForIntentSearch =
      base::FeatureList::IsEnabled(kForceNewTabForIntentSearch) &&
      (self.startupParameters.postOpeningAction == FOCUS_OMNIBOX);
  BOOL alwaysInsertNewTab =
      initiatedByLensOverlay || forceNewTabForIntentSearch;

  // Don't call loadWithParams for chrome://newtab when it's already loaded.
  // Note that it's safe to use -GetVisibleURL here, as it doesn't matter if the
  // NTP hasn't finished loading.
  if (!alwaysInsertNewTab && currentWebState &&
      IsUrlNtp(currentWebState->GetVisibleURL()) &&
      IsUrlNtp(urlLoadParams.web_params.url)) {
    if (tabOpenedCompletion) {
      tabOpenedCompletion();
    }
    return;
  }

  if (urlLoadParams.disposition == WindowOpenDisposition::SWITCH_TO_TAB) {
    // Check if it's already the displayed tab and no switch is necessary
    if (currentWebState &&
        currentWebState->GetVisibleURL() == urlLoadParams.web_params.url) {
      if (tabOpenedCompletion) {
        tabOpenedCompletion();
      }
      return;
    }

    // Check if this tab exists in this web state list.
    // If not, fall back to opening a new tab instead.
    if (targetInterface.browser->GetWebStateList()->GetIndexOfWebStateWithURL(
            urlLoadParams.web_params.url) != WebStateList::kInvalidIndex) {
      UrlLoadingBrowserAgent::FromBrowser(targetInterface.browser)
          ->Load(urlLoadParams);
      if (tabOpenedCompletion) {
        tabOpenedCompletion();
      }
      return;
    }
  }

  // If the current tab isn't an NTP, open a new tab.  Be sure to use
  // -GetLastCommittedURL incase the NTP is still loading.
  if (alwaysInsertNewTab ||
      !(currentWebState && IsUrlNtp(currentWebState->GetVisibleURL()))) {
    [targetBVC appendTabAddedCompletion:tabOpenedCompletion];
    UrlLoadParams newTabParams = urlLoadParams;
    newTabParams.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
    newTabParams.in_incognito = targetMode == ApplicationMode::INCOGNITO;
    UrlLoadingBrowserAgent::FromBrowser(targetInterface.browser)
        ->Load(newTabParams);
    return;
  }

  // Otherwise, load `urlLoadParams` in the current tab.
  UrlLoadParams sameTabParams = urlLoadParams;
  sameTabParams.disposition = WindowOpenDisposition::CURRENT_TAB;
  UrlLoadingBrowserAgent::FromBrowser(targetInterface.browser)
      ->Load(sameTabParams);
  if (tabOpenedCompletion) {
    tabOpenedCompletion();
  }
}

// Displays current (incognito/normal) BVC and optionally focuses the omnibox.
- (void)displayCurrentBVCAndFocusOmnibox:(BOOL)focusOmnibox {
  ProceduralBlock completion = nil;
  if (focusOmnibox) {
    id<OmniboxCommands> omniboxHandler = HandlerForProtocol(
        self.currentInterface.browser->GetCommandDispatcher(), OmniboxCommands);
    completion = ^{
      [omniboxHandler focusOmnibox];
    };
  }
  [self.mainCoordinator
      showTabViewController:self.currentInterface.viewController
                  incognito:self.currentInterface.incognito
                 completion:completion];
  [HandlerForProtocol(self.currentInterface.browser->GetCommandDispatcher(),
                      ApplicationCommands)
      setIncognitoContentVisible:self.currentInterface.incognito];
}

#pragma mark - WebStateListObserving

- (void)didChangeWebStateList:(WebStateList*)webStateList
                       change:(const WebStateListChange&)change
                       status:(const WebStateListStatus&)status {
  switch (change.type()) {
    case WebStateListChange::Type::kStatusOnly:
      // Do nothing when a WebState is selected and its status is updated.
      break;
    case WebStateListChange::Type::kDetach: {
      // Do nothing during batch operation.
      if (webStateList->IsBatchInProgress()) {
        break;
      }

      if (webStateList->empty()) {
        [self onLastWebStateClosedForWebStateList:webStateList];
      }
      break;
    }
    case WebStateListChange::Type::kMove:
      // Do nothing when a WebState is moved.
      break;
    case WebStateListChange::Type::kReplace:
      // Do nothing when a WebState is replaced.
      break;
    case WebStateListChange::Type::kInsert:
      // Do nothing when a WebState is inserted.
      break;
    case WebStateListChange::Type::kGroupCreate:
      // Do nothing when a group is created.
      break;
    case WebStateListChange::Type::kGroupVisualDataUpdate:
      // Do nothing when a tab group's visual data are updated.
      break;
    case WebStateListChange::Type::kGroupMove:
      // Do nothing when a tab group is moved.
      break;
    case WebStateListChange::Type::kGroupDelete:
      // Do nothing when a group is deleted.
      break;
  }
}

- (void)webStateListWillBeginBatchOperation:(WebStateList*)webStateList {
  _tabCountBeforeBatchOperation.insert(
      std::make_pair(webStateList, webStateList->count()));
}

- (void)webStateListBatchOperationEnded:(WebStateList*)webStateList {
  auto iter = _tabCountBeforeBatchOperation.find(webStateList);
  DCHECK(iter != _tabCountBeforeBatchOperation.end());

  // Triggers the switcher view if the list is empty and at least one tab
  // was closed (i.e. it was not empty before the batch operation).
  if (webStateList->empty() && iter->second != 0) {
    [self onLastWebStateClosedForWebStateList:webStateList];
  }

  _tabCountBeforeBatchOperation.erase(iter);
}

#pragma mark - Private methods

// Triggers the switcher view when the last WebState is closed on a device
// that uses the switcher.
- (void)onLastWebStateClosedForWebStateList:(WebStateList*)webStateList {
  DCHECK(webStateList->empty());
  if (webStateList == self.incognitoInterface.browser->GetWebStateList()) {
    [self lastIncognitoTabClosed];
  } else if (webStateList == self.mainInterface.browser->GetWebStateList()) {
    [self lastRegularTabClosed];
  }
}

#pragma mark - Helpers for web state list events

// Called when the last incognito tab was closed.
- (void)lastIncognitoTabClosed {
  // If no other window has incognito tab, then destroy and rebuild the
  // Profile. Otherwise, just do the state transition animation.
  if ([self shouldDestroyAndRebuildIncognitoProfile]) {
    // Incognito profile cannot be deleted before all the requests are
    // deleted. Queue empty task on IO thread and destroy the Profile
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

  // a) The first condition can happen when the last incognito tab is closed
  // from the tab switcher.
  // b) The second condition can happen if some other code (like JS) triggers
  // closure of tabs from the otr tab model when it's not current.
  // Nothing to do here. The next user action (like clicking on an existing
  // regular tab or creating a new incognito tab from the settings menu) will
  // take care of the logic to mode switch.
  if (self.mainCoordinator.isTabGridActive ||
      !self.currentInterface.incognito) {
    return;
  }
  
  // TODO: Display something
}

// Called when the last regular tab was closed.
- (void)lastRegularTabClosed {
  // a) The first condition can happen when the last regular tab is closed from
  // the tab switcher.
  // b) The second condition can happen if some other code (like JS) triggers
  // closure of tabs from the main tab model when the main tab model is not
  // current.
  // Nothing to do here.
  if (self.mainCoordinator.isTabGridActive || self.currentInterface.incognito) {
    return;
  }

  [self showTabSwitcher];
}

// Clears incognito data that is specific to iOS and won't be cleared by
// deleting the profile.
- (void)clearIOSSpecificIncognitoData {
  DCHECK(self.sceneState.browserProviderInterface.mainBrowserProvider.browser
             ->GetProfile()
             ->HasOffTheRecordProfile());
  ProfileIOS* otrProfile =
      self.sceneState.browserProviderInterface.mainBrowserProvider.browser
          ->GetProfile()
          ->GetOffTheRecordProfile();

  __weak SceneController* weakSelf = self;
  BrowsingDataRemover* browsingDataRemover =
      BrowsingDataRemoverFactory::GetForProfile(otrProfile);
  browsingDataRemover->Remove(browsing_data::TimePeriod::ALL_TIME,
                              BrowsingDataRemoveMask::REMOVE_ALL,
                              base::BindOnce(^{
                                [weakSelf activateBVCAndMakeCurrentBVCPrimary];
                              }));
}

- (void)activateBVCAndMakeCurrentBVCPrimary {
  // If there are pending removal operations, the activation will be deferred
  // until the callback is received.
  BrowsingDataRemover* browsingDataRemover =
      BrowsingDataRemoverFactory::GetForProfileIfExists(
          self.currentInterface.profile);
  if (browsingDataRemover && browsingDataRemover->IsRemoving()) {
    return;
  }

  WebUsageEnablerBrowserAgent::FromBrowser(self.mainInterface.browser)
      ->SetWebUsageEnabled(true);
  WebUsageEnablerBrowserAgent::FromBrowser(self.incognitoInterface.browser)
      ->SetWebUsageEnabled(true);

  if (self.currentInterface) {
    TabUsageRecorderBrowserAgent* tabUsageRecorder =
        TabUsageRecorderBrowserAgent::FromBrowser(
            self.currentInterface.browser);
    if (tabUsageRecorder) {
      tabUsageRecorder->RecordPrimaryBrowserChange(true);
    }
  }
}

- (void)openURLContexts:(NSSet<UIOpenURLContext*>*)URLContexts {
  if (self.sceneState.profileState.initStage <= ProfileInitStage::kUIReady ||
      !self.currentInterface.profile) {
    // Don't handle the intent if the browser UI objects aren't yet initialized.
    // This is the case when the app is in safe mode or may be the case when the
    // app is going through an odd sequence of lifecyle events (shouldn't happen
    // but happens somehow), see crbug.com/1211006 for more details.
    return;
  }

  NSMutableSet<URLOpenerParams*>* URLsToOpen = [[NSMutableSet alloc] init];
  for (UIOpenURLContext* context : URLContexts) {
    URLOpenerParams* options =
        [[URLOpenerParams alloc] initWithUIOpenURLContext:context];
    NSSet* URLContextSet = [NSSet setWithObject:context];
    if (!GetApplicationContext()
             ->GetSystemIdentityManager()
             ->HandleSessionOpenURLContexts(self.sceneState.scene,
                                            URLContextSet)) {
      [URLsToOpen addObject:options];
    }
  }
  // When opening with URLs for GetChromeIdentityService, it is expected that a
  // single URL is passed.
  DCHECK(URLsToOpen.count == URLContexts.count || URLContexts.count == 1);
  BOOL active = [self canHandleIntents];

  for (URLOpenerParams* options : URLsToOpen) {
    [URLOpener openURL:options
            applicationActive:active
                    tabOpener:self
        connectionInformation:self
           startupInformation:self.sceneState.profileState.startupInformation
                  prefService:self.currentInterface.profile->GetPrefs()
                    initStage:self.sceneState.profileState.initStage];
  }
}

#pragma mark - TabGrid helpers

// Adds a new tab to the `browser` based on `urlLoadParams` and then presents
// it.
- (void)addANewTabAndPresentBrowser:(Browser*)browser
                  withURLLoadParams:(const UrlLoadParams&)urlLoadParams {
  TabInsertion::Params tabInsertionParams;
  tabInsertionParams.should_skip_new_tab_animation =
      urlLoadParams.from_external;
  TabInsertionBrowserAgent::FromBrowser(browser)->InsertWebState(
      urlLoadParams.web_params, tabInsertionParams);
  [self beginActivatingBrowser:browser focusOmnibox:NO];
}

#pragma mark - Handling of destroying the incognito profile

// The incognito Profile should be closed when the last incognito tab is
// closed (i.e. if there are other incognito tabs open in another Scene, the
// Profile must not be destroyed).
- (BOOL)shouldDestroyAndRebuildIncognitoProfile {
  ProfileIOS* profile = self.sceneState.browserProviderInterface
                            .mainBrowserProvider.browser->GetProfile();
  if (!profile->HasOffTheRecordProfile()) {
    return NO;
  }

  ProfileIOS* otrProfile = profile->GetOffTheRecordProfile();
  DCHECK(otrProfile);

  BrowserList* browserList = BrowserListFactory::GetForProfile(otrProfile);
  for (Browser* browser :
       browserList->BrowsersOfType(BrowserList::BrowserType::kIncognito)) {
    WebStateList* webStateList = browser->GetWebStateList();
    if (!webStateList->empty()) {
      return NO;
    }
  }

  return YES;
}

// Destroys and rebuilds the incognito Profile. This will inform all the
// other SceneController to destroy state tied to the Profile and to
// recreate it.
- (void)destroyAndRebuildIncognitoProfile {
  // This seems the best place to mark the start of destroying the incognito
  // profile.
  crash_keys::SetDestroyingAndRebuildingIncognitoBrowserState(
      /*in_progress=*/true);

  [self clearIOSSpecificIncognitoData];

  ProfileIOS* profile = self.sceneState.browserProviderInterface
                            .mainBrowserProvider.browser->GetProfile();
  DCHECK(profile->HasOffTheRecordProfile());
  ProfileIOS* otrProfile = profile->GetOffTheRecordProfile();

  NSMutableArray<SceneController*>* sceneControllers =
      [[NSMutableArray alloc] init];
  for (SceneState* sceneState in self.sceneState.profileState.connectedScenes) {
    SceneController* sceneController = sceneState.controller;
    // In some circumstances, the scene state may still exist while the
    // corresponding scene controller has been deallocated.
    // (see crbug.com/1142782).
    if (sceneController) {
      [sceneControllers addObject:sceneController];
    }
  }

  for (SceneController* sceneController in sceneControllers) {
    [sceneController willDestroyIncognitoProfile];
  }

  // Record off-the-record metrics before detroying the Profile.
  SessionMetrics::FromProfile(otrProfile)
      ->RecordAndClearSessionMetrics(MetricsToRecordFlags::kNoMetrics);

  // Destroy and recreate the off-the-record Profile.
  profile->DestroyOffTheRecordProfile();
  profile->GetOffTheRecordProfile();

  for (SceneController* sceneController in sceneControllers) {
    [sceneController incognitoProfileCreated];
  }

  // This seems the best place to deem the destroying and rebuilding the
  // incognito profile to be completed.
  crash_keys::SetDestroyingAndRebuildingIncognitoBrowserState(
      /*in_progress=*/false);
}

- (void)willDestroyIncognitoProfile {
  // Clear the Incognito Browser and notify the TabGrid that its otrBrowser
  // will be destroyed.
  self.mainCoordinator.incognitoBrowser = nil;

  if (breadcrumbs::IsEnabled(GetApplicationContext()->GetLocalState())) {
    BreadcrumbManagerBrowserAgent::FromBrowser(self.incognitoInterface.browser)
        ->SetLoggingEnabled(false);
  }

  _incognitoWebStateObserver.reset();
  [self.browserViewWrangler willDestroyIncognitoProfile];
}

- (void)incognitoProfileCreated {
  [self.browserViewWrangler incognitoProfileCreated];

  // There should be a new URL loading browser agent for the incognito browser,
  // so set the scene URL loading service on it.
  UrlLoadingBrowserAgent::FromBrowser(self.incognitoInterface.browser)
      ->SetSceneService(_sceneURLLoadingService.get());
  _incognitoWebStateObserver = std::make_unique<
      base::ScopedObservation<WebStateList, WebStateListObserverBridge>>(
      _webStateListForwardingObserver.get());
  _incognitoWebStateObserver->Observe(
      self.incognitoInterface.browser->GetWebStateList());
  if (self.currentInterface.incognito) {
    [self activateBVCAndMakeCurrentBVCPrimary];
  }

  // Always set the new otr Browser for the tablet or grid switcher.
  // Notify the TabGrid with the new Incognito Browser.
  self.mainCoordinator.incognitoBrowser = self.incognitoInterface.browser;
}

#pragma mark - SceneUIProvider

- (UIViewController*)activeViewController {
  return self.mainCoordinator.activeViewController;
}
@end
