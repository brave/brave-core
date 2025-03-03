
#include "src/ios/chrome/app/variations_app_state_agent.mm"

@implementation BraveVariationsAppStateAgent
- (void)showExtendedLaunchScreen:(SceneState*)sceneState {
  // Override to now show LaunchScreenViewController from Chromium
  UIViewController* controller = [[UIViewController alloc] init];
  controller.view.backgroundColor = [UIColor redColor];
  
  [sceneState setRootViewController:controller makeKeyAndVisible:YES];
}
@end
