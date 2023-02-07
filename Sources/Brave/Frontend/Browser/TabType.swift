// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

enum TabType: Int, CustomDebugStringConvertible {

  /// Regular browsing.
  case regular

  /// Private browsing.
  case `private`

  /// Textual representation suitable for debugging.
  var debugDescription: String {
    switch self {
    case .regular:
      return "Regular Tab"
    case .private:
      return "Private Tab"
    }
  }

  /// Returns whether the tab is private or not.
  var isPrivate: Bool {
    switch self {
    case .regular:
      return false
    case .private:
      return true
    }
  }

  /// Returns the type of the given Tab, if the tab is nil returns a regular tab type.
  ///
  /// - parameter tab: An object representing a Tab.
  /// - returns: A Tab type.
  static func of(_ tab: Tab?) -> TabType {
    return tab?.type ?? .regular
  }

}
