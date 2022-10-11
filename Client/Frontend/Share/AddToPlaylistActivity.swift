/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import UIKit

class AddToPlaylistActivity: UIActivity {
  fileprivate let callback: () -> Void

  init(callback: @escaping () -> Void) {
    self.callback = callback
  }

  override var activityTitle: String? {
    return Strings.PlayList.addToPlayListAlertTitle
  }

  override var activityImage: UIImage? {
    return UIImage(named: "playlist_menu", in: .module, compatibleWith: nil)!
  }

  override func perform() {
    callback()
    activityDidFinish(true)
  }

  override func canPerform(withActivityItems activityItems: [Any]) -> Bool {
    return true
  }
}

class OpenInPlaylistActivity: UIActivity {
  fileprivate let callback: () -> Void

  init(callback: @escaping () -> Void) {
    self.callback = callback
  }

  override var activityTitle: String? {
    return Strings.PlayList.toastExitingItemPlaylistTitle
  }

  override var activityImage: UIImage? {
    return UIImage(named: "playlist_menu", in: .module, compatibleWith: nil)!
  }

  override func perform() {
    callback()
    activityDidFinish(true)
  }

  override func canPerform(withActivityItems activityItems: [Any]) -> Bool {
    return true
  }
}
