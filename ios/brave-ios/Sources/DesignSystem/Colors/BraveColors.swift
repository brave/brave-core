// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import NalaAssets
import SwiftUI
import UIKit

extension Color {
  /// Initialize a `Color` with a color resource.
  public init(braveSystemName resource: FigmaColorResource) {
    self.init(resource.name, bundle: .nalaAssets)
  }
}

extension UIColor {
  /// Initialize a `UIColor` with a color resource.
  public convenience init(braveSystemName resource: FigmaColorResource) {
    self.init(named: resource.name, in: .nalaAssets, compatibleWith: nil)!
  }
}
