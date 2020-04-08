/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import Shared

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
}

public extension PopoverContentComponent {
    var extendEdgeIntoArrow: Bool {
        return true
    }
    
    var isPanToDismissEnabled: Bool {
        return true
    }
    
    func popoverShouldDismiss(_ popoverController: PopoverController) -> Bool {
        return true
    }
    
    func popoverDidDismiss(_ popoverController: PopoverController) {
    }

    var closeActionAccessibilityLabel: String {
        return Strings.Popover.closeContextMenu
    }
}

public extension PopoverContentComponent where Self: UINavigationController {
    var extendEdgeIntoArrow: Bool {
        return false
    }
}
