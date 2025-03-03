#include "brave/ios/app/brave_scene_delegate.h"
#include "brave/ios/app/brave_app_state.h"
#include "brave/ios/app/brave_core_main.h"
#include "ios/chrome/app/chrome_overlay_window.h"
#include "ios/chrome/browser/shared/coordinator/scene/scene_controller.h"
#include "ios/chrome/browser/shared/coordinator/scene/scene_state.h"
#include "base/apple/foundation_util.h"

@interface BraveCoreMain()
@property(nonatomic, readonly) BraveAppState* appState;
@end

@interface BraveCoreSceneDelegate()
@property(nonatomic) SceneState* sceneState;
@property(nonatomic) SceneController* sceneController;
@end

@implementation BraveCoreSceneDelegate
- (SceneState*)sceneState {
  if (!_sceneState) {
    BraveAppState* appState = [[[self class] braveCoreMain] appState];
    _sceneState = [[SceneState alloc] initWithAppState:appState];
    _sceneController = [[SceneController alloc] initWithSceneState:_sceneState];
    _sceneState.controller = _sceneController;
  }
  return _sceneState;
}

- (UIWindow*)window {
  if (!_window) {
    _window = [[ChromeOverlayWindow alloc] init];
    _window.accessibilityIdentifier = [NSString
        stringWithFormat:@"%ld",
                         UIApplication.sharedApplication.connectedScenes.count -
                             1];
  }
  return _window;
}

+ (BraveCoreMain*)braveCoreMain {
  return nil;
}

#pragma mark - UISceneDelegate

- (void)scene:(UIScene*)scene
    willConnectToSession:(UISceneSession*)session
                 options:(UISceneConnectionOptions*)connectionOptions {
  SceneState* sceneState = self.sceneState;
  sceneState.scene = base::apple::ObjCCastStrict<UIWindowScene>(scene);
  sceneState.currentOrigin = [self originFromSession:session
                                             options:connectionOptions];
  sceneState.activationLevel = SceneActivationLevelBackground;
  sceneState.connectionOptions = connectionOptions;
  if (connectionOptions.shortcutItem != nil ||
      connectionOptions.URLContexts.count != 0 ||
      connectionOptions.userActivities.count != 0) {
    sceneState.startupHadExternalIntent = YES;
  }
}

- (void)sceneDidDisconnect:(UIScene*)scene {
  CHECK(_sceneState);
  [self.sceneState setRootViewController:nil makeKeyAndVisible:NO];
  self.sceneState.activationLevel = SceneActivationLevelDisconnected;
  _sceneState = nil;
  // Setting the level to Disconnected had the side effect of tearing down the
  // controller’s UI.
  _sceneController = nil;
}

#pragma mark Transitioning to the Foreground

- (void)sceneWillEnterForeground:(UIScene*)scene {
  self.sceneState.currentOrigin = WindowActivityRestoredOrigin;
  self.sceneState.activationLevel = SceneActivationLevelForegroundInactive;
}

- (void)sceneDidBecomeActive:(UIScene*)scene {
  self.sceneState.currentOrigin = WindowActivityRestoredOrigin;
  self.sceneState.activationLevel = SceneActivationLevelForegroundActive;
}

#pragma mark Transitioning to the Background

- (void)sceneWillResignActive:(UIScene*)scene {
  self.sceneState.activationLevel = SceneActivationLevelForegroundInactive;
}

- (void)sceneDidEnterBackground:(UIScene*)scene {
  self.sceneState.activationLevel = SceneActivationLevelBackground;
}

- (void)scene:(UIScene*)scene
    openURLContexts:(NSSet<UIOpenURLContext*>*)URLContexts {
  DCHECK(!self.sceneState.URLContextsToOpen);
  self.sceneState.startupHadExternalIntent = YES;
  self.sceneState.URLContextsToOpen = URLContexts;
}

- (void)windowScene:(UIWindowScene*)windowScene
    performActionForShortcutItem:(UIApplicationShortcutItem*)shortcutItem
               completionHandler:(void (^)(BOOL succeeded))completionHandler {
  [_sceneController performActionForShortcutItem:shortcutItem
                               completionHandler:completionHandler];
}

- (void)scene:(UIScene*)scene
    continueUserActivity:(NSUserActivity*)userActivity {
  self.sceneState.pendingUserActivity = userActivity;
}

#pragma mark - private

- (WindowActivityOrigin)originFromSession:(UISceneSession*)session
                                  options:(UISceneConnectionOptions*)options {
  NSString* const kOriginDetectedKey = @"OriginDetectedKey";
  WindowActivityOrigin origin = WindowActivityUnknownOrigin;
  
  if (session.userInfo[kOriginDetectedKey]) {
    origin = WindowActivityRestoredOrigin;
  } else {
    NSMutableDictionary* userInfo =
        [NSMutableDictionary dictionaryWithDictionary:session.userInfo];
    userInfo[kOriginDetectedKey] = kOriginDetectedKey;
    session.userInfo = userInfo;
    origin = WindowActivityExternalOrigin;
    for (NSUserActivity* activity in options.userActivities) {
      WindowActivityOrigin activityOrigin = OriginOfActivity(activity);
      if (activityOrigin != WindowActivityUnknownOrigin) {
        origin = activityOrigin;
        break;
      }
    }
  }

  return origin;
}
@end
