// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Strings
import UIKit

/// Defines behavior of a component which will be used with a `PopoverController`
public protocol PopoverContentComponent {
  /// Whether or not the controller's frame begins at 0 (allowing it to bleed into the arrow) or start under the arrow
  ///
  /// Use safeAreaLayoutGuide to constrain content within the popover content view
  var extendEdgeIntoArrow: Bool { get }
  /// Whether or not the pan to dismiss gesture is enabled. Optional, true by defualt
  var isPanToDismissEnabled: Bool { get }
  /// Allows the component to decide whether or not the popover should dismiss based on some gestural action (tapping
  /// the background around the popover or dismissing via pan). Optional, true by defualt
  func popoverShouldDismiss(_ popoverController: PopoverController) -> Bool

  /// Description for closing the popover view for accessibility users
  var closeActionAccessibilityLabel: String { get }

  /// The background color applied to the popover
  var popoverBackgroundColor: UIColor { get }
}

extension PopoverContentComponent {
  public var extendEdgeIntoArrow: Bool {
    return true
  }

  public var isPanToDismissEnabled: Bool {
    return true
  }

  public func popoverShouldDismiss(_ popoverController: PopoverController) -> Bool {
    return true
  }

  public func popoverDidDismiss(_ popoverController: PopoverController) {
  }

  public var closeActionAccessibilityLabel: String {
    return NSLocalizedString(
      "PopoverDefaultClose",
      bundle: .module,
      value: "Close Context Menu",
      comment: "Description for closing a popover menu that is displayed."
    )
  }

  public var popoverBackgroundColor: UIColor {
    .braveBackground
  }
}

extension PopoverContentComponent where Self: UINavigationController {
  public var extendEdgeIntoArrow: Bool {
    return false
  }
}
