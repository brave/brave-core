/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import UIKit

class RequestDesktopSiteActivity: UIActivity {
  private weak var tab: Tab?
  fileprivate let callback: () -> Void

  init(tab: Tab?, callback: @escaping () -> Void) {
    self.tab = tab
    self.callback = callback
  }

  override var activityTitle: String? {
    tab?.isDesktopSite == true ? Strings.appMenuViewMobileSiteTitleString : Strings.appMenuViewDesktopSiteTitleString
  }

  override var activityImage: UIImage? {
    tab?.isDesktopSite == true ? UIImage(named: "shareRequestMobileSite", in: .module, compatibleWith: nil)! : UIImage(named: "shareRequestDesktopSite", in: .module, compatibleWith: nil)!
  }

  override func perform() {
    callback()
    activityDidFinish(true)
  }

  override func canPerform(withActivityItems activityItems: [Any]) -> Bool {
    return true
  }
}
