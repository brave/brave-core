// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// The content of a card placed in the Brave Today section on the NTP
protocol FeedCardContent {
    /// The content's view
    var view: UIView { get }
    /// An action handler to respond to users taps, menu items, etc.
    ///
    /// For single-item card's `itemIndex` will always be `0`, for multi-item cards
    /// such as group cards or the headline pairs, `itemIndex` will be based on
    /// the actioned upon feed item.
    var actionHandler: ((_ itemIndex: Int, _ action: FeedItemAction) -> Void)? { get set }
    /// Obtain the context menu to be used for the given item
    ///
    /// For single-item card's `itemIndex` will always be `0`, for multi-item cards
    /// such as group cards or the headline pairs, `itemIndex` will be based on
    /// the actioned upon feed item.
    var contextMenu: FeedItemMenu? { get set }
    
    /// A required initializer for making the cards content
    init()
}

extension FeedCardContent where Self: UIView {
    var view: UIView { self }
}

/// The actions you can perform on a feed item in the list
enum FeedItemAction: Equatable {
    /// The user choose to open the feed item in some way
    case opened(inNewTab: Bool = false, switchingToPrivateMode: Bool = false)
    /// Toggle the enabled status for the source of the feed item
    case toggledSource
    /// Handle a long press initiated by the user for legacy menus
    case longPressed(_ legacyContext: FeedItemMenu.LegacyContext)
}
