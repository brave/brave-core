/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

/// Defines something that be used as an option type where theres usually a static set of options in a selectable list
public protocol RepresentableOptionType: Equatable {
  /// A key that can be used to define this option type
  var key: String { get }
  /// The string to show to users when presenting this option
  var displayString: String { get }
  /// An image to display next to the option
  var image: UIImage? { get }
}

// Default to no images
extension RepresentableOptionType {
  public var image: UIImage? {
    return nil
  }
}

/// Automatically infer `key` and equality when Self already provides a rawValue (mostly String enum's)
extension RepresentableOptionType where Self: RawRepresentable, Self.RawValue: Equatable {

  public var key: String {
    return String(describing: rawValue)
  }

  public static func == (lhs: Self, rhs: Self) -> Bool {
    return lhs.rawValue == rhs.rawValue
  }
}
