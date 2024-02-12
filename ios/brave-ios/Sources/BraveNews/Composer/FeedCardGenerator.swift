// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Collections
import OSLog
import UIKit
import BraveCore

/// Generates the cards that appear on the Brave News feed
///
/// This is an AsyncSequence, so cards will be generated seqeuntially in order one-by-one. You can get each
/// card by `await`ing on the sequence like so
///
/// ```
/// let generator = FeedCardGenerator(...)
/// for await card in generator {
///   // Use card
/// }
/// ```
struct FeedCardGenerator: AsyncSequence {
  typealias Element = [FeedCard]
  
  private var items: [FeedItem]
  private var sequence: [FeedSequenceElement]
  private var followedSources: Set<String>
  private var hiddenSources: Set<String>
  private var followedChannels: [String: Set<String>]
  private var ads: BraveAds?
  
  init(
    scoredItems: [FeedItem],
    sequence: [FeedSequenceElement],
    followedSources: Set<String>,
    hiddenSources: Set<String>,
    followedChannels: [String: Set<String>],
    ads: BraveAds?
  ) {
    self.items = scoredItems
    self.sequence = sequence
    self.followedSources = followedSources
    self.hiddenSources = hiddenSources
    self.followedChannels = followedChannels
    self.ads = ads
  }
  
  func makeAsyncIterator() -> AsyncIterator {
    var feedsFromEnabledSources = OrderedSet(items.filter { item in
      if item.source.isUserSource {
        return true
      }
      return followedSources.contains(item.source.id)
    })
    for (key, value) in followedChannels {
      let channelForLocale = Set(value)
      feedsFromEnabledSources.formUnion(OrderedSet(items.filter({ item in
        if hiddenSources.contains(item.source.id) {
          return false
        }
        return item.source.localeDetails?.contains(where: {
          $0.locale == key && !$0.channels.intersection(channelForLocale).isEmpty
        }) ?? false
      })))
    }
    let feedItems = feedsFromEnabledSources.sorted(by: <)
    let sponsors = feedItems.filter { $0.content.contentType == .sponsor }
    let partners = feedItems.filter { $0.content.contentType == .partner }
    let deals = feedItems.filter { $0.content.contentType == .deals }
    let articles = feedItems.filter { $0.content.contentType == .article }
    
    return AsyncIterator(
      sponsors: sponsors,
      deals: deals,
      partners: partners,
      articles: articles,
      sequence: sequence,
      ads: ads
    )
  }
}

extension FeedCardGenerator {
  struct AsyncIterator: AsyncIteratorProtocol {
    var sponsors: [FeedItem]
    var deals: [FeedItem]
    var partners: [FeedItem]
    var articles: [FeedItem]
    var sequence: [FeedSequenceElement]
    let ads: BraveAds?
    private let categoryGroupFillStrategy: CategoryFillStrategy<String>
    private let dealsFillStrategy: CategoryFillStrategy<String?>
    private var contentAdsQueryFailed: Bool = false
    private var inlineContentAdsPurged: Bool = false
    private var repeatingSequence: [FeedSequenceElement]?
    private var repeatedSequenceCardCount: Int = 0
    
    init(
      sponsors: [FeedItem],
      deals: [FeedItem],
      partners: [FeedItem],
      articles: [FeedItem],
      sequence: [FeedSequenceElement],
      ads: BraveAds? = nil
    ) {
      self.sponsors = sponsors
      self.deals = deals
      self.partners = partners
      self.articles = articles
      self.sequence = sequence
      self.ads = ads
      // These fill strategies currently require knowledge of the current set of filtered items so we'll
      // create them here instead and use them directly in `makeCards(for:fillStrategy:)`
      self.categoryGroupFillStrategy = CategoryFillStrategy(
        categories: Set(articles.map(\.source.category)),
        category: \.source.category,
        initialCategory: FeedDataSource.topNewsCategory
      )
      self.dealsFillStrategy = CategoryFillStrategy(
        categories: Set(deals.compactMap(\.content.offersCategory)),
        category: \.content.offersCategory
      )
    }
    
    mutating func next() async throws -> Element? {
      guard var rule = sequence.first else {
        return nil
      }
      try Task.checkCancellation()
      sequence.removeFirst()
      // When we hit a infinitely repeating sequence (with at least one element), replace the current sequence
      // with that one.
      //
      // If we needed to support multiple infinite repeating we'll need to implement some sort of push/pop
      // version of this.
      if case .repeating(let repeatingSequence, let count) = rule,
         count == .max,
         let firstRepeatedRule = repeatingSequence.first {
        sequence = repeatingSequence
        self.repeatingSequence = repeatingSequence
        rule = firstRepeatedRule
      }
      let cards = await makeCards(for: rule, fillStrategy: DefaultFillStrategy()) ?? []
      if let repeatingSequence {
        repeatedSequenceCardCount += cards.count
        // Hit the end of the repeating sequence, check if we added any cards, if not then we're done
        if sequence.isEmpty {
          if repeatedSequenceCardCount == 0 {
            return nil
          }
          sequence = repeatingSequence
          repeatedSequenceCardCount = 0
        }
      }
      return cards
    }
    
    private mutating func makeCards(
      for element: FeedSequenceElement,
      fillStrategy: FillStrategy
    ) async -> [FeedCard]? {
      switch element {
      case .sponsor:
        return fillStrategy.next(from: &sponsors).map {
          [.sponsor($0)]
        }
      case .deals:
        return dealsFillStrategy.next(3, from: &deals).map {
          let title = $0.first?.content.offersCategory
          return [.deals($0, title: title)]
        }
      case .partner:
        let imageExists = { (item: FeedItem) -> Bool in
          item.content.imageURL != nil
        }
        return fillStrategy.next(from: &partners, where: imageExists).map {
          [.partner($0)]
        }
      case .braveAd:
        // If we fail to obtain inline content ads during a card gen it can be assumed that
        // all further calls will fail since cards are generated all at once
        guard !contentAdsQueryFailed, let ads = ads, ads.isServiceRunning() else { return [] }
        if !inlineContentAdsPurged {
          inlineContentAdsPurged = true
          _ = await Task { @MainActor in
            await ads.purgeOrphanedAdEvents(for: .inlineContentAd)
          }.value
        }
        let contentAd = await Task { @MainActor in
          await ads.maybeServeInlineContentAd("900x750").1
        }.value
        guard let ad = contentAd else {
          contentAdsQueryFailed = true
          Logger.module.debug("Inline content ads could not be filled; Skipping for the rest of this feed generation")
          return []
        }
        return [.ad(ad)]
      case .headline(let paired):
        if articles.isEmpty { return [] }
        let imageExists = { (item: FeedItem) -> Bool in
          item.content.imageURL != nil
        }
        if paired {
          if articles.count < 2 {
            return []
          }
          return fillStrategy.next(2, from: &articles, where: imageExists).map {
            [.headlinePair(.init($0[0], $0[1]))]
          }
        } else {
          return fillStrategy.next(from: &articles, where: imageExists).map {
            [.headline($0)]
          }
        }
      case .headlineRating:
        if articles.isEmpty { return [] }
        let imageExists = { (item: FeedItem) -> Bool in
          item.content.imageURL != nil
        }
        return fillStrategy.next(from: &articles, where: imageExists).map {
          [.headlineRatingCardPair($0)]
        }
      case .categoryGroup:
        return categoryGroupFillStrategy.next(3, from: &articles).map {
          let title = $0.first?.source.category ?? ""
          return [.group($0, title: title, direction: .vertical, displayBrand: false)]
        }
      case .brandedGroup(let numbered):
        if let item = fillStrategy.next(from: &articles) {
          return fillStrategy.next(2, from: &articles, where: { $0.source == item.source }).map {
            let items = [item] + $0
            if numbered {
              return [.numbered(items, title: item.source.name)]
            } else {
              return [.group(items, title: "", direction: .vertical, displayBrand: true)]
            }
          }
        }
        return []
      case .group:
        return fillStrategy.next(3, from: &articles).map {
          [.group($0, title: "", direction: .vertical, displayBrand: false)]
        }
      case .fillUsing(let strategy, let fallbackStrategy, let elements):
        var cards: [FeedCard] = []
        for element in elements {
          if let elementCards = await makeCards(for: element, fillStrategy: strategy) {
            cards.append(contentsOf: elementCards)
          } else {
            if let fallbackStrategy, let elementCards = await makeCards(for: element, fillStrategy: fallbackStrategy) {
              cards.append(contentsOf: elementCards)
            }
          }
        }
        return cards
      case .repeating(let elements, let times):
        var index = 0
        var cards: [FeedCard] = []
        repeat {
          var repeatedCards: [FeedCard] = []
          for element in elements {
            if let elementCards = await makeCards(for: element, fillStrategy: fillStrategy) {
              repeatedCards.append(contentsOf: elementCards)
            }
          }
          if repeatedCards.isEmpty {
            // Couldn't fill any of the cards so no reason to continue
            break
          }
          cards.append(contentsOf: repeatedCards)
          index += 1
        } while !articles.isEmpty && index < times
        return cards
      }
    }
  }
}
