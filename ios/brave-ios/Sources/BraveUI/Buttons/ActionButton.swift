// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

open class ActionButton: BraveButton {
  override open func updateConfiguration() {
    super.updateConfiguration()

    configuration?.cornerStyle = .capsule
  }
}
