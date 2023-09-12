// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import UIKit

extension Color {
  /// Initialize a `Color` with a color resource.
  public init(braveSystemName resource: FigmaColorResource) {
    self.init(resource.name, bundle: resource.bundle)
  }
}

extension UIColor {
  /// Initialize a `UIColor` with a color resource.
  public convenience init(braveSystemName resource: FigmaColorResource) {
    self.init(named: resource.name, in: resource.bundle, compatibleWith: nil)!
  }
}
