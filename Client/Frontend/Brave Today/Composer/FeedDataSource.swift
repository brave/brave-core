// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Data
import Shared
import BraveShared

// Named `logger` because we are using math function `log`
private let logger = Logger.browserLogger

/// Powers Brave Today's feed.
class FeedDataSource {
    /// The current view state of the data source
    enum State {
        /// Nothing has happened yet
        case initial
        /// Sources and or feed content items are currently downloading, read from cache, or cards are being
        /// generated from that data
        indirect case loading(_ previousState: State)
        /// Cards have been successfully generated from downloaded or cached content.
        case success([FeedCard])
        /// Some sort of error has occured when attempting to load feed content
        case failure(Error)
        
        /// The list of generated feed cards, if the state is `success`
        var cards: [FeedCard]? {
            switch self {
            case .success(let cards),
                 .loading(.success(let cards)):
                return cards
            default:
                return nil
            }
        }
        /// The error if the state is `failure`
        var error: Error? {
            switch self {
            case .failure(let error),
                 .loading(.failure(let error)):
                return error
            default:
                return nil
            }
        }
    }
    
    @Observable private(set) var state: State = .initial
    private(set) var sources: [FeedItem.Source] = []
    private var items: [FeedItem.Content] = []
    
    /// Add a closure that will execute when `state` is changed.
    ///
    /// Executes the closure on the main queue by default
    func observeState(
        from object: AnyObject,
        onQueue queue: DispatchQueue = .main,
        _ handler: @escaping Observable<State>.Handler
    ) {
        _state.observe(from: object, on: queue, handler)
    }
    
    private let todayQueue = DispatchQueue(label: "com.brave.today")
    private let reloadQueue = DispatchQueue(label: "com.brave.today.reload")
    
    // MARK: - Initialization
    
    init() {
        restoreCachedSources()
    }
    
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
    
    private struct TodayBucket {
        var name: String
        var path: String = ""
        
        var url: URL {
            var components = URLComponents()
            components.scheme = "https"
            // TODO: At the moment these files are only available on the dev servers, eventually we will
            // change `brave.software` to `bravesoftware.com` or `brave.com` based on staging/prod
            components.host = "\(name).brave.software"
            components.path = "/\(path)"
            return components.url!
        }
        
        static let `default` = TodayBucket(name: "brave-today-cdn")
        static let privateCDN = TodayBucket(name: "pcdn", path: "brave-today")
    }
    
    private struct TodayResource {
        var bucket: TodayBucket
        var filename: String
        var cacheLifetime: TimeInterval
        
        static let sources = TodayResource(
            bucket: .default,
            filename: "sources.json",
            cacheLifetime: 1.days
        )
        static let feed = TodayResource(
            bucket: .privateCDN,
            filename: "feed.json",
            cacheLifetime: 1.hours
        )
    }
    
    private static let cacheFolderName = "brave-today"
    
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
    
    /// Get a cached Brave Today resource file, optionally allowing expired data to be returned
    private func cachedResource(_ resource: TodayResource, loadExpiredData: Bool = false) -> Deferred<Data?> {
        let filename = resource.filename
        let fileManager = FileManager.default
        let deferred = Deferred<Data?>(value: nil, defaultQueue: .main)
        let cachedPath = fileManager.getOrCreateFolder(name: Self.cacheFolderName)?.appendingPathComponent(filename).path
        if (loadExpiredData || !isResourceExpired(resource)),
            let cachedPath = cachedPath,
            fileManager.fileExists(atPath: cachedPath) {
            todayQueue.async {
                if let cachedContents = fileManager.contents(atPath: cachedPath) {
                    deferred.fill(cachedContents)
                } else {
                    deferred.fill(nil)
                }
            }
        } else {
            deferred.fill(nil)
        }
        return deferred
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
        return cachedResource(resource)
            .bind({ [weak self] data -> Deferred<Result<Data, Error>> in
                guard let self = self else { return .init() }
                if let data = data {
                    return .init(value: .success(data), defaultQueue: .main)
                }
                let deferred = Deferred<Result<Data, Error>>(value: nil, defaultQueue: .main)
                self.session.dataRequest(with: resource.bucket.url.appendingPathComponent(filename)) { data, response, error in
                    if let error = error {
                        deferred.fill(.failure(error))
                        return
                    }
                    guard let data = data else { return }
                    deferred.fill(.success(data))
                }
                return deferred
            })
            .mapQueue(todayQueue) { result in
                switch result {
                case .success(let data):
                    do {
                        let decodedResource = try self.decoder.decode(DataType.self, from: data)
                        if !data.isEmpty {
                            if !FileManager.default.writeToDiskInFolder(data, fileName: filename, folderName: Self.cacheFolderName) {
                                logger.error("Failed to write sources to disk")
                            }
                        }
                        return .success(decodedResource)
                    } catch {
                        return .failure(error)
                    }
                case .failure(let error):
                    return .failure(error)
                }
            }
    }
    
    private func restoreCachedSources() {
        cachedResource(.sources, loadExpiredData: true).uponQueue(todayQueue) { [weak self] data in
            guard let self = self, let data = data else { return }
            do {
                let decodedResource = try self.decoder.decode([FailableDecodable<FeedItem.Source>].self, from: data)
                DispatchQueue.main.async {
                    self.sources = decodedResource.compactMap(\.wrappedValue)
                }
            } catch {
                // Could be a source type change, so may not be a big issue. If the user goes to download
                // updated lists and it still fails it will show an error on the feed
                logger.debug("Failed to decode previously cached sources: \(error)")
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
        case .initial, .failure:
            return true
        case .loading:
            return false
        case .success:
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
    func load(_ completion: (() -> Void)? = nil) {
        state = .loading(state)
        loadSources().both(loadFeed()).uponQueue(.main) { [weak self] results in
            guard let self = self else { return }
            switch results {
            case (.failure(let error), _),
                 (_, .failure(let error)):
                self.state = .failure(error)
                completion?()
            case (.success(let sources), .success(let items)):
                self.sources = sources
                self.items = items
                self.reloadCards(from: items, sources: sources, completion: completion)
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
        } catch {
            logger.error("Could not remove cached files")
            return false
        }
        return true
    }
    
    // MARK: - Sources
    
    static let topNewsCategory = "Top News"
    
    /// Get a map of customized sources IDs and their overridden enabled states
    var customizedSources: [String: Bool] {
        let all = FeedSourceOverride.all()
        return all.reduce(into: [:]) { (result, source) in
            result[source.publisherID] = source.enabled
        }
    }
    
    /// Whether or not a source is currently enabled (whether or not by default or by a user changing
    /// said default)
    func isSourceEnabled(_ source: FeedItem.Source) -> Bool {
        FeedSourceOverride.get(fromId: source.id)?.enabled ?? source.isDefault
    }
    
    /// Toggle a source's enabled status
    func toggleSource(_ source: FeedItem.Source, enabled: Bool) {
        FeedSourceOverride.setEnabled(forId: source.id, enabled: enabled)
        
        if let cards = state.cards, cards.isEmpty && enabled {
            // If we're enabling a source and we don't have any items because their source selection was
            // causing an empty generation, regenerate the cards
            self.reloadCards(from: self.items, sources: self.sources)
        }
    }
    
    /// Toggle an entire category on or off
    func toggleCategory(_ category: String, enabled: Bool) {
        let sourcesInCategory = sources.filter { $0.category == category }
        if sourcesInCategory.isEmpty {
            return
        }
        FeedSourceOverride.setEnabled(forIds: sourcesInCategory.map(\.id), enabled: enabled)
        
        if let cards = state.cards, cards.isEmpty && enabled {
            // If we're enabling a category and we don't have any items because their source selection was
            // causing an empty generation, regenerate the cards
            self.reloadCards(from: self.items, sources: self.sources)
        }
    }
    
    /// Reset all source settings back to default
    func resetSourcesToDefault() {
        FeedSourceOverride.resetSourceSelection()
    }
    
    // MARK: - Card Generation
    
    /// Scores and generates cards from a set of items and sources
    private func reloadCards(
        from items: [FeedItem.Content],
        sources: [FeedItem.Source],
        completion: (() -> Void)? = nil
    ) {
        // Only allow 1 reload at a time
        // Since the scoring/generation work hops between threads a lot it would be possible for reloads to
        // mess up the ending state or update the cards
        reloadQueue.async { [weak self] in
            let group = DispatchGroup()
            group.enter()
            DispatchQueue.main.async {
                // Score must be called from main queue
                self?.score(feeds: items, sources: sources) { [weak self] feedItems in
                    self?.generateCards(from: feedItems) { [weak self] cards in
                        defer { group.leave() }
                        self?.state = .success(cards)
                        completion?()
                    }
                }
            }
            _ = group.wait(timeout: .now() + 30)
        }
    }
    
    /// Scores a set of items in the feed based on recency, personalization and variety.
    ///
    /// The items returned in `completion` will be sorted based on score
    ///
    /// Must be called on the main thread, `completion` is called on main thread
    private func score(
        feeds: [FeedItem.Content],
        sources: [FeedItem.Source],
        completion: @escaping ([FeedItem]) -> Void
    ) {
        // Ensure main thread since we're querying from CoreData
        dispatchPrecondition(condition: .onQueue(.main))
        let lastVisitedDomains = (try? History.suffix(200)
            .lazy
            .compactMap(\.url)
            .compactMap { URL(string: $0)?.baseDomain }) ?? []
        todayQueue.async {
            let items: [FeedItem] = feeds.compactMap { content in
                var score = content.baseScore
                if let feedBaseDomain = content.url?.baseDomain,
                    lastVisitedDomains.contains(feedBaseDomain) {
                    score -= 5
                }
                guard let source = sources.first(where: { $0.id == content.publisherID }) else {
                    return nil
                }
                return FeedItem(score: score, content: content, source: source)
            }
            .sorted(by: <)
            DispatchQueue.main.async {
                completion(items)
            }
        }
    }
    
    private func generateCards(from items: [FeedItem], completion: @escaping ([FeedCard]) -> Void) {
        // Ensure main thread since we're querying from CoreData
        dispatchPrecondition(condition: .onQueue(.main))
        
        let overridenSources = FeedSourceOverride.all()
        let feedsFromEnabledSources = items.filter { item in
            overridenSources.first(where: {
                $0.publisherID == item.source.id
            })?.enabled ?? item.source.isDefault
        }
        var sponsors = feedsFromEnabledSources.filter { $0.content.contentType == .sponsor }
        var deals = feedsFromEnabledSources.filter { $0.content.contentType == .deals }
        var articles = feedsFromEnabledSources.filter { $0.content.contentType == .article }
        
        let dealsCategoryFillStrategy = CategoryFillStrategy(
            categories: Set(deals.compactMap(\.content.offersCategory)),
            category: \.content.offersCategory
        )
        
        let rules: [FeedSequenceElement] = [
            .sponsor,
            .fillUsing(FilteredFillStrategy(isIncluded: { $0.source.category == Self.topNewsCategory }), [
                .headline(paired: false)
            ]),
            .fillUsing(dealsCategoryFillStrategy, [
                .deals
            ]),
            .repeating([
                .repeating([.headline(paired: false)], times: 2),
                .repeating([.headline(paired: true)], times: 2),
                .fillUsing(
                    CategoryFillStrategy(
                        categories: Set(articles.map(\.source.category)),
                        category: \.source.category,
                        initialCategory: Self.topNewsCategory
                    ), [
                        .categoryGroup,
                    ]
                ),
                .headline(paired: false),
                .fillUsing(dealsCategoryFillStrategy, [
                    .deals
                ]),
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
        
        func _cards(for element: FeedSequenceElement, fillStrategy: FillStrategy) -> [FeedCard]? {
            switch element {
            case .sponsor:
                return fillStrategy.next(from: &sponsors).map {
                    [.sponsor($0)]
                }
            case .deals:
                return fillStrategy.next(3, from: &deals).map {
                    let title = $0.first?.content.offersCategory
                    return [.deals($0, title: title ?? Strings.BraveToday.deals)]
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
                return fillStrategy.next(3, from: &articles).map {
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
        
        todayQueue.async {
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

extension FeedDataSource {
    private enum FeedSequenceElement {
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

}
