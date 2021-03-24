// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

/// The Core NFC APIs don't properly notify when the system Core NFC action sheet goes away. This issue
/// is present since iOS 11, when the NDEFReaderSession was introduced, but Apple didn't provide since then
/// a notification or other mechanism to detect this. To overcome this the SceneObserver uses a newly
/// introduced notification in iOS 13, UISceneDidActivateNotification, to detect when the application scene
/// becomes again active after a system scene is dismissed.
///
/// The Apple documentation states:
///  "UIKit posts this notification for temporary interruptions, such as when displaying system alerts.".
///  https://developer.apple.com/documentation/uikit/uiscenewilldeactivatenotification?language=objc

class SceneObserver: NSObject {

    private(set) static var applicationWindow: UIWindow!
    
    // This is set only once in the AppDelegate.
    class func setupApplication(window: UIWindow) {
        applicationWindow = window
    }

    private var window: UIWindow
    private var sceneActivationClosure: () -> Void
    
    // MARK: - Object lifecycle
    init(sceneActivationClosure: @escaping () -> Void) {
        self.window = SceneObserver.applicationWindow
        self.sceneActivationClosure = sceneActivationClosure
        super.init()
        addSceneOnservation()
    }
    
    deinit {
        removeSceneOnservation()
    }
    
    // MARK: - Scene Observation
    
    private var sceneDidActivateNotificationToken: NSObjectProtocol?
    
    private func addSceneOnservation() {
        guard sceneDidActivateNotificationToken == nil else {
            return
        }
        let center = NotificationCenter.default
        let mainQueue = OperationQueue.main
        
        sceneDidActivateNotificationToken = center.addObserver(forName: UIScene.didActivateNotification, object: nil, queue: mainQueue) { [weak self] notification in
            guard let self = self else {
                return
            }
            guard let scene = notification.object as? UIScene else {
                return
            }
            if self.window.windowScene == scene {
                self.sceneActivationClosure()
            }
        }
    }
    
    private func removeSceneOnservation() {
        guard let token = sceneDidActivateNotificationToken else {
            return
        }
        let center = NotificationCenter.default
        center.removeObserver(token)
    }
}
