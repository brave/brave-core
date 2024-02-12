// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import SwiftUI

extension UIImage {
  /// Creates an image object from BraveUI's shared bundle using the named image asset that is compatible with
  /// the specified trait collection.
  public convenience init?(
    sharedNamed name: String,
    compatibleWith traitCollection: UITraitCollection? = nil
  ) {
    self.init(named: name, in: .module, compatibleWith: traitCollection)
  }
}

extension Image {
  /// Creates a labeled image from BraveUI's shared bundle that you can use as content for controls.
  public init(sharedName name: String) {
    self.init(name, bundle: .module)
  }
}
