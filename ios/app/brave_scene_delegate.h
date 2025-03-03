#ifndef BRAVE_IOS_APP_BRAVE_SCENE_DELEGATE_H_
#define BRAVE_IOS_APP_BRAVE_SCENE_DELEGATE_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class BraveCoreMain;

OBJC_EXPORT
@interface BraveCoreSceneDelegate: NSObject<UIWindowSceneDelegate>
@property(nonatomic) UIWindow* window;

+ (BraveCoreMain*)braveCoreMain;

- (void)scene:(UIScene*)scene
    willConnectToSession:(UISceneSession*)session
      options:(UISceneConnectionOptions*)connectionOptions;

- (void)sceneDidDisconnect:(UIScene*)scene;

- (void)scene:(UIScene*)scene
    willConnectToSession:(UISceneSession*)session
      options:(UISceneConnectionOptions*)connectionOptions;

- (void)sceneDidDisconnect:(UIScene*)scene;

#pragma mark Transitioning to the Foreground

- (void)sceneWillEnterForeground:(UIScene*)scene;

- (void)sceneDidBecomeActive:(UIScene*)scene;

#pragma mark Transitioning to the Background

- (void)sceneWillResignActive:(UIScene*)scene;

- (void)sceneDidEnterBackground:(UIScene*)scene;

- (void)scene:(UIScene*)scene
openURLContexts:(NSSet<UIOpenURLContext*>*)URLContexts;

- (void)windowScene:(UIWindowScene*)windowScene
    performActionForShortcutItem:(UIApplicationShortcutItem*)shortcutItem
  completionHandler:(void (^)(BOOL succeeded))completionHandler;

- (void)scene:(UIScene*)scene
continueUserActivity:(NSUserActivity*)userActivity;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_APP_BRAVE_SCENE_DELEGATE_H_
