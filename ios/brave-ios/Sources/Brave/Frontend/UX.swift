// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

struct UX {
  struct TabsBar {
    static let isPad = UIDevice.current.userInterfaceIdiom == .pad
    static let buttonWidth: CGFloat = isPad ? 44 : 0
    static let height: CGFloat = isPad ? 34 : 29
    static let minimumWidth: CGFloat = isPad ? 192 : 160
    static let closeButtonWidth: CGFloat = isPad ? 36 : 30
    static let closeButtonImageInset: CGFloat = isPad ? 8 : 6
    static let closeButtonImageScale: CGFloat = isPad ? 1.2 : 1
  }
}
