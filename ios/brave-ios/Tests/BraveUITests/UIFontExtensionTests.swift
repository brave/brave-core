// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import UIKit
import XCTest

extension UIFont {
  fileprivate var weight: UIFont.Weight? {
    if let traits = fontDescriptor.object(forKey: .traits) as? [UIFontDescriptor.TraitKey: Any],
      let weight = traits[.weight] as? UIFont.Weight.RawValue
    {
      return UIFont.Weight(rawValue: weight)
    }
    return nil
  }
}

class UIFontExtensionTests: XCTestCase {
  func testPreferredFontWithWeightNoTraits() {
    let body = UIFont.preferredFont(forTextStyle: .body)
    let bodyMedium = UIFont.preferredFont(for: .body, weight: .medium)
    XCTAssertEqual(body.pointSize, bodyMedium.pointSize)
    XCTAssertEqual(bodyMedium.weight, .medium)
  }

  func testPreferredFontWithWeightCompatibleTraits() {
    let traits = UITraitCollection(preferredContentSizeCategory: .extraExtraLarge)
    let body = UIFont.preferredFont(forTextStyle: .body, compatibleWith: traits)
    let bodyMedium = UIFont.preferredFont(for: .body, weight: .medium, traitCollection: traits)
    XCTAssertEqual(body.pointSize, bodyMedium.pointSize)
    XCTAssertEqual(bodyMedium.weight, .medium)
  }
}
