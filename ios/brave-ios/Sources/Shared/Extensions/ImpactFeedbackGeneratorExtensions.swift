// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

extension UIImpactFeedbackGenerator {
  @discardableResult
  public func bzzt() -> Self {
    self.prepare()
    self.impactOccurred()
    // Returned in case wanted for retaining to re-impact
    return self
  }
}
