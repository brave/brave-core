// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit

/// The context menu information
public class FeedItemMenu {
  /// A `UIMenu` to display when the user long-presses the feed item
  ///
  /// For single-item card's `itemIndex` will always be `0`, for multi-item cards
  /// such as group cards or the headline pairs, `itemIndex` will be based on
  /// the actioned upon feed item.
  public let menu: ((_ itemIndex: Int) -> UIMenu?)?

  public init(_ menu: @escaping (_ itemIndex: Int) -> UIMenu?) {
    self.menu = menu
  }
}
