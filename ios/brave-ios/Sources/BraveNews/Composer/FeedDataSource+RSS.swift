// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import FeedKit
import Fuzi
import Shared
import Growth
import SwiftUI

public struct RSSFeedLocation: Hashable, Identifiable {
  public var title: String?
  public var url: URL

  public var id: String {
    url.absoluteString
  }
  
  public init(title: String? = nil, url: URL) {
    self.title = title
    self.url = url
  }
}

extension FeedDataSource {
  // MARK: - RSS Sources

  @MainActor public var rssFeedLocations: [RSSFeedLocation] {
    RSSFeedSource.all().compactMap {
      guard let url = URL(string: $0.feedUrl) else { return nil }
      return RSSFeedLocation(title: $0.title, url: url)
    }
  }

  /// Add a users custom RSS feed to the list of sources
  ///
  /// - returns: `true` if the feed is successfully added, `false` if it already exists or the
  ///            url location is not a web page url
  @discardableResult
  public func addRSSFeedLocation(_ location: RSSFeedLocation) -> Bool {
    if !location.url.isWebPage(includeDataURIs: false), !InternalURL.isValid(url: location.url) {
      return false
    }
    if let firstPartySource = sources.first(where: { $0.feedURL == location.url }) {
      // If there is a source with the same feed URL already we will just follow it instead.
      isFollowingSourceBinding(source: firstPartySource).wrappedValue = true
      return true
    }
    let feedUrl = location.url.absoluteString
    if RSSFeedSource.get(with: feedUrl) != nil {
      return false
    }
    RSSFeedSource.insert(
      title: location.title,
      feedUrl: feedUrl)
    setNeedsReloadCards()
    recordTotalExternalFeedsP3A()
    recordExternalFeedCountChange(1)
    objectWillChange.send()
    return true
  }

  /// Remove a users custom RSS feed to the list of sources
  public func removeRSSFeed(_ location: RSSFeedLocation) {
    let feedUrl = location.url.absoluteString
    if RSSFeedSource.get(with: feedUrl) == nil {
      return
    }
    RSSFeedSource.delete(with: feedUrl)
    FeedSourceOverride.resetStatus(forId: location.id)
    setNeedsReloadCards()
    recordTotalExternalFeedsP3A()
    recordExternalFeedCountChange(-1)
    objectWillChange.send()
  }

  /// Whether or not an RSS feed is currently enabled
  ///
  /// - note: RSS Feeds are enabled by default since they are added by the user
  public func isRSSFeedEnabled(_ location: RSSFeedLocation) -> Bool {
    FeedSourceOverride.get(fromId: location.id)?.enabled ?? true
  }
  
  @MainActor public func isFollowingRSSFeedBinding(feed: RSSFeedLocation) -> Binding<Bool> {
    .init {
      self.rssFeedLocations.contains(where: { $0.id == feed.id })
    } set: { [self] newValue in
      // In news revamp, you cannot enable or disable an RSS feed, unfollowing results in removing it entirely
      if newValue {
        addRSSFeedLocation(feed)
      } else {
        removeRSSFeed(feed)
      }
      objectWillChange.send()
    }
  }
  
  public func updateRSSFeed(feed: RSSFeedLocation, title: String) {
    RSSFeedSource.update(feedUrl: feed.id, title: title)
  }
  
  @MainActor public func purgeDisabledRSSLocations() {
    // News 2.0 no longer allows keeping RSS feeds, so this will attempt to remove any RSS feeds the user has
    // specifically disabled
    let locations = Set(rssFeedLocations.map(\.id))
    let disabledLocations = FeedSourceOverride.all().filter {
      !$0.enabled && locations.contains($0.publisherID)
    }
    for location in disabledLocations {
      guard let url = URL(string: location.publisherID) else { continue }
      removeRSSFeed(.init(title: nil, url: url))
    }
  }
  
  // MARK: - P3A
  
  func recordTotalExternalFeedsP3A() {
    // Q49 How many external feeds do you have in total?
    Task { @MainActor in
      UmaHistogramRecordValueToBucket(
        "Brave.Today.DirectFeedsTotal",
        buckets: [0, 1, 2, 3, 4, 5, .r(6...10), .r(11...)],
        value: rssFeedLocations.count
      )
    }
  }
  
  func recordExternalFeedCountChange(_ delta: Int) {
    // Q48 How many external feeds did you add last week?
    var storage = P3ATimedStorage<Int>.rssFeedCountStorage
    storage.add(value: delta, to: Date())
    UmaHistogramRecordValueToBucket(
      "Brave.Today.WeeklyAddedDirectFeedsCount",
      buckets: [0, 1, 2, 3, 4, 5, .r(6...10), .r(11...)],
      value: storage.combinedValue
    )
  }
}

extension FeedItem.Content {
  private static func imageURL(from document: HTMLDocument, releativeTo baseURL: URL?) -> URL? {
    if let src = document.firstChild(xpath: "//img[@src]")?.attr("src"),
      let url = URL(string: src, relativeTo: baseURL),
      url.isWebPage(includeDataURIs: false), !InternalURL.isValid(url: url) {
      return url
    }
    return nil
  }

  private static func descriptionText(from document: HTMLDocument) -> String? {
    if let text = document.root?.childNodes(ofTypes: [.Text, .Element]).map({ node in
      node.stringValue
        .trimmingCharacters(in: .whitespacesAndNewlines)
        .replacingOccurrences(of: "\n", with: " ")
    }).joined(separator: " ") {
      return text
    }
    return nil
  }

  init?(from feedItem: JSONFeedItem, location: RSSFeedLocation) {
    guard let publishTime = feedItem.datePublished,
      let url = feedItem.url?.asURL,
      let title = feedItem.title,
      url.isWebPage()
    else {
      return nil
    }
    var description = ""
    var imageURL: URL?
    if let image = feedItem.image, let url = URL(string: image, relativeTo: location.url.domainURL),
      url.isWebPage(includeDataURIs: false), !InternalURL.isValid(url: url) {
      imageURL = url
    }
    if let text = feedItem.contentText {
      description = text
    }
    if let html = feedItem.contentHtml, let doc = try? HTMLDocument(string: html) {
      if imageURL == nil,
        let imageURLFromHTML = Self.imageURL(from: doc, releativeTo: location.url.domainURL) {
        imageURL = imageURLFromHTML
      }
      if description.isEmpty, let text = Self.descriptionText(from: doc) {
        description = text
      }
    }
    self.init(
      publishTime: publishTime,
      url: url,
      imageURL: imageURL,
      title: title,
      description: description,
      contentType: .article,
      publisherID: location.id,
      urlHash: url.absoluteString,
      baseScore: 0,
      offersCategory: nil
    )
  }
  init?(from feedItem: AtomFeedEntry, location: RSSFeedLocation) {
    // Attempts to get the blog post URL from a set of URLs posted in the entry.
    //
    // Sometimes link blogs will post multiple <link> entries in their RSS feeds. This is more clear in
    // JSON RSS which have `url` and `external_url` fields and are more concise.
    func entryURL(from urls: [URL], feedURL: URL) -> URL? {
      guard let feedURLDomain = feedURL.baseDomain,
            let postURL = urls.first(where: { $0.baseDomain == feedURLDomain }) else {
        return urls.first
      }
      return postURL
    }
    guard let publishTime = feedItem.published,
      let urls = feedItem.links?.compactMap({ $0.attributes?.href?.asURL }),
      let url = entryURL(from: urls, feedURL: location.url),
      let title = feedItem.title,
      url.isWebPage()
    else {
      return nil
    }
    var description = ""
    var imageURL: URL?
    if let thumbnail = feedItem.media?.mediaThumbnails?.first?.attributes?.url,
      let url = URL(string: thumbnail, relativeTo: location.url.domainURL),
      url.isWebPage(includeDataURIs: false), !InternalURL.isValid(url: url) {
      imageURL = url
    }
    if feedItem.summary?.attributes?.type == "text" {
      description = feedItem.summary?.value ?? ""
    } else if feedItem.content?.attributes?.type == "html",
      let html = feedItem.content?.value,
      let doc = try? HTMLDocument(string: html) {
      // Find one in description?
      if imageURL == nil,
        let imageURLFromHTML = Self.imageURL(from: doc, releativeTo: location.url.domainURL) {
        imageURL = imageURLFromHTML
      }
      if description.isEmpty, let text = Self.descriptionText(from: doc) {
        description = text
      }
    }
    self.init(
      publishTime: publishTime,
      url: url,
      imageURL: imageURL,
      title: title,
      description: description,
      contentType: .article,
      publisherID: location.id,
      urlHash: url.absoluteString,
      baseScore: 0,
      offersCategory: nil
    )
  }
  init?(from feedItem: RSSFeedItem, location: RSSFeedLocation) {
    guard let publishTime = feedItem.pubDate,
      let href = feedItem.link,
      let url = URL(string: href),
      let title = feedItem.title,
      url.isWebPage()
    else {
      return nil
    }
    var description = ""
    var imageURL: URL?
    if let thumbnail = feedItem.media?.mediaThumbnails?.first?.attributes?.url,
      let url = URL(string: thumbnail, relativeTo: location.url.domainURL),
      url.isWebPage(includeDataURIs: false), !InternalURL.isValid(url: url) {
      imageURL = url
    }
    if let html = feedItem.description, let doc = try? HTMLDocument(string: html) {
      if imageURL == nil,
        let imageURLFromHTML = Self.imageURL(from: doc, releativeTo: location.url.domainURL) {
        imageURL = imageURLFromHTML
      }
      if description.isEmpty, let text = Self.descriptionText(from: doc) {
        description = text
      }
    }
    self.init(
      publishTime: publishTime,
      url: url,
      imageURL: imageURL,
      title: title,
      description: description,
      contentType: .article,
      publisherID: location.id,
      urlHash: url.absoluteString,
      baseScore: 0,
      offersCategory: nil
    )
  }
}

extension FeedItem.LegacySource {
  init?(from feed: FeedKit.Feed, location: RSSFeedLocation) {
    let id = location.id
    guard let title = feed.title else { return nil }
    self.init(id: id, isDefault: true, category: "", name: title, isUserSource: true)
  }
}

extension FeedItem.Source {
  init?(from feed: FeedKit.Feed, location: RSSFeedLocation) {
    let id = location.id
    guard let title = feed.title else { return nil }
    self.init(id: id, isDefault: true, category: "", name: title, isUserSource: true, destinationDomains: [])
  }
}

extension Feed {
  public var title: String? {
    switch self {
    case .atom(let feed):
      return feed.title
    case .rss(let feed):
      return feed.title
    case .json(let feed):
      return feed.title
    }
  }
}

extension P3ATimedStorage where Value == Int {
  fileprivate static var rssFeedCountStorage: Self { .init(name: "rss-feeds-added", lifetimeInDays: 7) }
}
