// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

public class BrowserState {
  public static let sceneId = "com.brave.ios.browser-scene"

  let window: UIWindow
  let profile: LegacyBrowserProfile

  init(window: UIWindow, profile: LegacyBrowserProfile) {
    self.window = window
    self.profile = profile
  }

  public static func newWindowUserActivity(
    isPrivate: Bool,
    openURL: URL?
  ) -> NSUserActivity {
    let windowId = UUID().uuidString
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

  public static func userActivity(
    for windowId: String
  ) -> NSUserActivity {
    return NSUserActivity(activityType: sceneId).then {
      $0.targetContentIdentifier = windowId
      $0.addUserInfoEntries(from: [
        SessionState.windowIDKey: windowId
      ])
    }
  }

  public static func setWindowId(for activity: NSUserActivity, windowId: String) {
    if activity.userInfo == nil {
      activity.userInfo = [:]
    }

    activity.targetContentIdentifier = windowId
    activity.addUserInfoEntries(from: [
      SessionState.windowIDKey: windowId
    ])
  }

  public static func getWindowId(from session: UISceneSession) -> String? {
    guard let userInfo = session.userInfo else {
      return nil
    }
    return userInfo[SessionState.windowIDKey] as? String
  }

  public static func getNewWindowInfo(from activity: NSUserActivity) -> SessionState {
    guard let userInfo = activity.userInfo else {
      return SessionState(windowId: nil, isPrivate: false, openURL: nil)
    }

    return SessionState(
      windowId: userInfo[SessionState.windowIDKey] as? String,
      isPrivate: userInfo[SessionState.isPrivateKey] as? Bool == true,
      openURL: userInfo[SessionState.openURLKey] as? URL
    )
  }

  public static func setWindowId(for session: UISceneSession, windowId: String) {
    let userInfo: [String: Any] = [
      SessionState.windowIDKey: windowId
    ]

    if session.userInfo == nil {
      session.userInfo = userInfo
    } else {
      session.userInfo?.merge(with: userInfo)
    }
  }

  public struct SessionState {
    public var windowId: String?
    public var isPrivate: Bool
    public var openURL: URL?

    public init(windowId: String? = nil, isPrivate: Bool, openURL: URL? = nil) {
      self.windowId = windowId
      self.isPrivate = isPrivate
      self.openURL = openURL
    }

    static let windowIDKey = "WindowID"
    static let isPrivateKey = "isPrivate"
    static let openURLKey = "OpenURL"
  }
}
