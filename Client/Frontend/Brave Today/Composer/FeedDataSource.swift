// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Data
import Shared

private let logger = Logger.browserLogger

struct FeedPair: Equatable {
    var first: FeedItem
    var second: FeedItem
    init(_ first: FeedItem, _ second: FeedItem) {
        self.first = first
        self.second = second
    }
}

enum FeedCard: Equatable {
    case sponsor(_ feed: FeedItem)
    case headline(_ feed: FeedItem)
    case headlinePair(_ pair: FeedPair)
    case group(_ feeds: [FeedItem], title: String, direction: NSLayoutConstraint.Axis, displayBrand: Bool)
    case numbered(_ feeds: [FeedItem], title: String)
    
    func estimatedHeight(for width: CGFloat) -> CGFloat {
        switch self {
        case .sponsor:
            return FeedItemView.Layout.bannerThumbnail.estimatedHeight(for: width)
        case .headline:
            return FeedItemView.Layout.brandedHeadline.estimatedHeight(for: width)
        case .headlinePair:
            return 300
        case .group, .numbered:
            return 400
        }
    }
}

// TODO: Move this to its own file along with `URLString`
@propertyWrapper private struct FailableDecodable<T: Decodable>: Decodable {
    var wrappedValue: T?
    init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        wrappedValue = try? container.decode(T.self)
    }
}

enum FeedSequenceElement {
    /// Display a sponsored image with the content type of `product`
    case sponsor
    /// Displays a horizontal list of deals with the content type of `brave_offers`
    case deals
    /// Displays an `article` type item in a headline card. Can also be displayed as two (smaller) paired
    /// headlines
    case headline(paired: Bool)
    /// Displays a list of `article` typed items with the same category in a vertical list.
    case categoryGroup
    /// Displays a list of `article` typed items with the same source. It can optionally be displayed as a
    /// numbered list
    case brandedGroup(numbered: Bool = false)
    /// Displays a list of `article` typed items that can have different categories and different sources.
    case group
    /// Displays the provided elements a number of times. Passing in `.max` for `times` means it will repeat
    /// until there is no more content available
    indirect case repeating([FeedSequenceElement], times: Int = .max)
}

/// Powers Brave Today's feed.
class FeedDataSource {
    private(set) var cards: [FeedCard] = []
    
    private let session = NetworkManager(session: URLSession(configuration: .ephemeral))
    private var feeds: [FeedItem] = []
    private var sources: [String: FeedItem.Source] = [:]
    private var hiddenItems: Set<String> = []
    
    init() {
    }
    
    func load(_ completion: @escaping () -> Void) {
        if !cards.isEmpty {
            return
        }
        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .formatted(DateFormatter().then {
            $0.dateFormat = "yyyy-MM-dd HH:mm:ss"
            $0.timeZone = TimeZone(secondsFromGMT: 0)
        })
        session.dataRequest(with: URL(string: "https://pcdn.brave.software/brave-today/sources.json")!) { [weak self] data, response, error in
            guard let self = self, let data = data else { return }
            do {
                let decodedSources = try decoder.decode([String: FailableDecodable<FeedItem.Source>].self, from: data).compactMapValues({ $0.wrappedValue })
                self.sources = decodedSources
                self.session.dataRequest(with: URL(string: "https://pcdn.brave.software/brave-today/feed.json")!) { [weak self] data, response, error in
                    guard let self = self, let data = data else { return }
                    do {
                        let decodedFeeds = try decoder.decode([FailableDecodable<FeedItem.Content>].self, from: data).compactMap { $0.wrappedValue }
                        DispatchQueue.main.async {
                            self.feeds = self.scored(feeds: decodedFeeds, sources: decodedSources).sorted(by: <)
                            self.generateCards(completion)
                        }
                    } catch {
                        logger.error(error)
                        DispatchQueue.main.async {
                            completion()
                        }
                    }
                }
            } catch {
                logger.error(error)
                DispatchQueue.main.async {
                    completion()
                }
            }
        }
    }
    
    /// Hide a specific feed item from a card showing in the list
    func hide(item: FeedItem, in card: FeedCard) {
        hiddenItems.insert(item.content.urlHash)
        if let cardIndex = cards.firstIndex(of: card) {
            var alteredItem = item
            alteredItem.isContentHidden = true
            switch card {
            case .headline:
                cards[cardIndex] = .headline(alteredItem)
            case .headlinePair(let pair):
                if pair.first == item {
                    cards[cardIndex] = .headlinePair(.init(alteredItem, pair.second))
                } else {
                    cards[cardIndex] = .headlinePair(.init(pair.first, alteredItem))
                }
            case .sponsor:
                cards[cardIndex] = .sponsor(alteredItem)
            case .numbered(var feeds, let title):
                if let matchedItemIndex = feeds.firstIndex(of: item) {
                    feeds[matchedItemIndex] = alteredItem
                    cards[cardIndex] = .numbered(feeds, title: title)
                }
            case .group(var feeds, let title, let direction, let displayBrand):
                if let matchedItemIndex = feeds.firstIndex(of: item) {
                    feeds[matchedItemIndex] = alteredItem
                    cards[cardIndex] = .group(feeds, title: title, direction: direction, displayBrand: displayBrand)
                }
            }
        }
    }
    
    private func scored(feeds: [FeedItem.Content], sources: [String: FeedItem.Source]) -> [FeedItem] {
        let lastVisitedDomains = (try? History.suffix(200)
            .lazy
            .compactMap(\.url)
            .compactMap { URL(string: $0)?.baseDomain }) ?? []
        return feeds.compactMap {
            let timeSincePublished = Double($0.publishTime.timeIntervalSinceNow)
            var score = timeSincePublished > 0 ? log(timeSincePublished) : 0
            if let feedBaseDomain = $0.url?.baseDomain, lastVisitedDomains.contains(feedBaseDomain) {
                score -= 5
            }
            guard let source = sources[$0.publisherID] else {
                return nil
            }
            return FeedItem(score: score, content: $0, source: source, isContentHidden: self.hiddenItems.contains($0.urlHash))
        }
    }
    
    private func generateCards(_ completion: @escaping () -> Void) {
        /**
         Beginning of new session:
            Sponsor card
            1st item, Large Card, Latest item from query
            Deals card
         */
        var sponsors = feeds.filter { $0.content.contentType == "brave_offers" }
        var deals = feeds.filter { $0.content.contentType == "product" }
        var articles = feeds.filter { $0.content.contentType == "article" && !$0.isContentHidden }
        
        let rules: [FeedSequenceElement] = [
            .sponsor,
            .headline(paired: false),
            .deals,
            .repeating([
                .repeating([.headline(paired: false)], times: 2),
                .repeating([.headline(paired: true)], times: 2),
                .categoryGroup,
                .headline(paired: false),
                .deals,
                .headline(paired: false),
                .headline(paired: true),
                .brandedGroup(numbered: true),
                .group,
                .headline(paired: false),
                .headline(paired: true),
            ])
        ]
        
        func _cards(for element: FeedSequenceElement) -> [FeedCard]? {
            switch element {
            case .sponsor:
                if sponsors.isEmpty { return nil }
                return [.sponsor(sponsors.removeFirst())]
            case .deals:
                if deals.isEmpty { return nil }
                let items = Array(deals.prefix(3))
                deals.removeFirst(min(3, items.count))
                return [.group(items, title: "Deals", direction: .horizontal, displayBrand: false)]
            case .headline(let paired):
                if articles.isEmpty { return nil }
                let imageExists = { (item: FeedItem) -> Bool in
                    item.content.imageURL != nil
                }
                if paired {
                    if articles.count < 2 {
                        return nil
                    }
                    guard
                        let firstIndex = articles.firstIndex(where: imageExists),
                        let secondIndex = articles[(firstIndex+1)...].firstIndex(where: imageExists) else {
                        return nil
                    }
                    let item1 = articles[firstIndex]
                    let item2 = articles[secondIndex]
                    articles.remove(at: firstIndex)
                    articles.remove(at: secondIndex - 1)
                    return [.headlinePair(.init(item1, item2))]
                } else {
                    guard let index = articles.firstIndex(where: imageExists) else {
                        return nil
                    }
                    let item = articles[index]
                    articles.remove(at: index)
                    return [.headline(item)]
                }
            case .categoryGroup:
                guard let category = articles.first?.content.category else { return nil }
                let items = Array(articles.lazy.filter({ $0.content.category == category }).prefix(3))
                articles.removeAll(where: { items.contains($0) })
                return [.group(items, title: category, direction: .vertical, displayBrand: false)]
            case .brandedGroup(let numbered):
                if articles.isEmpty { return nil }
                guard let source = articles.first?.source else { return nil }
                let items = Array(articles.lazy.filter({ $0.source == source }).prefix(3))
                articles.removeAll(where: { items.contains($0) })
                if numbered {
                    return [.numbered(items, title: items.first?.content.publisherName ?? "")]
                } else {
                    return [.group(items, title: "", direction: .vertical, displayBrand: true)]
                }
            case .group:
                if articles.isEmpty { return nil }
                let items = Array(articles.prefix(3))
                articles.removeFirst(min(3, items.count))
                return [.group(items, title: "", direction: .vertical, displayBrand: false)]
            case .repeating(let elements, let times):
                var index = 0
                var cards: [FeedCard] = []
                repeat {
                    var repeatedCards: [FeedCard] = []
                    for element in elements {
                        if let elementCards = _cards(for: element) {
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
        
        DispatchQueue.global(qos: .default).async {
            var generatedCards: [FeedCard] = []
            for rule in rules {
                if let elementCards = _cards(for: rule) {
                    generatedCards.append(contentsOf: elementCards)
                }
            }
            DispatchQueue.main.async {
                self.cards = generatedCards
                completion()
            }
        }
    }
}
