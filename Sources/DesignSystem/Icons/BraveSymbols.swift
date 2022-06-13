// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SwiftUI

extension UIImage {
  /// Creates an image object from the Design System bundle using the named image asset that is compatible
  /// with the specified trait collection.
  public convenience init?(
    braveSystemNamed name: String,
    compatibleWith traitCollection: UITraitCollection? = nil
  ) {
    self.init(named: name, in: .current, compatibleWith: traitCollection)
  }
}

extension Image {
  /// Creates a labeled image from the Design System bundle that you can use as content for controls.
  public init(braveSystemName name: String) {
    self.init(name, bundle: .current)
  }
}
