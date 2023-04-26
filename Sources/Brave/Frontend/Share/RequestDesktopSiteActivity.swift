/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import UIKit
import SwiftUI

class RequestDesktopSiteActivity: UIActivity, MenuActivity {
  private weak var tab: Tab?
  fileprivate let callback: () -> Void

  init(tab: Tab?, callback: @escaping () -> Void) {
    self.tab = tab
    self.callback = callback
  }

  override var activityTitle: String? {
    tab?.isDesktopSite == true ? Strings.appMenuViewMobileSiteTitleString : Strings.appMenuViewDesktopSiteTitleString
  }
  
  private var imageName: String {
    tab?.isDesktopSite == true ? "leo.smartphone" : "leo.monitor"
  }

  override var activityImage: UIImage? {
    UIImage(braveSystemNamed: imageName)?.applyingSymbolConfiguration(.init(scale: .large))
  }
  
  var menuImage: Image {
    Image(braveSystemName: imageName)
  }

  override func perform() {
    callback()
    activityDidFinish(true)
  }

  override func canPerform(withActivityItems activityItems: [Any]) -> Bool {
    return true
  }
}
