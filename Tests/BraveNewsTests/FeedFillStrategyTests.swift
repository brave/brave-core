// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import XCTest
@testable import BraveNews

// Tests things that all fill strategies should do the same
class CommonFillStrategyTests: XCTestCase {
  let strategies: [FillStrategy] = [
    DefaultFillStrategy(),
    FilteredFillStrategy(isIncluded: { _ in true }),
    CategoryFillStrategy(
      categories: Set<String>(arrayLiteral: FeedDataSource.topNewsCategory),
      category: \.source.category
    ),
  ]

  func testFillFromEmptyList() {
    var list: [FeedItem] = []
    for strategy in strategies {
      XCTAssertNil(strategy.next(3, from: &list))
    }
  }

  func testInsufficientListCount() {
    var list: [FeedItem] = [
      .init(score: 0, content: .mockArticle, source: .mock),
      .init(score: 0, content: .mockArticle, source: .mock),
    ]
    // We shouldn't fill any items if the length we want is more than whats available
    for strategy in strategies {
      XCTAssertNil(strategy.next(3, from: &list))
    }
    // Shouldn't of deleted any items if we couldn't obtain the total number
    XCTAssertEqual(list.count, 2)
  }
}

class DefaultFillStrategyTests: XCTestCase {
  let strategy = DefaultFillStrategy()

  func testFillSingleItem() {
    let item = FeedItem(score: 0, content: .mockArticle, source: .mock)
    var list: [FeedItem] = [
      item,
      .init(score: 0, content: .mockArticle, source: .mock),
      .init(score: 0, content: .mockArticle, source: .mock),
    ]
    let listCopy = list
    XCTAssertEqual(strategy.next(from: &list), item)
    XCTAssertEqual(list.count, listCopy.count - 1)
  }

  func testFillSuccessfully() {
    var list: [FeedItem] = [
      .init(score: 0, content: .mockArticle, source: .mock),
      .init(score: 0, content: .mockArticle, source: .mock),
      .init(score: 0, content: .mockArticle, source: .mock),
    ]
    let listCopy = list
    XCTAssertEqual(strategy.next(3, from: &list), listCopy)
    XCTAssert(list.isEmpty)
  }

  func testFillWithPredicate() {
    let adjustedScoresList: [FeedItem] = (0..<3).map { _ in
      var article: FeedItem.Content = .mockArticle
      article.baseScore = 10
      return .init(score: 0, content: article, source: .mock)
    }
    var list: [FeedItem] =
      (0..<3).map { _ in .init(score: 0, content: .mockArticle, source: .mock) } + adjustedScoresList
    let listCopy = list
    let items = strategy.next(3, from: &list, where: { ($0.content.baseScore ?? 0) >= 10 })
    XCTAssertEqual(items, adjustedScoresList)
    XCTAssertEqual(list.count, listCopy.count - adjustedScoresList.count)
  }
}

class FilteredFillStrategyTests: XCTestCase {
  let strategy = FilteredFillStrategy(isIncluded: { $0.content.title == "Special Title" })

  func testNoItemsFound() {
    var list: [FeedItem] = [
      .init(score: 0, content: .mockArticle, source: .mock),
      .init(score: 0, content: .mockArticle, source: .mock),
      .init(score: 0, content: .mockArticle, source: .mock),
    ]
    // No items have title == Special Title, so should return nil
    XCTAssertNil(strategy.next(3, from: &list))
    // No items should be removed from items
    XCTAssertEqual(list.count, 3)
  }

  func testFillSuccessfully() {
    var list: [FeedItem] = (0..<3).map { _ in
      var article: FeedItem.Content = .mockArticle
      article.title = "Special Title"
      return .init(score: 0, content: article, source: .mock)
    }
    let listCopy = list
    XCTAssertEqual(strategy.next(3, from: &list), listCopy)
    XCTAssert(list.isEmpty)
  }

  func testFillWithAdditionalPredicate() {
    let adjustedScoresList: [FeedItem] = (0..<3).map { _ in
      var article: FeedItem.Content = .mockArticle
      article.title = "Special Title"
      article.baseScore = 10
      return .init(score: 0, content: article, source: .mock)
    }
    var list: [FeedItem] =
      (0..<3).map { _ in .init(score: 0, content: .mockArticle, source: .mock) } + adjustedScoresList
    let listCopy = list
    let items = strategy.next(3, from: &list, where: { ($0.content.baseScore ?? 0) >= 10 })
    XCTAssertEqual(items, adjustedScoresList)
    XCTAssertEqual(list.count, listCopy.count - adjustedScoresList.count)
  }
}

class CategoryFillStrategyTests: XCTestCase {
  let strategy = CategoryFillStrategy(
    categories: Set(mockCategories),
    category: \.source.category,
    initialCategory: FeedDataSource.topNewsCategory
  )

  func testCategoryFill() {
    var items: [FeedItem] = (0..<mockCategories.count * 3).map { i in
      var source: FeedItem.Source = .mock
      source.category = mockCategories[i % mockCategories.count]
      return .init(score: 0, content: .mockArticle, source: source)
    }
    // First item filled should be the same as initial category
    var item = strategy.next(from: &items)
    XCTAssertEqual(item?.source.category, strategy.initialCategory)
    // After that, each item should NOT be the previous category. Its random so we can't test specifics
    repeat {
      let lastCategory = item?.source.category
      item = strategy.next(from: &items)
      XCTAssertNotEqual(lastCategory, item?.source.category)
    } while item != nil
    XCTAssert(items.isEmpty)
  }
}

private let mockCategories = [
  FeedDataSource.topNewsCategory,
  "Food",
  "Crypto",
  "Fashion",
  "Science",
  "Travel",
  "Entertainment",
  "Home",
  "Tech",
]

extension FeedItem.LegacySource {
  fileprivate static var mock: FeedItem.LegacySource {
    .init(
      id: UUID().uuidString,
      isDefault: true,
      category: "Top News",
      name: "Test Source"
    )
  }
}

extension FeedItem.Source {
  fileprivate static var mock: FeedItem.Source {
    .init(
      id: UUID().uuidString,
      isDefault: true,
      category: "Top News",
      name: "Test Source",
      destinationDomains: []
    )
  }
}

extension FeedItem.Content {
  fileprivate static var mockArticle: FeedItem.Content {
    .init(
      publishTime: Date(timeIntervalSinceNow: -60),
      url: URL(string: "https://brave.com/"),
      imageURL: nil,
      title: "Test Title",
      description: "Test Description",
      contentType: .article,
      publisherID: UUID().uuidString,
      urlHash: UUID().uuidString,
      baseScore: 0,
      offersCategory: nil
    )
  }
  fileprivate static var mockOffer: FeedItem.Content {
    .init(
      publishTime: Date(timeIntervalSinceNow: -60),
      url: URL(string: "https://brave.com/"),
      imageURL: nil,
      title: "Test Title",
      description: "Test Description",
      contentType: .deals,
      publisherID: UUID().uuidString,
      urlHash: UUID().uuidString,
      baseScore: 0,
      offersCategory: "Discounts"
    )
  }
}
