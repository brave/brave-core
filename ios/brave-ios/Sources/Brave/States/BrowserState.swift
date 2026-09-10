// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import Foundation
import Preferences
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
    for windowId: String,
    isPrivate: Bool? = nil
  ) -> NSUserActivity {
    return NSUserActivity(activityType: sceneId).then {
      $0.targetContentIdentifier = windowId
      var userInfo: [String: Any] = [
        SessionState.windowIDKey: windowId
      ]
      if let isPrivate {
        userInfo[SessionState.isPrivateKey] = isPrivate
      }
      $0.addUserInfoEntries(from: userInfo)
    }
  }

  public static func setWindowId(
    for activity: NSUserActivity,
    windowId: String,
    isPrivate: Bool? = nil
  ) {
    if activity.userInfo == nil {
      activity.userInfo = [:]
    }

    activity.targetContentIdentifier = windowId
    var userInfo: [String: Any] = [
      SessionState.windowIDKey: windowId
    ]
    if let isPrivate {
      userInfo[SessionState.isPrivateKey] = isPrivate
    }
    activity.addUserInfoEntries(from: userInfo)
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

  public static func windowUserActivity(from activities: Set<NSUserActivity>) -> NSUserActivity? {
    activities.first(where: { getNewWindowInfo(from: $0).windowId != nil }) ?? activities.first
  }

  public static func setWindowId(
    for session: UISceneSession,
    windowId: String,
    isPrivate: Bool? = nil
  ) {
    var userInfo: [String: Any] = [
      SessionState.windowIDKey: windowId
    ]
    if let isPrivate {
      userInfo[SessionState.isPrivateKey] = isPrivate
    }

    if session.userInfo == nil {
      session.userInfo = userInfo
    } else {
      session.userInfo?.merge(with: userInfo)
    }
  }

  public static func getSessionState(from session: UISceneSession) -> SessionState {
    guard let userInfo = session.userInfo else {
      return SessionState(windowId: nil, isPrivate: false, openURL: nil)
    }

    return SessionState(
      windowId: userInfo[SessionState.windowIDKey] as? String,
      isPrivate: userInfo[SessionState.isPrivateKey] as? Bool == true,
      openURL: nil
    )
  }

  /// True when the user opted to remember private mode and private tabs are still on disk.
  public static var shouldRestorePrivateBrowsingMode: Bool {
    shouldRestorePrivateBrowsingMode(windowId: nil)
  }

  /// True when remembered private mode should apply, optionally scoped to one window.
  public static func shouldRestorePrivateBrowsingMode(windowId: UUID?) -> Bool {
    guard Preferences.Privacy.persistentPrivateBrowsing.value,
      Preferences.Privacy.rememberBrowsingMode.value,
      Preferences.Privacy.lastPrivateBrowsingMode.value
    else {
      return false
    }
    return hasPrivateTabs(windowId: windowId)
  }

  /// Persists the browsing mode for scene restoration when "Remember Browsing Mode" is enabled.
  public static func persistBrowsingMode(
    session: UISceneSession,
    userActivity: NSUserActivity?,
    windowId: String,
    isPrivate: Bool
  ) {
    setWindowId(for: session, windowId: windowId, isPrivate: isPrivate)
    if let userActivity {
      setWindowId(for: userActivity, windowId: windowId, isPrivate: isPrivate)
    }
  }

  /// Persists the current browsing mode for relaunch when "Remember Browsing Mode" is enabled.
  public static func persistRememberedBrowsingMode(
    isPrivate: Bool,
    session: UISceneSession? = nil,
    userActivity: NSUserActivity? = nil,
    windowId: String? = nil
  ) {
    guard Preferences.Privacy.persistentPrivateBrowsing.value,
      Preferences.Privacy.rememberBrowsingMode.value
    else {
      return
    }

    Preferences.Privacy.lastPrivateBrowsingMode.value = isPrivate

    if let session, let windowId {
      persistBrowsingMode(
        session: session,
        userActivity: userActivity,
        windowId: windowId,
        isPrivate: isPrivate
      )
    }
  }

  public static func clearRememberedBrowsingMode() {
    Preferences.Privacy.lastPrivateBrowsingMode.value = false
  }

  public static func handleRememberBrowsingModeToggled(
    enabled: Bool,
    isCurrentlyPrivate: Bool
  ) {
    if enabled {
      persistRememberedBrowsingMode(isPrivate: isCurrentlyPrivate)
    } else {
      clearRememberedBrowsingMode()
    }
  }

  /// Resolves the window ID for a connecting scene.
  /// `userActivity` and session state win so a new iPad window is not collapsed onto the last one.
  public static func resolveWindowId(
    activityWindowId: UUID?,
    sessionWindowId: UUID?,
    lastSessionWindowId: UUID?,
    activeWindowId: UUID?,
    existingWindows: [(id: UUID, tabCount: Int)],
    claimedWindowIds: Set<UUID>,
    supportsMultipleScenes: Bool
  ) -> UUID {
    if let activityWindowId {
      return activityWindowId
    }
    if let sessionWindowId {
      return sessionWindowId
    }
    if !supportsMultipleScenes {
      if let lastSessionWindowId {
        return lastSessionWindowId
      }
      if let activeWindowId {
        return activeWindowId
      }
      if let windowWithTabs = existingWindows.max(by: { $0.tabCount < $1.tabCount }),
        windowWithTabs.tabCount > 0
      {
        return windowWithTabs.id
      }
      return existingWindows.first?.id ?? UUID()
    }
    // iPad: an unidentified scene must not adopt a window that another session already owns.
    if let activeWindowId, !claimedWindowIds.contains(activeWindowId) {
      return activeWindowId
    }
    return UUID()
  }

  /// Resolves whether a window should launch in private mode.
  public static func resolveLaunchIsPrivate(
    rememberBrowsingModeEnabled: Bool,
    privateBrowsingOnly: Bool,
    lastPrivateBrowsingMode: Bool,
    hasPrivateTabsInWindow: Bool,
    activityRequestsPrivate: Bool,
    selectedTabIsPrivate: Bool,
    windowHasOnlyPrivateTabs: Bool
  ) -> Bool {
    if privateBrowsingOnly {
      return true
    }
    if activityRequestsPrivate {
      return true
    }
    guard rememberBrowsingModeEnabled else {
      return false
    }
    if lastPrivateBrowsingMode && hasPrivateTabsInWindow {
      return true
    }
    return hasPrivateTabsInWindow && (selectedTabIsPrivate || windowHasOnlyPrivateTabs)
  }

  public static func hasPrivateTabs(windowId: UUID? = nil) -> Bool {
    SessionTab.all().contains { tab in
      guard tab.isPrivate else { return false }
      if let windowId {
        return tab.sessionWindow?.windowId == windowId
      }
      return true
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
