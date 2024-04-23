// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

extension UIImpactFeedbackGenerator {
  @discardableResult
  public func vibrate() -> Self {
    self.prepare()
    self.impactOccurred()
    // Returned in case wanted for retaining to re-impact
    return self
  }
}

extension UINotificationFeedbackGenerator {
  @discardableResult
  public func vibrate(style: FeedbackType) -> Self {
    self.prepare()
    self.notificationOccurred(style)
    // Returned in case wanted for retaining to re-notify
    return self
  }
}

extension UISelectionFeedbackGenerator {
  @discardableResult
  public func vibrate() -> Self {
    self.prepare()
    self.selectionChanged()
    // Return in case wanted for retaining re-selection
    return self
  }
}
