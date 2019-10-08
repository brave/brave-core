/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

extension UIImage {

  private class _BundleClass {}
  
  /// Get an image from the current framework's bundle
  convenience init(frameworkResourceNamed name: String) {
    self.init(named: name, in: Bundle(for: _BundleClass.self), compatibleWith: nil)!
  }
  
  var alwaysOriginal: UIImage {
    return withRenderingMode(.alwaysOriginal)
  }
  
  var alwaysTemplate: UIImage {
    return withRenderingMode(.alwaysTemplate)
  }
}
