// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

public class BrowserState {
  public static let sceneId = "com.brave.ios.browser-scene"
  
  let window: UIWindow
  let profile: Profile
  
  init(window: UIWindow, profile: Profile) {
    self.window = window
    self.profile = profile
  }
  
  public static func userActivity(for windowId: String, isPrivate: Bool, openURL: URL? = nil) -> NSUserActivity {
    return NSUserActivity(activityType: sceneId).then {
      $0.targetContentIdentifier = windowId
      $0.addUserInfoEntries(from: [
        SessionState.windowIDKey: windowId,
        SessionState.isPrivateKey: isPrivate,
      ])
      
      if let openURL = openURL {
        $0.addUserInfoEntries(from: [
          SessionState.openURLKey: openURL
        ])
      }
    }
  }
  
  public static func setWindowInfo(for activity: NSUserActivity, windowId: String, isPrivate: Bool) {
    if activity.userInfo == nil {
      activity.userInfo = [:]
    }
    
    activity.targetContentIdentifier = windowId
    activity.addUserInfoEntries(from: [
      SessionState.windowIDKey: windowId,
      SessionState.isPrivateKey: isPrivate
    ])
  }
  
  public static func getWindowInfo(from session: UISceneSession) -> SessionState {
    guard let userInfo = session.userInfo else {
      return SessionState(windowId: nil, isPrivate: false, openURL: nil)
    }
    
    return SessionState(windowId: userInfo[SessionState.windowIDKey] as? String,
                        isPrivate: userInfo[SessionState.isPrivateKey] as? Bool == true,
                        openURL: userInfo[SessionState.openURLKey] as? URL)
  }
  
  public static func getWindowInfo(from activity: NSUserActivity) -> SessionState {
    guard let userInfo = activity.userInfo else {
      return SessionState(windowId: nil, isPrivate: false, openURL: nil)
    }
    
    return SessionState(windowId: userInfo[SessionState.windowIDKey] as? String,
                        isPrivate: userInfo[SessionState.isPrivateKey] as? Bool == true,
                        openURL: userInfo[SessionState.openURLKey] as? URL)
  }
  
  public static func setWindowInfo(for session: UISceneSession, windowId: String, isPrivate: Bool) {
    let userInfo: [String: Any] = [
      SessionState.windowIDKey: windowId,
      SessionState.isPrivateKey: isPrivate
    ]
    
    if session.userInfo == nil {
      session.userInfo = userInfo
    } else {
      session.userInfo?.merge(with: userInfo)
    }
  }
  
  public struct SessionState {
    public let windowId: String?
    public let isPrivate: Bool
    public let openURL: URL?
    
    static let windowIDKey = "WindowID"
    static let isPrivateKey = "isPrivate"
    static let openURLKey = "OpenURL"
  }
}
