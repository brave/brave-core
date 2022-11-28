// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveUI
import Data
import Shared
import BraveShared
import FeedKit
import BraveCore
import CodableHelpers
import os.log
import SwiftUI

/// Powers the Brave News feed.
public class FeedDataSource: ObservableObject {
  /// The current view state of the data source
  public enum State {
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
    public var cards: [FeedCard]? {
      switch self {
      case .success(let cards),
        .loading(.success(let cards)):
        return cards
      default:
        return nil
      }
    }
    /// The error if the state is `failure`
    public var error: Error? {
      switch self {
      case .failure(let error),
        .loading(.failure(let error)):
        return error
      default:
        return nil
      }
    }
  }

  @Published public private(set) var state: State = .initial
  @Published private(set) var sources: [FeedItem.Source] = [] {
    didSet {
      reloadChannels()
      reloadAvailableLocales()
    }
  }
  @Published private(set) var sourceSuggestions: [String: [FeedItem.SourceSimilarity]] = [:]
  @Published public var selectedLocale: String {
    didSet {
      reloadChannels()
    }
  }
  private var availableLocales: Set<String> = []
  private var localesWithFollowing: Set<String> = []
  @Published private var allChannels: [String: Set<String>] = [:]
  var channels: Set<String> {
    allChannels[selectedLocale] ?? []
  }
  private var items: [FeedItem.Content] = []
  
  private func reloadChannels() {
    let channelLocaleMap = Dictionary(
      grouping: sources.compactMap(\.localeDetails).flatMap({ $0 }),
      by: { $0.locale }
    )
    allChannels = channelLocaleMap.mapValues({ localeDetails in
      localeDetails.compactMap({ $0 }).reduce(into: Set<String>(), { $0.formUnion($1.channels) })
    })
  }
  
  private func reloadAvailableLocales() {
    let localeMap = Dictionary(
      grouping: sources.compactMap(\.localeDetails).flatMap({ $0 }),
      by: { $0.locale }
    )
    availableLocales = Set(localeMap.keys)
  }
  
  var followedSources: Set<FeedItem.Source> {
    let allOverrides = Set(FeedSourceOverride.all().filter(\.enabled).map(\.publisherID))
    return Set(sources.filter({ allOverrides.contains($0.id) }))
  }
  
  var followedChannels: Set<FeedChannel> {
    let channels = Preferences.BraveNews.followedChannels.value
    return channels.reduce(into: Set<FeedChannel>()) { result, element in
      result.formUnion(element.value.map({
        .init(localeIdentifier: element.key, name: $0)
      }))
    }
  }

  /// An ads object to handle inserting Inline Content Ads within the Brave News sequence
  public var ads: BraveAds?

  private let todayQueue = DispatchQueue(label: "com.brave.today")
  private let reloadQueue = DispatchQueue(label: "com.brave.today.reload")

  // MARK: - Initialization

  public init() {
    selectedLocale = Preferences.BraveNews.selectedLocale.value ?? "en_US"
    restoreCachedSources()
    
    if !AppConstants.buildChannel.isPublic,
      let savedEnvironment = Preferences.BraveNews.debugEnvironment.value,
      let environment = Environment(rawValue: savedEnvironment) {
      self.environment = environment
    }
    
    recordTotalExternalFeedsP3A()
  }

  // MARK: - Resource Managment

  private let session = URLSession(configuration: .ephemeral)

  private let decoder: JSONDecoder = {
    let decoder = JSONDecoder()
    decoder.dateDecodingStrategy = .formatted(
      DateFormatter().then {
        $0.dateFormat = "yyyy-MM-dd HH:mm:ss"
        $0.timeZone = TimeZone(secondsFromGMT: 0)
      })
    return decoder
  }()

  /// A Brave News environment
  enum Environment: String, CaseIterable {
    case dev = "brave.software"
    case staging = "bravesoftware.com"
    case production = "brave.com"
  }

  /// The current Brave News environment.
  ///
  /// Updating the environment automatically clears the current cached items if any exist.
  ///
  /// - warning: Should only be changed in non-public releases
  var environment: Environment = .production {
    didSet {
      if oldValue == environment { return }
      assert(
        !AppConstants.buildChannel.isPublic,
        "Environment cannot be changed on non-public build channels")
      Preferences.BraveNews.debugEnvironment.value = environment.rawValue
      clearCachedFiles()
    }
  }

  /// A list of the supported languages
  public static let supportedLanguages = [
    "en",
    "ja",
  ]
  
  /// A list of known supported locales
  public static let knownSupportedLocales = [
    "en_US",
    "en_CA",
    "en_UK",
    "ja_JP",
    "es_ES",
    "es_MX",
  ]

  private struct NewsBucket {
    var name: String
    var path: String = ""
  }

  private func resourceUrl(for bucket: NewsBucket) -> URL? {
    var components = URLComponents()
    components.scheme = "https"
    components.host = "\(bucket.name).\(environment.rawValue)"
    components.path = "/\(bucket.path)"
    return components.url
  }

  private struct NewsResource {
    var bucket: NewsBucket
    var name: String
    var type: String
    var isLocalized: Bool
    var cacheLifetime: TimeInterval

    static let globalSources = NewsResource(
      bucket: NewsBucket(name: "brave-today-cdn"),
      name: "sources.global",
      type: "json",
      isLocalized: false,
      cacheLifetime: 1.days
    )
    static let sources = NewsResource(
      bucket: NewsBucket(name: "brave-today-cdn"),
      name: "sources",
      type: "json",
      isLocalized: true,
      cacheLifetime: 1.days
    )
    static let feed = NewsResource(
      bucket: NewsBucket(name: "brave-today-cdn", path: "brave-today"),
      name: "feed",
      type: "json",
      isLocalized: true,
      cacheLifetime: 1.hours
    )
    static let sourceSuggestions = NewsResource(
      bucket: NewsBucket(name: "brave-today-cdn", path: "source-suggestions"),
      name: "source_similarity_t10",
      type: "json",
      isLocalized: true,
      cacheLifetime: 1.days
    )
  }

  /// Get the full name of a file for a giv en Brave News resource, taking into account whether
  /// or not the resource can be localized for supported languages
  private func resourceFilename(for resource: NewsResource, localeIdentifier: String? = nil) -> String {
    // "en" is the default language and thus does not get the language code inserted into the
    // file name.
    if resource.isLocalized, let localeIdentifier {
      return "\(resource.name).\(localeIdentifier).\(resource.type)"
    }
    return "\(resource.name).\(resource.type)"
  }

  private static let cacheFolderName = "brave-today"

  /// Determine whether or not some cached resource is expired
  ///
  /// - Note: If no file can be found, this returns `true`
  private func isResourceExpired(_ resource: NewsResource, localeIdentifier: String? = nil) -> Bool {
    assert(!resource.isLocalized || localeIdentifier != nil)
    let fileManager = FileManager.default
    let filename = resourceFilename(for: resource, localeIdentifier: localeIdentifier)
    let cachedPath = fileManager.getOrCreateFolder(name: Self.cacheFolderName)?.appendingPathComponent(filename).path
    if let cachedPath = cachedPath,
      let attributes = try? fileManager.attributesOfItem(atPath: cachedPath),
      let date = attributes[.modificationDate] as? Date {
      return Date().timeIntervalSince(date) > resource.cacheLifetime
    }
    return true
  }

  /// A set of Brave News specific errors that could occur outside of JSON decoding or network errors
  enum BraveNewsError: Error {
    /// The resource data that was loaded was empty after parsing
    case resourceEmpty
    /// Something went wrong
    case unknownError
  }
  
  private func on<T>(queue: DispatchQueue, work: @escaping () -> T) async -> T {
    return await withCheckedContinuation { c in
      queue.async {
        c.resume(returning: work())
      }
    }
  }
  
  private func onThrowing<T>(queue: DispatchQueue, work: @escaping () throws -> T) async throws -> T {
    return try await withCheckedThrowingContinuation { c in
      queue.async {
        do {
          c.resume(returning: try work())
        } catch {
          c.resume(throwing: error)
        }
      }
    }
  }

  /// Get a cached Brave News resource file, optionally allowing expired data to be returned
  @MainActor private func cachedResource(
    _ resource: NewsResource,
    localeIdentifier: String? = nil,
    loadExpiredData: Bool = false
  ) async -> Data? {
    let name = resourceFilename(for: resource, localeIdentifier: localeIdentifier)
    let fileManager = FileManager.default
    let cachedPath = fileManager.getOrCreateFolder(name: Self.cacheFolderName)?.appendingPathComponent(name).path
    if (loadExpiredData || !isResourceExpired(resource, localeIdentifier: localeIdentifier)),
       let cachedPath = cachedPath,
       fileManager.fileExists(atPath: cachedPath) {
      return await on(queue: todayQueue) {
        return fileManager.contents(atPath: cachedPath)
      }
    }
    return nil
  }

  /// Load a Brave News resource either from a file cache or the web
  ///
  /// The `filename` provided will be appended as a path component to the request URL, and be used to
  /// fetch the cache and save the response so it should include the full path for the endpoint (For
  /// example: `sources.json`)
  ///
  /// Cache lifetime will be based on the modification date of the cached file. Data downloaded from the web
  /// will only be cached if it is successfully decoded into the given `DataType`.
  private func loadResource<DataType>(
    _ resource: NewsResource,
    localeIdentifier: String? = nil,
    decodedTo: DataType.Type
  ) async throws -> DataType where DataType: Decodable {
    assert(!resource.isLocalized || localeIdentifier != nil)
    func data(for resource: NewsResource) async throws -> Data {
      if let cachedData = await cachedResource(resource, localeIdentifier: localeIdentifier) {
        return cachedData
      }
      return try await withCheckedThrowingContinuation { continuation in
        guard let url = self.resourceUrl(for: resource.bucket) else {
          fatalError("Incorrect URL generated for the given resource: \(resource)")
        }
        self.session.dataTask(with: url.appendingPathComponent(filename)) { data, response, error in
          if let error = error {
            continuation.resume(throwing: error)
            return
          }
          continuation.resume(returning: data ?? Data())
        }
        .resume()
      }
    }
    let filename = resourceFilename(for: resource, localeIdentifier: localeIdentifier)
    let data = try await data(for: resource)
    let decodedResource = try self.decoder.decode(DataType.self, from: data)
    if !data.isEmpty {
      if !FileManager.default.writeToDiskInFolder(data, fileName: filename, folderName: Self.cacheFolderName) {
        Logger.module.error("Failed to write sources to disk")
      }
    }
    return decodedResource
  }

  private func restoreCachedSources() {
    Task { @MainActor in
      guard let data = await cachedResource(.globalSources, loadExpiredData: true) else { return }
      do {
        let decodedResource = try await onThrowing(queue: todayQueue) {
          return try self.decoder
            .decode([FailableDecodable<FeedItem.Source>].self, from: data)
            .compactMap(\.wrappedValue)
        }
        self.sources = decodedResource
      } catch {
        // Could be a source type change, so may not be a big issue. If the user goes to download
        // updated lists and it still fails it will show an error on the feed
        Logger.module.debug("Failed to decode previously cached sources: \(error.localizedDescription)")
      }
    }
  }
  
  private func loadGlobalSources() async throws -> [FeedItem.Source] {
    let sources = try await loadResource(.globalSources, decodedTo: [FailableDecodable<FeedItem.Source>].self)
    if sources.isEmpty {
      throw BraveNewsError.resourceEmpty
    }
    return sources.compactMap(\.wrappedValue)
  }

  private func loadLegacySources(for localeIdentifier: String) async throws -> [FeedItem.LegacySource] {
    let sources = try await loadResource(.sources, localeIdentifier: localeIdentifier, decodedTo: [FailableDecodable<FeedItem.LegacySource>].self)
    if sources.isEmpty {
      throw BraveNewsError.resourceEmpty
    }
    return sources.compactMap(\.wrappedValue)
  }

  private func loadFeed(for localeIdentifier: String) async throws -> [FeedItem.Content] {
    let items = try await loadResource(.feed, localeIdentifier: localeIdentifier, decodedTo: [FailableDecodable<FeedItem.Content>].self)
    if items.isEmpty {
      throw BraveNewsError.resourceEmpty
    }
    return items.compactMap(\.wrappedValue)
  }
  
  private func loadSourceSuggestions(for localeIdentifier: String) async throws -> [String: [FeedItem.SourceSimilarity]] {
    let items = try await loadResource(.sourceSuggestions, localeIdentifier: localeIdentifier, decodedTo: [String: [FailableDecodable<FeedItem.SourceSimilarity>]].self)
    if items.isEmpty {
      throw BraveNewsError.resourceEmpty
    }
    return items.mapValues { $0.compactMap(\.wrappedValue) }
  }

  /// Describes a single RSS feed's loaded data set converted into Brave News based data
  private struct RSSDataFeed {
    var source: FeedItem.Source
    var items: [FeedItem.Content]
  }

  private func loadRSSLocation(_ location: RSSFeedLocation) async throws -> RSSDataFeed {
    let parser = FeedParser(URL: location.url)
    return try await withCheckedThrowingContinuation { continuation in
      parser.parseAsync { [weak self] result in
        switch result {
        case .success(let feed):
          if let source = FeedItem.Source(from: feed, location: location) {
            var content: [FeedItem.Content] = []
            switch feed {
            case .atom(let atomFeed):
              if let feedItems = atomFeed.entries?.compactMap({ entry -> FeedItem.Content? in
                FeedItem.Content(from: entry, location: location)
              }) {
                content = feedItems
              }
            case .rss(let rssFeed):
              if let feedItems = rssFeed.items?.compactMap({ entry -> FeedItem.Content? in
                FeedItem.Content(from: entry, location: location)
              }) {
                content = feedItems
              }
            case .json(let jsonFeed):
              if let feedItems = jsonFeed.items?.compactMap({ entry -> FeedItem.Content? in
                FeedItem.Content(from: entry, location: location)
              }) {
                content = feedItems
              }
            }
            guard let self = self else {
              continuation.resume(throwing: BraveNewsError.unknownError)
              return
            }
            content = self.scored(rssItems: content)
            continuation.resume(returning: .init(source: source, items: content))
          }
        case .failure(let error):
          continuation.resume(throwing: error)
        }
      }
    }
  }

  /// Load all RSS feeds that the user has enabled
  @MainActor private func loadRSSFeeds() async -> [Result<RSSDataFeed, Error>] {
    let locations = rssFeedLocations.filter(isRSSFeedEnabled)
    return await withTaskGroup(
      of: Result<RSSDataFeed, Error>.self,
      returning: [Result<RSSDataFeed, Error>].self
    ) { group in
      for location in locations {
        group.addTask {
          do {
            return .success(try await self.loadRSSLocation(location))
          } catch {
            return .failure(error)
          }
        }
      }
      var results: [Result<RSSDataFeed, Error>] = []
      for await result in group {
        results.append(result)
      }
      return results
    }
  }

  /// Scores RSS items similar to how the backend scores regular Brave News sources
  private func scored(rssItems: [FeedItem.Content]) -> [FeedItem.Content] {
    var varianceBySource: [String: Double] = [:]
    return rssItems.map {
      var content = $0
      let recency = log(max(1, -content.publishTime.timeIntervalSinceNow))
      let variance = (varianceBySource[content.publisherID] ?? 1.0) * 2.0
      varianceBySource[content.publisherID] = variance
      content.baseScore = recency * variance
      return content
    }
  }

  /// Whether or not we should load content or just use what's in `state`.
  ///
  /// If the data source is already loading, returns `false`
  public var shouldLoadContent: Bool {
    switch state {
    case .initial, .failure:
      return true
    case .loading:
      return false
    case .success:
      return isFeedContentExpired || isSourcesExpired || needsReloadCards
    }
  }

  /// Whether or not the feed content is currently expired and needs to be reloaded
  public var isFeedContentExpired: Bool {
    return followedLocales.allSatisfy({ isResourceExpired(.feed, localeIdentifier: $0) })
  }

  /// Whether or not the sources are currently expired and needs to be reloaded
  public var isSourcesExpired: Bool {
    isResourceExpired(.globalSources)
  }
  
  /// A set of locales that the user is following
  private var followedLocales: Set<String> {
    var locales: Set<String> = []
    // Followed channels
    locales.formUnion(followedChannels.map(\.localeIdentifier))
    // Followed sources
    let localeDetails = followedSources.map(\.localeDetails).compactMap { $0 }
    for detail in localeDetails where !detail.isEmpty {
      // Sources are tricky, they can exist in multiple locales even though the contents of those will be the
      // same in each feed.
      //
      // We don't want to unnecessarily download extra feeds so it'll be best to only download from 1 that
      // we're already downloading from.
      if detail.count == 1 {
        // Only 1 to work with
        locales.insert(detail.first!.locale)
      } else {
        // Try and see if it exists in a feed we're already downloading
        if let locale = detail.first(where: { locales.contains($0.locale) })?.locale {
          locales.insert(locale)
        } else if let locale = locales.first {
          // Nothing found so we'll just pick one
          locales.insert(locale)
        }
      }
    }
    return locales
  }

  /// Loads Brave News resources and generates cards for the loaded data. The result will be placed in
  /// the `state` property.
  ///
  /// Resources are loaded either from cache (if the cache is valid for said resource) or from the web,
  /// scored, and then used to generate a list of `FeedCard` objects.
  ///
  /// Given the nature of async card regeneration, calling this method will always set the state to
  /// `loading` initially.
  public func load(_ completion: (() -> Void)? = nil) {
    state = .loading(state)
    Task { @MainActor in
      do {
        self.sources = try await loadGlobalSources()
        if !Preferences.BraveNews.isNewsRevampSetUpCompleted.value {
          defer { Preferences.BraveNews.isNewsRevampSetUpCompleted.value = true }
          // If the user in a matching region we will persist that locale as their selected one
          let currentLocaleIdentifier = Locale.current.identifier
          if availableLocales.contains(currentLocaleIdentifier) {
            Preferences.BraveNews.selectedLocale.value = currentLocaleIdentifier
            self.selectedLocale = currentLocaleIdentifier
          }
          // Regardless of in a matching region the user will always auto-follow the "Top Sources" channel on
          // migration
          Preferences.BraveNews.followedChannels.value[self.selectedLocale, default: []].append("Top Sources")
          purgeDisabledRSSLocations()
        }
        self.items = []
        self.sourceSuggestions = [:]
        for locale in followedLocales {
          async let suggestions = loadSourceSuggestions(for: locale)
          async let items = loadFeed(for: locale)
          let (localeSpecificItems, localeSpecificSuggestions) = try await (items, suggestions)
          self.items.append(contentsOf: localeSpecificItems)
          self.sourceSuggestions.merge(with: localeSpecificSuggestions)
        }
        for result in await self.loadRSSFeeds() {
          switch result {
          case .success(let feed):
            self.sources.append(feed.source)
            self.items.append(contentsOf: feed.items)
          case .failure:
            // At the moment we dont handle any load errors once the feed has been
            // added
            break
          }
        }
        self.reloadCards(from: self.items, sources: self.sources, completion: completion)
      } catch {
        self.state = .failure(error)
        completion?()
      }
    }
  }

  /// Clears any cached files from the users device
  @discardableResult
  public func clearCachedFiles() -> Bool {
    do {
      let fileManager = FileManager.default
      if let braveNewsPath = fileManager.getOrCreateFolder(name: Self.cacheFolderName) {
        let filePaths = try fileManager.contentsOfDirectory(atPath: braveNewsPath.path)
        try filePaths.forEach {
          var fileUrl = braveNewsPath
          fileUrl.appendPathComponent($0)
          try fileManager.removeItem(atPath: fileUrl.path)
        }
      }
    } catch {
      Logger.module.error("Could not remove cached files")
      return false
    }
    return true
  }

  // MARK: - Sources

  public static let topNewsCategory = "Top News"

  /// Whether or not a source is currently enabled (whether or not by default or by a user changing
  /// said default)
  public func isSourceEnabled(_ source: FeedItem.Source) -> Bool {
    FeedSourceOverride.get(fromId: source.id)?.enabled ?? source.isDefault
  }

  /// Toggle a source's enabled status
  public func toggleSourceHidden(_ source: FeedItem.Source, hidden: Bool) {
    if hidden {
      FeedSourceOverride.setEnabled(forId: source.id, enabled: false)
    } else {
      FeedSourceOverride.resetStatus(forId: source.id)
    }

    if let cards = state.cards, cards.isEmpty && !hidden {
      // If we're enabling a source and we don't have any items because their source selection was
      // causing an empty generation, regenerate the cards
      reloadCards(from: self.items, sources: self.sources)
    } else {
      needsReloadCards = true
    }
  }
  
  /// Reset all source settings back to default
  public func resetSourcesToDefault() {
    FeedSourceOverride.resetSourceSelection()
    needsReloadCards = true
  }
  
  func isFollowingChannelBinding(channel: FeedChannel) -> Binding<Bool> {
    .init {
      Preferences.BraveNews.followedChannels.value[channel.localeIdentifier]?.contains(channel.name) ?? false
    } set: { [self] newValue in
      if newValue {
        Preferences.BraveNews.followedChannels.value[channel.localeIdentifier, default: []].append(channel.name)
      } else {
        Preferences.BraveNews.followedChannels.value[channel.localeIdentifier, default: []].removeAll(where: { $0 == channel.name })
      }
      if let cards = state.cards, cards.isEmpty && newValue {
        // If we're enabling a category and we don't have any items because their source selection was
        // causing an empty generation, regenerate the cards
        reloadCards(from: self.items, sources: self.sources)
      } else {
        needsReloadCards = true
      }
      Preferences.Review.braveNewsCriteriaPassed.value = true
      objectWillChange.send()
    }
  }
  
  func isFollowingSourceBinding(source: FeedItem.Source) -> Binding<Bool> {
    .init {
      FeedSourceOverride.get(fromId: source.id)?.enabled ?? false
    } set: { [self] newValue in
      if newValue {
        FeedSourceOverride.setEnabled(forId: source.id, enabled: true)
      } else {
        // Unfollowing doesn't actually set the enabled status to false anymore, simply resets the override
        FeedSourceOverride.resetStatus(forId: source.id)
      }
      if let cards = state.cards, cards.isEmpty && newValue {
        // If we're enabling a category and we don't have any items because their source selection was
        // causing an empty generation, regenerate the cards
        reloadCards(from: self.items, sources: self.sources)
      } else {
        needsReloadCards = true
      }
      Preferences.Review.braveNewsCriteriaPassed.value = true
      objectWillChange.send()
    }
  }
  
  /// Searches sources, channels and RSS feeds
  @MainActor func search(query: String) -> SearchResults? {
    let sourceResults = sources.filter({
      $0.name.localizedCaseInsensitiveContains(query) ||
      $0.siteURL?.absoluteString.localizedCaseInsensitiveContains(query) == true
    })
    var channelResults: Set<FeedChannel> = Set(channels.filter({ $0.localizedCaseInsensitiveContains(query) }).map({
      .init(localeIdentifier: self.selectedLocale, name: $0)
    }))
    // Add any followed channels that aren't from the selected locale
    channelResults.formUnion(followedChannels.filter({ $0.name.localizedCaseInsensitiveContains(query) }))
    let rssFeeds = rssFeedLocations.filter {
      $0.title?.localizedCaseInsensitiveContains(query) == true ||
      $0.url.absoluteString.localizedCaseInsensitiveContains(query)
    }
    return .init(sources: sourceResults, channels: Array(channelResults), rssFeeds: rssFeeds)
  }

  // MARK: - Card Generation

  /// Whether or not cards need to be reloaded next time we attempt to request state data
  private var needsReloadCards = false

  /// Notify the feed data source that it needs to reload cards next time we request state data
  public func setNeedsReloadCards() {
    needsReloadCards = true
  }

  /// Scores and generates cards from a set of items and sources
  private func reloadCards(
    from items: [FeedItem.Content],
    sources: [FeedItem.Source],
    completion: (() -> Void)? = nil
  ) {
    needsReloadCards = false
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
    let lastVisitedDomains =
      (try? History.suffix(200)
        .lazy
        .compactMap(\.url)
        .compactMap { URL(string: $0)?.baseDomain }) ?? []
    let followedSources = FeedSourceOverride.all().filter(\.enabled).map(\.publisherID)
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
        if followedSources.contains(where: { $0 == source.id }) {
          score -= 5
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
    var feedsFromEnabledSources = Set(items.filter { item in
      overridenSources.first(where: {
        $0.publisherID == item.source.id
      })?.enabled ?? false
    })
    for (key, value) in Preferences.BraveNews.followedChannels.value {
      let channelForLocale = Set(value)
      feedsFromEnabledSources.formUnion(Set(items.filter({
        $0.source.localeDetails?.contains(where: {
          $0.locale == key && !$0.channels.intersection(channelForLocale).isEmpty
        }) ?? false
      })))
    }
    let feedItems = Array(feedsFromEnabledSources)
    var sponsors = feedItems.filter { $0.content.contentType == .sponsor }
    var partners = feedItems.filter { $0.content.contentType == .partner }
    var deals = feedItems.filter { $0.content.contentType == .deals }
    var articles = feedItems.filter { $0.content.contentType == .article }

    let dealsCategoryFillStrategy = CategoryFillStrategy(
      categories: Set(deals.compactMap(\.content.offersCategory)),
      category: \.content.offersCategory
    )

    ads?.purgeOrphanedAdEvents(.inlineContentAd) { _ in }
    var contentAdsQueryFailed = false

    let rules: [FeedSequenceElement] = [
      .sponsor,
      .fillUsing(
        FilteredFillStrategy(isIncluded: { $0.source.category == Self.topNewsCategory }),
        fallback: DefaultFillStrategy(),
        [
          .headline(paired: false)
        ]),
      .braveAd,
      .repeating([
        .repeating([.headline(paired: false)], times: 2),
        .headline(paired: true),
        .partner,
        .fillUsing(
          CategoryFillStrategy(
            categories: Set(articles.map(\.source.category)),
            category: \.source.category,
            initialCategory: Self.topNewsCategory
          ),
          [
            .categoryGroup
          ]
        ),
        .repeating([.headline(paired: false)], times: 2),
        .repeating([.headline(paired: true)], times: 2),
        .braveAd,
        .repeating([.headline(paired: false)], times: 2),
        .brandedGroup(numbered: true),
        .group,
        .fillUsing(
          RandomizedFillStrategy(isIncluded: { Date().timeIntervalSince($0.content.publishTime) < 48.hours }),
          [
            .headline(paired: false)
          ]),
        .fillUsing(
          dealsCategoryFillStrategy,
          [
            .deals
          ]),
        .fillUsing(
          RandomizedFillStrategy(isIncluded: { Date().timeIntervalSince($0.content.publishTime) < 48.hours }),
          [
            .headline(paired: true),
            .headline(paired: false),
          ]),
      ]),
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
          return [.deals($0, title: title ?? Strings.BraveNews.deals)]
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
        guard !contentAdsQueryFailed, let ads = ads else { return nil }
        let group = DispatchGroup()
        group.enter()
        var contentAd: InlineContentAd?
        DispatchQueue.main.async {
          ads.inlineContentAds(
            dimensions: "900x750",
            completion: { dimensions, ad in
              if let ad = ad {
                contentAd = ad
              } else {
                contentAdsQueryFailed = true
                Logger.module.debug("Inline content ads could not be filled; Skipping for the rest of this feed generation")
              }
              group.leave()
            })
        }
        let result = group.wait(timeout: .now() + .seconds(1))
        if result == .success, let ad = contentAd {
          return [.ad(ad)]
        }
        return nil
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
      case .fillUsing(let strategy, let fallbackStrategy, let elements):
        var cards: [FeedCard] = []
        for element in elements {
          if let elementCards = _cards(for: element, fillStrategy: strategy) {
            cards.append(contentsOf: elementCards)
          } else {
            if let fallbackStrategy, let elementCards = _cards(for: element, fillStrategy: fallbackStrategy) {
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
      if generatedCards.count < 10,
        generatedCards.allSatisfy({
          if case .ad = $0 {
            return true
          }
          return false
        }) {
        // If there are less than 10 cards and they all are ads, show nothing
        generatedCards.removeAll()
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
    /// Display a headline from a list of partnered items
    case partner
    /// Displays a Brave ad from the ads catalog
    case braveAd
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
    /// feed list. You can provide a fallback strategy that will be used if the strategy provided does not
    /// yield any results.
    indirect case fillUsing(_ strategy: FillStrategy, fallback: FillStrategy? = nil, _ elements: [FeedSequenceElement])
    /// Displays the provided elements a number of times. Passing in `.max` for `times` means it will repeat
    /// until there is no more content available
    indirect case repeating([FeedSequenceElement], times: Int = .max)
  }

}

struct SearchResults {
  var sources: [FeedItem.Source]
  var channels: [FeedChannel]
  var rssFeeds: [RSSFeedLocation]
  
  static var empty: Self {
    .init(sources: [], channels: [], rssFeeds: [])
  }
  
  var isEmpty: Bool {
    sources.isEmpty && channels.isEmpty && rssFeeds.isEmpty
  }
}
