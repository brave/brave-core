// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

/// A set of 2 items
struct FeedPair: Equatable {
    /// The first item
    var first: FeedItem
    /// The second item
    var second: FeedItem
    
    init(_ first: FeedItem, _ second: FeedItem) {
        self.first = first
        self.second = second
    }
}

/// A container for one or many `FeedItem`s
enum FeedCard: Equatable {
    /// A sponsored image to display
    case sponsor(_ feed: FeedItem)
    /// A group of deals/offers displayed horizontally
    case deals(_ feeds: [FeedItem], title: String)
    /// A single item displayed prompinently with an image
    case headline(_ feed: FeedItem)
    /// A pair of `headline` items that should be displayed side by side horizontally with equal sizes
    case headlinePair(_ pair: FeedPair)
    /// A group of items that can be displayed in a number of different configurations
    case group(_ feeds: [FeedItem], title: String, direction: NSLayoutConstraint.Axis, displayBrand: Bool)
    /// A numbered group of items which will always be displayed in a vertical list.
    case numbered(_ feeds: [FeedItem], title: String)
    
    /// Obtain an estimated height for this card given a width it will be displayed with
    func estimatedHeight(for width: CGFloat) -> CGFloat {
        switch self {
        case .sponsor:
            return FeedItemView.Layout.bannerThumbnail.estimatedHeight(for: width)
        case .headline:
            return FeedItemView.Layout.brandedHeadline.estimatedHeight(for: width)
        case .headlinePair:
            return 300
        case .group, .numbered, .deals:
            return 400
        }
    }
    
    /// A list of feed items that are present in the card
    var items: [FeedItem] {
        switch self {
        case .headline(let item), .sponsor(let item):
            return [item]
        case .headlinePair(let pair):
            return [pair.first, pair.second]
        case .group(let items, _, _, _), .numbered(let items, _), .deals(let items, _):
            return items
        }
    }
    
    /// Creates a new card that has replaced an item it is displaying with a replacement
    ///
    /// If `item` is not being displayed by this card this function returns itself
    func replacing(item: FeedItem, with replacementItem: FeedItem) -> Self {
        if !items.contains(item) { return self }
        switch self {
        case .headline:
            return .headline(replacementItem)
        case .headlinePair(let pair):
            if pair.first == item {
                return .headlinePair(.init(replacementItem, pair.second))
            } else {
                return .headlinePair(.init(pair.first, replacementItem))
            }
        case .sponsor:
            return .sponsor(replacementItem)
        case .numbered(var feeds, let title):
            if let matchedItemIndex = feeds.firstIndex(of: item) {
                feeds[matchedItemIndex] = replacementItem
                return .numbered(feeds, title: title)
            }
            return self
        case .group(var feeds, let title, let direction, let displayBrand):
            if let matchedItemIndex = feeds.firstIndex(of: item) {
                feeds[matchedItemIndex] = replacementItem
                return .group(feeds, title: title, direction: direction, displayBrand: displayBrand)
            }
            return self
        case .deals(var feeds, let title):
            if let matchedItemIndex = feeds.firstIndex(of: item) {
                feeds[matchedItemIndex] = replacementItem
                return .deals(feeds, title: title)
            }
            return self
        }
    }
}
