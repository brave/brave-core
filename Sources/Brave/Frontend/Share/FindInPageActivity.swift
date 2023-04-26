/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import UIKit
import SwiftUI

class FindInPageActivity: UIActivity, MenuActivity {
  fileprivate let callback: () -> Void

  init(callback: @escaping () -> Void) {
    self.callback = callback
  }

  override var activityTitle: String? {
    return Strings.findInPage
  }

  override var activityImage: UIImage? {
    return UIImage(braveSystemNamed: "leo.search")?.applyingSymbolConfiguration(.init(scale: .large))
  }
  
  var menuImage: Image {
    Image(braveSystemName: "leo.search")
  }

  override func perform() {
    callback()
    activityDidFinish(true)
  }

  override func canPerform(withActivityItems activityItems: [Any]) -> Bool {
    return true
  }
}
