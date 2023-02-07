/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit

struct UX {
  struct TabsBar {
    static let buttonWidth = UIDevice.current.userInterfaceIdiom == .pad ? 40 : 0
    static let height: CGFloat = 29
    static let minimumWidth: CGFloat = UIDevice.current.userInterfaceIdiom == .pad ? 180 : 160
  }
}
