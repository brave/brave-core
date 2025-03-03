#import <UIKit/UIKit.h>

#import "base/functional/callback.h"
#import "ios/chrome/browser/shared/coordinator/scene/connection_information.h"
#import "ios/chrome/browser/shared/coordinator/scene/scene_state.h"
#import "ios/chrome/browser/shared/coordinator/scene/scene_state_observer.h"
#import "ios/chrome/browser/shared/model/web_state_list/web_state_list_observer_bridge.h"

@protocol BrowserProviderInterface;
@class ProfileState;

// The controller object for a scene. Reacts to scene state changes.
@interface SceneController : NSObject <ConnectionInformation,
                                       SceneStateObserver,
                                       WebStateListObserving>
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithSceneState:(SceneState*)sceneState
    NS_DESIGNATED_INITIALIZER;

// The interface provider for this scene.
@property(nonatomic, strong, readonly) id<BrowserProviderInterface>
    browserProviderInterface;

// Connects the ProfileState to this SceneController.
- (void)setProfileState:(ProfileState*)profileState;

// Handler for the UIWindowSceneDelegate callback with the same selector.
- (void)performActionForShortcutItem:(UIApplicationShortcutItem*)shortcutItem
                   completionHandler:
                       (void (^)(BOOL succeeded))completionHandler;
@end
