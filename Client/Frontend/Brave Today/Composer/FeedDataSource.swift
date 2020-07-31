// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Data
import Shared
import Deferred
import BraveShared

// Named `logger` because we are using math function `log`
private let logger = Logger.browserLogger

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
    /// The current view state of the data source
    enum State {
        /// Nothing has happened yet
        case initial
        /// Sources and or feed content items are currently downloading, read from cache, or cards are being
        /// generated from that data
        case loading
        /// Cards have been successfully generated from downloaded or cached content.
        case success([FeedCard])
        /// Some sort of error has occured when attempting to load feed content
        case failure(Error)
        
        /// The list of generated feed cards, if the state is `success`
        var cards: [FeedCard]? {
            get {
                if case .success(let cards) = self {
                    return cards
                }
                return nil
            }
            set {
                if let cards = newValue {
                    self = .success(cards)
                }
            }
        }
        /// The error if the state is `failure`
        var error: Error? {
            if case .failure(let error) = self {
                return error
            }
            return nil
        }
    }
    
    private(set) var state: State = .initial
    private(set) var sources: [FeedItem.Source] = []
    
    private let session = NetworkManager(session: URLSession(configuration: .ephemeral))
    
    private let decoder: JSONDecoder = {
        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .formatted(DateFormatter().then {
            $0.dateFormat = "yyyy-MM-dd HH:mm:ss"
            $0.timeZone = TimeZone(secondsFromGMT: 0)
        })
        return decoder
    }()
    
    private struct TodayResource {
        var filename: String
        var cacheLifetime: TimeInterval
        
        static let sources = TodayResource(filename: "sources.json", cacheLifetime: 1.days)
        static let feed = TodayResource(filename: "feed.json", cacheLifetime: 1.hours)
    }
    
    private static let cacheFolderName = "brave-today"
    private static let baseURL = URL(string: "https://pcdn.brave.software/brave-today")!
    
    /// Determine whether or not some cached resource is expired
    ///
    /// - Note: If no file can be found, this returns `true`
    private func isResourceExpired(_ resource: TodayResource) -> Bool {
        let fileManager = FileManager.default
        let cachedPath = fileManager.getOrCreateFolder(name: Self.cacheFolderName)?.appendingPathComponent(resource.filename).path
        if let cachedPath = cachedPath,
            let attributes = try? fileManager.attributesOfItem(atPath: cachedPath),
            let date = attributes[.modificationDate] as? Date {
            return Date().timeIntervalSince(date) > resource.cacheLifetime
        }
        return true
    }
    
    /// Load a Brave Today resource either from a file cache or the web
    ///
    /// The `filename` provided will be appended as a path component to the request URL, and be used to
    /// fetch the cache and save the response so it should include the full path for the endpoint (For
    /// example: `sources.json`)
    ///
    /// Cache lifetime will be based on the modification date of the cached file. Data downloaded from the web
    /// will only be cached if it is successfully decoded into the given `DataType`.
    private func loadResource<DataType>(
        _ resource: TodayResource,
        decodedTo: DataType.Type
    ) -> Deferred<Result<DataType, Error>> where DataType: Decodable {
        let filename = resource.filename
        let fileManager = FileManager.default
        let deferred = Deferred<Result<Data, Error>>(value: nil, defaultQueue: .main)
        let cachedPath = fileManager.getOrCreateFolder(name: Self.cacheFolderName)?.appendingPathComponent(filename).path
        if let cachedPath = cachedPath,
            !isResourceExpired(resource),
            let cachedContents = fileManager.contents(atPath: cachedPath) {
            deferred.fill(.success(cachedContents))
        } else {
            session.dataRequest(with: Self.baseURL.appendingPathComponent(filename)) { data, response, error in
                if let error = error {
                    deferred.fill(.failure(error))
                    return
                }
                guard let data = data else { return }
                deferred.fill(.success(data))
            }
        }
        return deferred.map { result in
            switch result {
            case .success(let data):
                do {
                    let decodedSources = try self.decoder.decode(DataType.self, from: data)
                    if !FileManager.default.writeToDiskInFolder(data, fileName: filename, folderName: Self.cacheFolderName) {
                        logger.error("Failed to write sources to disk")
                    }
                    return .success(decodedSources)
                } catch {
                    return .failure(error)
                }
            case .failure(let error):
                return .failure(error)
            }
        }
    }
    
    private func loadSources() -> Deferred<Result<[FeedItem.Source], Error>> {
        loadResource(.sources, decodedTo: [FailableDecodable<FeedItem.Source>].self).map { result in
            result.map {
                $0.compactMap(\.wrappedValue)
            }
        }
    }
    
    private func loadFeed() -> Deferred<Result<[FeedItem.Content], Error>> {
        loadResource(.feed, decodedTo: [FailableDecodable<FeedItem.Content>].self).map { result in
            result.map {
                $0.compactMap(\.wrappedValue)
            }
        }
    }
    
    /// Whether or not we should load content or just use what's in `state`.
    ///
    /// If the data source is already loading, returns `false`
    var shouldLoadContent: Bool {
        switch state {
        case .initial:
            return true
        case .loading:
            return false
        default:
            return isFeedContentExpired || isSourcesExpired
        }
    }
    
    /// Whether or not the feed content is currently expired and needs to be reloaded
    var isFeedContentExpired: Bool {
        isResourceExpired(.feed)
    }
    
    /// Whether or not the sources are currently expired and needs to be reloaded
    var isSourcesExpired: Bool {
        isResourceExpired(.sources)
    }
    
    /// Loads Brave Today resources and generates cards for the loaded data. The result will be placed in
    /// the `state` property.
    ///
    /// Resources are loaded either from cache (if the cache is valid for said resource) or from the web,
    /// scored, and then used to generate a list of `FeedCard` objects.
    ///
    /// Given the nature of async card regeneration, calling this method will always set the state to
    /// `loading` initially.
    func load(_ completion: @escaping () -> Void) {
        state = .loading
        loadSources().both(loadFeed()).uponQueue(.main) { [weak self] results in
            guard let self = self else { return }
            switch results {
            case (.failure(let error), _),
                 (_, .failure(let error)):
                self.state = .failure(error)
                completion()
            case (.success(let sources), .success(let items)):
                let overridenSources = BraveTodaySourceMO.all()
                self.sources = sources.map { source in
                    if let overridenSource = overridenSources.first(where: { $0.publisherID == source.id }) {
                        var copy = source
                        copy.enabled = overridenSource.enabled
                        return copy
                    }
                    return source
                }
                let feedItems = self.scored(feeds: items, sources: self.sources).sorted(by: <)
                self.generateCards(from: feedItems) { [weak self] cards in
                    self?.state = .success(cards)
                    completion()
                }
            }
        }
    }
    
    /// Clears any cached files from the users device
    @discardableResult
    func clearCachedFiles() -> Bool {
        do {
            let fileManager = FileManager.default
            if let braveTodayPath = fileManager.getOrCreateFolder(name: Self.cacheFolderName) {
                let filePaths = try fileManager.contentsOfDirectory(atPath: braveTodayPath.path)
                try filePaths.forEach {
                    var fileUrl = braveTodayPath
                    fileUrl.appendPathComponent($0)
                    try fileManager.removeItem(atPath: fileUrl.path)
                }
            }
            // state = .initial
        } catch {
            logger.error("Could not remove cached files")
            return false
        }
        return true
    }
    
    /// Toggle a source's enabled status
    func toggleSource(_ source: FeedItem.Source, enabled: Bool) {
        guard let cards = state.cards, let sourceIndex = sources.firstIndex(where: { $0.id == source.id }) else { return }
        self.sources[sourceIndex].enabled = enabled
        
        BraveTodaySourceMO.setEnabled(forId: source.id, enabled: enabled)
        
        // Propigate source toggle to cards list
        let cardsWithItemsFromSources = cards.enumerated().filter {
            $0.element.items.contains(where: {
                $0.source == source
            })
        }
        for (index, element) in cardsWithItemsFromSources {
            for item in element.items where item.source == source {
                var alteredItem = item
                alteredItem.source = self.sources[sourceIndex]
                state.cards?[index] = element.replacing(item: item, with: alteredItem)
            }
        }
    }
    
    private func scored(feeds: [FeedItem.Content], sources: [FeedItem.Source]) -> [FeedItem] {
        let lastVisitedDomains = (try? History.suffix(200)
            .lazy
            .compactMap(\.url)
            .compactMap { URL(string: $0)?.baseDomain }) ?? []
        return feeds.compactMap { content in
            let timeSincePublished = Double(content.publishTime.timeIntervalSinceNow)
            var score = timeSincePublished > 0 ? log(timeSincePublished) : 0
            if let feedBaseDomain = content.url?.baseDomain,
                lastVisitedDomains.contains(feedBaseDomain) {
                score -= 5
            }
            guard let source = sources.first(where: { $0.id == content.publisherID }) else {
                return nil
            }
            return FeedItem(score: score, content: content, source: source)
        }
    }
    
    private func generateCards(from items: [FeedItem], completion: @escaping ([FeedCard]) -> Void) {
        /**
         Beginning of new session:
            Sponsor card
            1st item, Large Card, Latest item from query
            Deals card
         */
        let feedsFromEnabledSources = items.filter(\.source.enabled)
        var sponsors = feedsFromEnabledSources.filter { $0.content.contentType == .offer }
        var deals = feedsFromEnabledSources.filter { $0.content.contentType == .product }
        var articles = feedsFromEnabledSources.filter { $0.content.contentType == .article }
        
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
                // FIXME: Localize
                return [.deals(items, title: "Deals")]
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
                guard let category = articles.first?.source.category else { return nil }
                let items = Array(articles.lazy.filter({ $0.source.category == category }).prefix(3))
                articles.removeAll(where: { items.contains($0) })
                return [.group(items, title: category, direction: .vertical, displayBrand: false)]
            case .brandedGroup(let numbered):
                if articles.isEmpty { return nil }
                guard let source = articles.first?.source else { return nil }
                let items = Array(articles.lazy.filter({ $0.source == source }).prefix(3))
                articles.removeAll(where: { items.contains($0) })
                if numbered {
                    return [.numbered(items, title: items.first?.source.name ?? "")]
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
                completion(generatedCards)
            }
        }
    }
}
