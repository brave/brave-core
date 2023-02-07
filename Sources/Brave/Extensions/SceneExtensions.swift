// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

extension UIWindowScene {
  /// A single scene should only have ONE browserViewController
  /// However, it is possible that someone can create multiple,
  /// Therefore, we support this possibility if needed
  var browserViewControllers: [BrowserViewController] {
    windows.compactMap({
      $0.rootViewController as? UINavigationController
    }).flatMap({
      $0.viewControllers.compactMap({
        $0 as? BrowserViewController
      })
    })
  }
  
  /// A scene should only ever have one browserViewController
  /// Returns the first instance of `BrowserViewController` that is found in the current scene
  public var browserViewController: BrowserViewController? {
    return browserViewControllers.first
  }
}
