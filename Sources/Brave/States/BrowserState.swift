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
  
  public static func userActivity(for windowId: UUID, isPrivate: Bool) -> NSUserActivity {
    return NSUserActivity(activityType: sceneId).then {
      $0.targetContentIdentifier = windowId.uuidString
      $0.addUserInfoEntries(from: [
        "WindowID": windowId.uuidString,
        "isPrivate": isPrivate
      ])
    }
  }
}
