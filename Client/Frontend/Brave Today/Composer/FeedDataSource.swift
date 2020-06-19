// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Data
import Shared

private let logger = Logger.browserLogger

enum FeedCard {
    case sponsor(_ feed: FeedItem)
    case headline(_ feed: FeedItem)
    case headlinePair(_ feeds: (FeedItem, FeedItem))
    case group(_ feeds: [FeedItem], title: String, direction: NSLayoutConstraint.Axis, displayBrand: Bool)
    case numbered(_ feeds: [FeedItem], title: String)
}

//@propertyWrapper struct CompactMappedDecodableArray<T: Decodable>: Decodable {
//    var wrappedValue: [T]
//    init(from decoder: Decoder) throws {
//        var container = try decoder.unkeyedContainer()
//        var items: [T] = []
//        while !container.isAtEnd {
//            if let item = try? container.decode(T.self) {
//                items.append(item)
//            }
//        }
//        wrappedValue = items
//    }
//}

@propertyWrapper struct FailableDecodable<T: Decodable>: Decodable {
    var wrappedValue: T?
    init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        wrappedValue = try? container.decode(T.self)
    }
}

struct ScoredFeedItem: Equatable, Comparable {
    var score: Double
    var item: FeedItem
    
    static func < (lhs: Self, rhs: Self) -> Bool {
        return lhs.score < rhs.score
    }
}

/// Powers Brave Today's feed.
class FeedDataSource {
    private(set) var cards: [FeedCard] = []
    
    private let session = NetworkManager()
    private var feeds: [ScoredFeedItem] = []
    
    init() {
    }
    
    func load(_ completion: @escaping () -> Void) {
        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .formatted(DateFormatter().then {
            $0.dateFormat = "yyyy-MM-dd HH:mm:ss"
            $0.timeZone = TimeZone(secondsFromGMT: 0)
        })
        session.dataRequest(with: URL(string: "https://nlbtest.rapidpacket.com/mvp_200.json")!) { [weak self] data, response, error in
            guard let self = self, let data = data else { return }
            do {
                let decodedFeeds = try decoder.decode([FailableDecodable<FeedItem>].self, from: data).compactMap { $0.wrappedValue }
                DispatchQueue.main.async {
                    self.feeds = self.scored(feeds: decodedFeeds).sorted(by: <)
                    self.generateCards()
                    completion()
                }
            } catch {
                logger.error(String(describing: error))
                completion()
            }
        }
    }
    
    private func scored(feeds: [FeedItem]) -> [ScoredFeedItem] {
        let lastVisitedDomains = (try? History.suffix(200)
            .lazy
            .compactMap(\.url)
            .compactMap { URL(string: $0)?.baseDomain }) ?? []
        return feeds.map {
            let timeSincePublished = Double($0.publishTime.timeIntervalSinceNow)
            var score = timeSincePublished > 0 ? log(timeSincePublished) : 0
            if let feedBaseDomain = $0.url?.baseDomain, lastVisitedDomains.contains(feedBaseDomain) {
                score -= 5
            }
            return ScoredFeedItem(score: score, item: $0)
        }
    }
    
    private func generateCards() {
        var cards: [FeedCard] = []
        /**
         Beginning of new session:
            Sponsor card
            1st item, Large Card, Latest item from query
            Deals card
         */
        var deals = feeds.filter { $0.item.publisherID == "brave_offers" }
        var sponsors = feeds.filter { $0.item.contentType == "product" }
        var articles = feeds.filter { $0.item.contentType == "article" }
        var media = feeds.lazy.filter { $0.item.contentType == "image" }
        
        if !sponsors.isEmpty {
            cards.append(.sponsor(sponsors.removeFirst().item))
        }
        if !articles.isEmpty {
            cards.append(.headline(articles.removeFirst().item))
        }
        if !deals.isEmpty {
            let items = deals.prefix(3).map(\.item)
            cards.append(.group(items, title: "Deals", direction: .horizontal, displayBrand: false))
            deals.removeFirst(3)
        }
        
        /**
         Then infinitely repeating collections of content:
             17 Cards, 25 Content Items
             Rank content from collection query (See Weighting model)
             Distribute Large Image Cards across list for visual impact
             Group Small Cards in pairs in rows
             1x Vertical List Card from single source (3x Items), personalized or random
             1x Vertical List Card from a category (3x Items), don’t repeat category
             1x Deals Card (3x Items)
             2x Sponsored Content Card
             1x Large Image Card
             1x Vertical List Card (3x Items)
             Video Cards always Large, less than 3 per collection
         */
        
        do {
            // Cards 1 to 6: Latest items
            // - 2x Large Headline
            // - 2x Small Headline Pair
            let items = articles.prefix(6).map(\.item)
            cards.append(.headline(items[0]))
            cards.append(.headline(items[1]))
            cards.append(.headlinePair((items[2], items[3])))
            cards.append(.headlinePair((items[4], items[5])))
            articles.removeFirst(6)
        }
        
        do {
            // Card 7: Category of items
            // - Vertical List card, 3 stories from multiple sources of same category
            // - Don't repeat category until all have been used
            if let category = articles.first?.item.category {
                let items = Array(articles.lazy.filter({ $0.item.category == category }).prefix(3).map(\.item))
                cards.append(.group(items, title: category, direction: .vertical, displayBrand: false))
                articles.removeAll(where: { items.contains($0.item) })
            }
        }
        
        do {
            // Card 8 and 9: Commercial
            // - 1x sponsored card (large headline)
            // - 1x affiliate deals card
            cards.append(.sponsor(sponsors.removeFirst().item))
            let items = deals.prefix(3).map(\.item)
            cards.append(.group(items, title: "Deals", direction: .horizontal, displayBrand: false))
            deals.removeFirst(3)
        }
        
        self.cards = cards
    }
}
