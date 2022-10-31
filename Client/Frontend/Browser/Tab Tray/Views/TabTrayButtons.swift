/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import Shared
import BraveUI

extension UIButton {
  static func newTabButton() -> UIButton {
    let newTab = UIButton()
    newTab.setImage(UIImage(named: "quick_action_new_tab", in: .module, compatibleWith: nil)!.template, for: .normal)
    newTab.accessibilityLabel = Strings.tabTrayNewTabButtonAccessibilityLabel
    return newTab
  }
}
