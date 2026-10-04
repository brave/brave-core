// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit

extension UIBarButtonItem {
  /// Creates a done button that is themed correctly
  public static func doneButton(_ action: @escaping () -> Void) -> UIBarButtonItem {
    let item = UIBarButtonItem(
      systemItem: .done,
      primaryAction: .init(handler: { _ in
        action()
      })
    )
    if #available(iOS 26.0, *) {
      // Liquid Glass will turn this bar button into a prominent glass button by default, so we
      // want to use an explicit tint color
      item.tintColor = UIColor(braveSystemName: .primitivePrimary40)
    }
    return item
  }
  /// Creates a done button that is themed correctly
  public static func doneButton(target: Any?, action: Selector?) -> UIBarButtonItem {
    let item = UIBarButtonItem(barButtonSystemItem: .done, target: target, action: action)
    if #available(iOS 26.0, *) {
      // Liquid Glass will turn this bar button into a prominent glass button by default, so we
      // want to use an explicit tint color
      item.tintColor = UIColor(braveSystemName: .primitivePrimary40)
    }
    return item
  }
}
