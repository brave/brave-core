// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import BraveCore

/// A set of 2 items
public struct FeedPair: Hashable {
  /// The first item
  public var first: FeedItem
  /// The second item
  public var second: FeedItem

  init(_ first: FeedItem, _ second: FeedItem) {
    self.first = first
    self.second = second
  }
}

/// A container for one or many `FeedItem`s
public enum FeedCard: Hashable {
  /// A sponsored image to display
  case sponsor(_ feed: FeedItem)
  /// A group of deals/offers displayed horizontally
  case deals(_ feeds: [FeedItem], title: String?)
  /// A brave partner item
  case partner(_ feed: FeedItem)
  /// A brave display ad card
  case ad(InlineContentAd)
  /// A single item displayed prompinently with an image
  case headline(_ feed: FeedItem)
  /// A pair of `headline` items that should be displayed side by side horizontally with equal sizes
  case headlinePair(_ pair: FeedPair)
  /// A pair of `headline` and `rating card` items where first item is a headline and second item is rating card
  case headlineRatingCardPair(_ feed: FeedItem)
  /// A group of items that can be displayed in a number of different configurations
  case group(_ feeds: [FeedItem], title: String, direction: NSLayoutConstraint.Axis, displayBrand: Bool)
  /// A numbered group of items which will always be displayed in a vertical list.
  case numbered(_ feeds: [FeedItem], title: String)
  
  /// Obtain an estimated height for this card given a width it will be displayed with
  public func estimatedHeight(for width: CGFloat) -> CGFloat {
    switch self {
    case .sponsor:
      return FeedItemView.Layout.bannerThumbnail.estimatedHeight(for: width)
    case .headline:
      return FeedItemView.Layout.brandedHeadline.estimatedHeight(for: width)
    case .partner:
      return FeedItemView.Layout.partner.estimatedHeight(for: width)
    case .ad:
      return FeedItemView.Layout.ad.estimatedHeight(for: width)
    case .headlinePair, .headlineRatingCardPair:
      return 300
    case .group, .numbered, .deals:
      return 400
    }
  }

  /// A list of feed items that are present in the card
  public var items: [FeedItem] {
    switch self {
    case .headline(let item), .sponsor(let item), .partner(let item):
      return [item]
    case .headlinePair(let pair):
      return [pair.first, pair.second]
    case .headlineRatingCardPair(let item):
      return [item]
    case .group(let items, _, _, _), .numbered(let items, _), .deals(let items, _):
      return items
    case .ad:
      return []
    }
  }

  /// Creates a new card that has replaced an item it is displaying with a replacement
  ///
  /// If `item` is not being displayed by this card this function returns itself
  public func replacing(item: FeedItem, with replacementItem: FeedItem) -> Self {
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
    case .headlineRatingCardPair:
      return .headlineRatingCardPair(replacementItem)
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
    case .partner:
      return .partner(replacementItem)
    case .ad(let ad):
      return .ad(ad)
    }
  }
}
