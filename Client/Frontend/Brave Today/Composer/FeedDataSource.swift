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
    /// Displays the sequence element provided using a specific fill strategy to obtain feed items from the
    /// feed list
    indirect case fillUsing(_ strategy: FillStrategy, _ elements: [FeedSequenceElement])
    /// Displays the provided elements a number of times. Passing in `.max` for `times` means it will repeat
    /// until there is no more content available
    indirect case repeating([FeedSequenceElement], times: Int = .max)
}

/// Defines a ruleset for getting the next set of items from a list of `FeedItem`'s
protocol FillStrategy {
    /// Obtain the next `length` number of feed items from a list. If exactly `length` items can be queried,
    /// then those items are removed from `list` and returned.
    ///
    /// You can optionally provide some `predicate` to determine what items are valid in `list`
    ///
    /// - Returns: A set of feed items if `list` (or the filtered variant given some `predicate`) contains at
    ///            least `length` items.
    func next(
        _ length: Int,
        from list: inout [FeedItem],
        where predicate: ((FeedItem) -> Bool)?
    ) -> [FeedItem]?
}

extension FillStrategy {
    func next(
        _ length: Int,
        from list: inout [FeedItem],
        where predicate: ((FeedItem) -> Bool)? = nil
    ) -> [FeedItem]? {
        next(length, from: &list, where: predicate)
    }
    /// Obtain the next feed item from `list`. If that item can be queried successfully, then that item is
    /// removed from `list` and returned.
    func next(
        from list: inout [FeedItem],
        where predicate: ((FeedItem) -> Bool)? = nil
    ) -> FeedItem? {
        next(1, from: &list, where: predicate)?.first
    }
}

/// A fill strategy that always pulls from the beginning of the list
struct DefaultFillStrategy: FillStrategy {
    func next(
        _ length: Int,
        from list: inout [FeedItem],
        where predicate: ((FeedItem) -> Bool)? = nil
    ) -> [FeedItem]? {
        if let predicate = predicate {
            let filteredItems = list.filter(predicate)
            if filteredItems.count < length { return nil }
            let items = Array(filteredItems.prefix(upTo: length))
            items.forEach { item in
                if let index = list.firstIndex(of: item) {
                    list.remove(at: index)
                }
            }
            return items
        } else {
            if list.count < length { return nil }
            let items = Array(list.prefix(upTo: length))
            list.removeFirst(items.count)
            return items
        }
    }
}

/// A fill strategy that always pulls from the beginning of the list after said list has been filtered
/// by some given predicate
struct FilteredFillStrategy: FillStrategy {
    /// A global predicate to determine what items are valid to pull from. For example, only pulling items
    /// that are in a given category
    var isIncluded: ((FeedItem) -> Bool)
    
    func next(
        _ length: Int,
        from list: inout [FeedItem],
        where predicate: ((FeedItem) -> Bool)? = nil
    ) -> [FeedItem]? {
        let workingList = list.filter {
            (predicate?($0) ?? true) && isIncluded($0)
        }
        if workingList.count < length { return nil }
        let items = Array(workingList.prefix(upTo: length))
        items.forEach { item in
            if let index = list.firstIndex(of: item) {
                list.remove(at: index)
            }
        }
        return items
    }
}

/// A fill strategy that pulls random items from the list
struct RandomizedFillStrategy: FillStrategy {
    /// A global predicate to determine what random items are valid to pull from. For example, only pulling
    /// random items that are less than 48 hours old
    var isIncluded: ((FeedItem) -> Bool)?
    
    func next(
        _ length: Int,
        from list: inout [FeedItem],
        where predicate: ((FeedItem) -> Bool)? = nil
    ) -> [FeedItem]? {
        var workingList = list
        if predicate != nil || isIncluded != nil {
            workingList = workingList.filter {
                (predicate?($0) ?? true) && (isIncluded?($0) ?? true)
            }
        }
        if workingList.count < length { return nil }
        return (0..<length).compactMap { _ in
            if let index = workingList.indices.randomElement() {
                let item = workingList.remove(at: index)
                if let index = list.firstIndex(of: item) {
                    list.remove(at: index)
                }
                return item
            }
            return nil
        }
    }
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
    
    // MARK: - Resource Managment
    
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
    
    /// A set of Brave Today specific errors that could occur outside of JSON decoding or network errors
    enum BraveTodayError: Error {
        /// The resource data that was loaded was empty after parsing
        case resourceEmpty
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
                    if !data.isEmpty {
                        if !FileManager.default.writeToDiskInFolder(data, fileName: filename, folderName: Self.cacheFolderName) {
                            logger.error("Failed to write sources to disk")
                        }
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
            if case .success(let sources) = result, sources.isEmpty {
                return .failure(BraveTodayError.resourceEmpty)
            }
            return result.map {
                $0.compactMap(\.wrappedValue)
            }
        }
    }
    
    private func loadFeed() -> Deferred<Result<[FeedItem.Content], Error>> {
        loadResource(.feed, decodedTo: [FailableDecodable<FeedItem.Content>].self).map { result in
            if case .success(let sources) = result, sources.isEmpty {
                return .failure(BraveTodayError.resourceEmpty)
            }
            return result.map {
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
                self.sources = sources
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
    
    // MARK: - Sources
    
    /// Get a map of customized sources IDs and their overridden enabled states
    var customizedSources: [String: Bool] {
        let all = BraveTodaySourceMO.all()
        return all.reduce(into: [:]) { (result, source) in
            result[source.publisherID] = source.enabled
        }
    }
    
    /// Whether or not a source is currently enabled (whether or not by default or by a user changing
    /// said default)
    func isSourceEnabled(_ source: FeedItem.Source) -> Bool {
        BraveTodaySourceMO.get(fromId: source.id)?.enabled ?? source.isDefault
    }
    
    /// Toggle a source's enabled status
    func toggleSource(_ source: FeedItem.Source, enabled: Bool) {
        BraveTodaySourceMO.setEnabled(forId: source.id, enabled: enabled)
    }
    
    /// Toggle an entire category on or off
    func toggleCategory(_ category: String, enabled: Bool) {
        let sourcesInCategory = sources.filter { $0.category == category }
        if sourcesInCategory.isEmpty {
            return
        }
        BraveTodaySourceMO.setEnabled(forIds: sourcesInCategory.map(\.id), enabled: enabled)
    }
    
    /// Reset all source settings back to default
    func resetSourcesToDefault() {
        BraveTodaySourceMO.resetSourceSelection()
    }
    
    // MARK: - Card Generation
    
    private func scored(feeds: [FeedItem.Content], sources: [FeedItem.Source]) -> [FeedItem] {
        let lastVisitedDomains = (try? History.suffix(200)
            .lazy
            .compactMap(\.url)
            .compactMap { URL(string: $0)?.baseDomain }) ?? []
        var domainCount: [String: Int] = [:]
        return feeds.compactMap { content in
            let timeSincePublished = Date().timeIntervalSince(content.publishTime)
            var score = timeSincePublished > 0 ? log(timeSincePublished) : 0
            if let feedBaseDomain = content.url?.baseDomain,
                lastVisitedDomains.contains(feedBaseDomain) {
                score -= 5
            }
            let variety = 1.0 / Double(1 << domainCount[content.publisherID, default: 0])
            score *= variety
            domainCount[content.publisherID, default: 0] += 1
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
        let overridenSources = BraveTodaySourceMO.all()
        let feedsFromEnabledSources = items.filter { item in
            overridenSources.first(where: {
                $0.publisherID == item.source.id
            })?.enabled ?? item.source.isDefault
        }
        var sponsors = feedsFromEnabledSources.filter { $0.content.contentType == .sponsor }
        var deals = feedsFromEnabledSources.filter { $0.content.contentType == .deals }
        var articles = feedsFromEnabledSources.filter { $0.content.contentType == .article }
        
        let rules: [FeedSequenceElement] = [
            .sponsor,
            .fillUsing(FilteredFillStrategy(isIncluded: { $0.source.category == "Top News" }), [
                .headline(paired: false)
            ]),
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
                .fillUsing(RandomizedFillStrategy(isIncluded: { Date().timeIntervalSince($0.content.publishTime) < 48.hours }), [
                    .headline(paired: false),
                    .headline(paired: true),
                    .headline(paired: false),
                ])
            ])
        ]
        
        var nextCategory = "Top News" // FIXME: Magic string should be defined somewhere
        let allCategories = Set(articles.map(\.source.category))
        var categories: Set<String> = allCategories
        
        func _cards(for element: FeedSequenceElement, fillStrategy: FillStrategy) -> [FeedCard]? {
            switch element {
            case .sponsor:
                return fillStrategy.next(from: &sponsors).map {
                    [.sponsor($0)]
                }
            case .deals:
                return fillStrategy.next(3, from: &deals).map {
                    // FIXME: Localize
                    [.deals($0, title: "Deals")]
                }
            case .headline(let paired):
                if articles.isEmpty { return nil }
                let imageExists = { (item: FeedItem) -> Bool in
                    item.content.imageURL != nil
                }
                if paired {
                    if articles.count < 2 {
                        return nil
                    }
                    return fillStrategy.next(2, from: &articles, where: imageExists).map {
                        [.headlinePair(.init($0[0], $0[1]))]
                    }
                } else {
                    return fillStrategy.next(from: &articles, where: imageExists).map {
                        [.headline($0)]
                    }
                }
            case .categoryGroup:
                let items: [FeedCard]? = fillStrategy.next(3, from: &articles, where: { $0.source.category == nextCategory }).map {
                    [.group($0, title: nextCategory, direction: .vertical, displayBrand: false)]
                }
                // Ensure the next category card does not use the same categories until all categories have
                // been shown
                categories.remove(nextCategory)
                if categories.isEmpty {
                    // All categories have now been shown, reset back to the start
                    categories = allCategories
                    // FIXME: Magic string should be defined somewhere
                    nextCategory = "Top News"
                } else {
                    nextCategory = categories.randomElement()!
                }
                return items
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
                return nil
            case .group:
                return fillStrategy.next(3, from: &articles).map {
                    [.group($0, title: "", direction: .vertical, displayBrand: false)]
                }
            case .fillUsing(let strategy, let elements):
                var cards: [FeedCard] = []
                for element in elements {
                    if let elementCards = _cards(for: element, fillStrategy: strategy) {
                        cards.append(contentsOf: elementCards)
                    }
                }
                return cards
            case .repeating(let elements, let times):
                var index = 0
                var cards: [FeedCard] = []
                repeat {
                    var repeatedCards: [FeedCard] = []
                    for element in elements {
                        if let elementCards = _cards(for: element, fillStrategy: fillStrategy) {
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
                if let elementCards = _cards(for: rule, fillStrategy: DefaultFillStrategy()) {
                    generatedCards.append(contentsOf: elementCards)
                }
            }
            DispatchQueue.main.async {
                completion(generatedCards)
            }
        }
    }
}
