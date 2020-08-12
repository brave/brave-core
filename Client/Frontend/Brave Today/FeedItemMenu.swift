// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// The context menu information
class FeedItemMenu {
    /// A `UIMenu` to display when the user long-presses the feed item
    ///
    /// For single-item card's `itemIndex` will always be `0`, for multi-item cards
    /// such as group cards or the headline pairs, `itemIndex` will be based on
    /// the actioned upon feed item.
    ///
    /// Note: Must be lazy to allow usage of `@available` on a stored property
    @available(iOS 13.0, *)
    lazy var menu: ((_ itemIndex: Int) -> UIMenu?)? = nil
    
    @available(iOS 13.0, *)
    init(_ menu: @escaping (_ itemIndex: Int) -> UIMenu?) {
        self.menu = menu
    }
    
    /// The legacy menu for iOS 12. Displays an action sheet
    ///
    /// For single-item card's `itemIndex` will always be `0`, for multi-item cards
    /// such as group cards or the headline pairs, `itemIndex` will be based on
    /// the actioned upon feed item.
    var legacyMenu: ((_ itemIndex: Int) -> LegacyContext?)?
    
    @available(iOS, deprecated: 13.0, message: "Use UIMenu based API through `init(_ menu:)`")
    init(_ legacyMenu: @escaping (_ itemIndex: Int) -> LegacyContext?) {
        self.legacyMenu = legacyMenu
    }
    
    /// A legacy context to use on iOS 12 for an action sheet
    struct LegacyContext: Equatable {
        /// Optionally, the alert title
        var title: String?
        /// Optionally, the alert message
        var message: String?
        /// A list of actions to display in the action sheet
        var actions: [UIAlertAction]
    }
}
