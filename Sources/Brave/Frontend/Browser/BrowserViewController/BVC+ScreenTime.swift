// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences

extension BrowserViewController {
  /// Updates the url of the screen time controller. Keep in mind a single screen time controller is used for every window.
  /// Multiple instances of it seem to crash the app.
  /// There is also one hack required: STWebpageController breaks if you pass scheme other than http or https,
  /// it will not block anything for the rest of its lifecycle. Our internal urls have to be bridged to an empty https url.
  func updateScreenTimeUrl(_ url: URL?) {
    guard let url = url, (url.scheme == "http" || url.scheme == "https") else {
      screenTimeViewController?.url = URL(string: "https://about:blank")
      return
    }
    
    screenTimeViewController?.url = url
  }
  
  func recordScreenTimeUsage(for tab: Tab) {
    screenTimeViewController?.suppressUsageRecording = tab.isPrivate || !Preferences.Privacy.screenTimeEnabled.value
  }
}
