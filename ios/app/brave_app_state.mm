#include "brave/ios/app/brave_app_state.h"
#include "brave/ios/app/brave_scene_delegate.h"
#include "ios/chrome/browser/shared/coordinator/scene/scene_state.h"
#include "ios/chrome/browser/shared/coordinator/scene/scene_controller.h"
#include "base/apple/foundation_util.h"
#include "ios/chrome/browser/crash_report/model/crash_keys_helper.h"

@interface BraveCoreSceneDelegate()
@property(nonatomic, readonly) SceneState* sceneState;
@property(nonatomic, readonly) SceneController* controller;
@end

@implementation BraveAppState

- (NSArray<SceneState*>*)connectedScenes {
  NSMutableArray* sceneStates = [[NSMutableArray alloc] init];
  NSSet* connectedScenes = [UIApplication sharedApplication].connectedScenes;
  for (UIWindowScene* scene in connectedScenes) {
    if (![scene.delegate isKindOfClass:[BraveCoreSceneDelegate class]]) {
      // This might happen in tests.
      // TODO(crbug.com/40710078): This shouldn't be needed. (It might also
      // be the cause of crbug.com/1142782).
      [sceneStates addObject:[[SceneState alloc] initWithAppState:self]];
      continue;
    }

    BraveCoreSceneDelegate* sceneDelegate =
        base::apple::ObjCCastStrict<BraveCoreSceneDelegate>(scene.delegate);
    [sceneStates addObject:sceneDelegate.sceneState];
  }
  return sceneStates;
}

- (void)sceneWillConnect:(NSNotification*)notification {
  UIWindowScene* scene =
      base::apple::ObjCCastStrict<UIWindowScene>(notification.object);
  BraveCoreSceneDelegate* sceneDelegate =
      base::apple::ObjCCastStrict<BraveCoreSceneDelegate>(scene.delegate);

  // Under some iOS 15 betas, Chrome gets scene connection events for some
  // system scene connections. To handle this, early return if the connecting
  // scene doesn't have a valid delegate. (See crbug.com/1217461)
  if (!sceneDelegate) {
    return;
  }

  SceneState* sceneState = sceneDelegate.sceneState;
  DCHECK(sceneState);

  [[self valueForKey:@"_observers"] appState:self sceneConnected:sceneState];
  crash_keys::SetConnectedScenesCount([self connectedScenes].count);
}

@end
