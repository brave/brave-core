// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

struct UX {
  struct TabsBar {
    static let isPad = UIDevice.current.userInterfaceIdiom == .pad
    static let buttonWidth: CGFloat = isPad ? 44 : 0
    static let height: CGFloat = isPad ? 40 : 29
    static let minimumWidth: CGFloat = isPad ? 200 : 160
    static let closeButtonWidth: CGFloat = isPad ? 44 : 30
    static let closeButtonSymbolPointSize: CGFloat = 20
  }
}
