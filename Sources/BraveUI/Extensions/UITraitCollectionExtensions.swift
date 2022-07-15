// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

extension UITraitCollection {
  
  /// Create a new UITraitCollection by clamping the current trait collection so that the preferred size
  /// category is always at least the specified size category
  public func clampingSizeCategory(minimum: UIContentSizeCategory) -> Self {
    _clampingSizeCategory(minimum: minimum, maximum: nil)
  }
  
  /// Create a new UITraitCollection by clamping the current trait collection so that the preferred size
  /// category is always at most the specified size category
  public func clampingSizeCategory(maximum: UIContentSizeCategory) -> Self {
    _clampingSizeCategory(minimum: nil, maximum: maximum)
  }
  
  /// Create a new UITraitCollection by clamping the current trait collection so that the preferred size
  /// category is always at least the specified size category but not exeed the provided maximum size category
  private func clampingSizeCategory(minimum: UIContentSizeCategory, maximum: UIContentSizeCategory) -> Self {
    _clampingSizeCategory(minimum: minimum, maximum: maximum)
  }
  
  private func _clampingSizeCategory(
    minimum: UIContentSizeCategory? = nil,
    maximum: UIContentSizeCategory? = nil
  ) -> Self {
    let minCategory = minimum ?? .extraSmall
    let maxCategory = maximum ?? .accessibilityExtraExtraExtraLarge
    let clampedCategory = preferredContentSizeCategory < minCategory ? minCategory : preferredContentSizeCategory > maxCategory ? maxCategory : preferredContentSizeCategory
    return .init(preferredContentSizeCategory: clampedCategory)
  }
}
