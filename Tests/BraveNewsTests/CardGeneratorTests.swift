// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import XCTest
@testable import BraveNews

extension FeedCardGenerator {
  /// Convenience method for tests to retrieve all cards in the sequence
  fileprivate var allCards: [FeedCard] {
    get async throws {
      var result: [FeedCard] = []
      for try await cards in self {
        result.append(contentsOf: cards)
      }
      return result
    }
  }
}

final class CardGeneratorTests: XCTestCase {
  
  private static let mockSources: [FeedItem.Source] = [
    .init(id: "1", isDefault: false, category: "Top News", name: "Top News", destinationDomains: [], localeDetails: [.init(channels: ["Top Sources"], locale: "en_US")]),
    .init(id: "2", isDefault: false, category: "Top News", name: "Top News 2", destinationDomains: [], localeDetails: [.init(channels: ["Top Sources"], locale: "en_US")]),
    .init(id: "3", isDefault: false, category: "Top News", name: "Top News 3", destinationDomains: [], localeDetails: [.init(channels: ["Top Sources"], locale: "en_US")]),
    .init(id: "4", isDefault: false, category: "Technology", name: "Technology", destinationDomains: [], localeDetails: [.init(channels: ["Technology"], locale: "ja_JP")]),
    .init(id: "5", isDefault: false, category: "Technology", name: "Technology 2", destinationDomains: [], localeDetails: [.init(channels: ["Technology"], locale: "ja_JP")]),
    .init(id: "6", isDefault: false, category: "Technology", name: "Technology 3", destinationDomains: [], localeDetails: [.init(channels: ["Technology"], locale: "ja_JP")]),
  ]
  
  private static let scoredItems: [FeedItem] = Range<Int>(1...6)
    .compactMap { id -> FeedItem? in
      guard let source = mockSources.first(where: { $0.id == String(id) }) else { return nil }
      return .init(
        score: Double(id),
        content: .init(
          publishTime: Date(),
          imageURL: URL(string: "image")!,
          title: "",
          description: "",
          contentType: .article,
          publisherID: String(id),
          urlHash: "1",
          baseScore: Double(id)
        ),
        source: source
      )
    }
  
  /// Tests that no cards will generate when the user follows no sources or channels within the list of items
  func testEmptyFollowList() async throws {
    let sequence: [FeedSequenceElement] = [
      .headline(paired: false)
    ]
    let generator = FeedCardGenerator(
      scoredItems: Self.scoredItems,
      sequence: sequence,
      followedSources: [],
      hiddenSources: [],
      followedChannels: [:],
      ads: nil
    )
    let cards = try await generator.allCards
    XCTAssertTrue(cards.isEmpty)
  }
  
  /// Tests that following a channel will render cards from sources that the user doesn't follow explicilty
  func testFollowedChannel() async throws {
    let sequence: [FeedSequenceElement] = [
      .repeating([
        .headline(paired: false),
      ]),
    ]
    let generator = FeedCardGenerator(
      scoredItems: Self.scoredItems,
      sequence: sequence,
      followedSources: [],
      hiddenSources: [],
      followedChannels: ["en_US": ["Top Sources"]],
      ads: nil
    )
    let expectedItems = Self.scoredItems.filter({
      $0.source.localeDetails?.contains(where: {
        $0.locale == "en_US" && !$0.channels.intersection(["Top Sources"]).isEmpty
      }) ?? false
    })
    let cards = try await generator.allCards
    XCTAssertFalse(cards.isEmpty)
    XCTAssertEqual(cards.map(\.items).flatMap { $0 }, expectedItems)
  }
  
  /// Tests that hidden sources dont generate cards even if they exist in a followed channel
  func testHiddenSources() async throws {
    let sequence: [FeedSequenceElement] = [
      .repeating([
        .headline(paired: false),
      ]),
    ]
    let generator = FeedCardGenerator(
      scoredItems: Self.scoredItems,
      sequence: sequence,
      followedSources: [],
      hiddenSources: ["1"],
      followedChannels: ["en_US": ["Top Sources"]],
      ads: nil
    )
    let expectedItems = Self.scoredItems.filter({
      if $0.source.id == "1" { return false }
      return $0.source.localeDetails?.contains(where: {
        $0.locale == "en_US" && !$0.channels.intersection(["Top Sources"]).isEmpty
      }) ?? false
    })
    let cards = try await generator.allCards
    XCTAssertFalse(cards.isEmpty)
    XCTAssertEqual(cards.map(\.items).flatMap { $0 }, expectedItems)
  }
  
  /// Tests that an empty sequence results in no cards
  func testEmptySequence() async throws {
    let generator = FeedCardGenerator(
      scoredItems: Self.scoredItems,
      sequence: [],
      followedSources: [],
      hiddenSources: [],
      followedChannels: [:],
      ads: nil
    )
    let cards = try await generator.allCards
    XCTAssertTrue(cards.isEmpty)
  }
  
  /// Tests a finite sequence that only generates cards until rules run out
  func testFiniteSequence() async throws {
    let sequence: [FeedSequenceElement] = [
      .headline(paired: false),
    ]
    let generator = FeedCardGenerator(
      scoredItems: Self.scoredItems,
      sequence: sequence,
      followedSources: Set(Self.mockSources.map(\.id)),
      hiddenSources: [],
      followedChannels: [:],
      ads: nil
    )
    let expectedItem = Self.scoredItems.first(where: { $0.content.contentType == .article })
    let cards = try await generator.allCards
    XCTAssertEqual(cards.count, 1)
    if case .headline(let item) = cards.first {
      XCTAssertEqual(item, expectedItem)
    } else {
      XCTFail()
    }
  }
  
  /// Tests generating cards using a repeating sequence that will fill cards until there are none left to fill
  func testRepeatingSequence() async throws {
    let sequence: [FeedSequenceElement] = [
      .repeating([
        .headline(paired: false),
      ]),
    ]
    let generator = FeedCardGenerator(
      scoredItems: Self.scoredItems,
      sequence: sequence,
      followedSources: Set(Self.mockSources.map(\.id)),
      hiddenSources: [],
      followedChannels: [:],
      ads: nil
    )
    let expectedItems = Self.scoredItems.filter({
      $0.content.contentType == .article && $0.content.imageURL != nil
    })
    let cards = try await generator.allCards
    XCTAssertEqual(cards.count, expectedItems.count)
    XCTAssertTrue(cards.allSatisfy({
      if case .headline = $0 {
        return true
      }
      return false
    }))
    XCTAssertEqual(cards.map(\.items).flatMap { $0 }, expectedItems)
  }
  
  /// Tests generating a category group card, which by default will always be a Top News category initially
  func testInitialCategoryGroup() async throws {
    let sequence: [FeedSequenceElement] = [
      .categoryGroup
    ]
    let generator = FeedCardGenerator(
      scoredItems: Self.scoredItems,
      sequence: sequence,
      followedSources: Set(Self.mockSources.map(\.id)),
      hiddenSources: [],
      followedChannels: [:],
      ads: nil
    )
    let expectedItems = Self.scoredItems.filter({
      $0.content.contentType == .article && $0.source.category == "Top News"
    })
    let cards = try await generator.allCards
    XCTAssertEqual(cards.count, 1)
    XCTAssertTrue(cards.allSatisfy({
      if case .group = $0 {
        return true
      }
      return false
    }))
    XCTAssertEqual(cards.map(\.items).flatMap { $0 }, expectedItems)
  }
  
  /// Tests generating a deals card with feed items that have the `deals` content type and an offers category
  func testDealsCard() async throws {
    let sequence: [FeedSequenceElement] = [
      .deals
    ]
    let dealsSource = FeedItem.Source(id: "1", isDefault: false, category: "Deals", name: "Deals", destinationDomains: [], backgroundColor: nil, localeDetails: [.init(channels: ["Brave"], locale: "en_US")])
    let dealsItems: [FeedItem] = (1...3).map {
      .init(score: Double($0), content: .init(publishTime: Date(), title: "", description: "", contentType: .deals, publisherID: dealsSource.id, urlHash: "\($0)", baseScore: Double($0), offersCategory: "Deal"), source: dealsSource)
    }
    let generator = FeedCardGenerator(
      scoredItems: dealsItems,
      sequence: sequence,
      followedSources: [dealsSource.id],
      hiddenSources: [],
      followedChannels: [:],
      ads: nil
    )
    let cards = try await generator.allCards
    XCTAssertEqual(cards.count, 1)
    if case .deals(let items, _) = cards.first {
      XCTAssertEqual(items, dealsItems)
    } else {
      XCTFail()
    }
  }
  
  /// Tests that feed items are scored properly when following a source that is also included in channel
  func testScoringFromSourcesAndChannelFollow() async throws {
    let sequence: [FeedSequenceElement] = [
      .repeating([
        .headline(paired: false),
      ]),
    ]
    let source = FeedItem.Source(id: "1", isDefault: false, category: "Top News", name: "Source 1", destinationDomains: [], localeDetails: [.init(channels: ["Top Sources"], locale: "en_US")])
    let source2 = FeedItem.Source(id: "2", isDefault: false, category: "Top News", name: "Source 2", destinationDomains: [], localeDetails: [.init(channels: ["Top Sources"], locale: "en_US")])
    let items = [
      FeedItem(score: 10, content: .testArticle(sourceID: source.id, contentID: "a", score: 10), source: source),
      FeedItem(score: 4, content: .testArticle(sourceID: source.id, contentID: "b", score: 4), source: source),
      FeedItem(score: 2, content: .testArticle(sourceID: source.id, contentID: "c", score: 2), source: source),
      FeedItem(score: 44, content: .testArticle(sourceID: source.id, contentID: "d", score: 44), source: source),
      FeedItem(score: 1, content: .testArticle(sourceID: source2.id, contentID: "e", score: 1), source: source2),
    ]
    let expectedItems = items.sorted(by: { $0.score < $1.score })
    let generator = FeedCardGenerator(
      scoredItems: items,
      sequence: sequence,
      followedSources: ["1"], // Following source 1 directly
      hiddenSources: [],
      followedChannels: ["en_US": ["Top Sources"]], // Following source 2 indirectly
      ads: nil
    )
    let cards = try await generator.allCards
    XCTAssertEqual(cards.count, items.count)
    XCTAssertEqual(cards, expectedItems.map({ .headline($0) }))
  }
}

extension FeedItem.Content {
  fileprivate static func testArticle(sourceID: String, contentID: String, score: Double, containsImage: Bool = true) -> Self {
    .init(
      publishTime: Date(),
      imageURL: containsImage ? URL(string: "image") : nil,
      title: "",
      description: "",
      contentType: .article,
      publisherID: sourceID,
      urlHash: contentID,
      baseScore: score
    )
  }
}
