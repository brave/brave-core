// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import XCTest
@testable import Data
import TestHelpers

class FavoriteTests: CoreDataTestCase {

  private let fetchRequest: NSFetchRequest<Favorite> = {
    let fetchRequest = NSFetchRequest<Favorite>(entityName: String(describing: "Bookmark"))
    // We always want favorites folder to be on top, in the first section.
    fetchRequest.sortDescriptors = [
      NSSortDescriptor(key: "order", ascending: true),
      NSSortDescriptor(key: "created", ascending: false),
    ]
    fetchRequest.predicate = NSPredicate(format: "isFavorite == true")
    return fetchRequest
  }()

  private lazy var fetchController = Favorite.frc()

  private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
    return NSEntityDescription.entity(forEntityName: String(describing: "Bookmark"), in: context)!
  }

  // MARK: - Adding

  func testAddFavorite() {
    let url = "http://brave.com"
    let title = "Brave"

    let favoritedBookmark = createAndWait(url: URL(string: url), title: title)
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)

    XCTAssertEqual(favoritedBookmark.url, url)
    XCTAssertEqual(favoritedBookmark.title, title)
  }

  // MARK: - Editing

  func testEditFavoriteURL() {
    let url = "http://brave.com"
    let newUrl = "http://updated.example.com"
    let newTitle = "newtitle"

    let object = createAndWait(url: URL(string: url), title: "title")

    XCTAssertEqual(object.displayTitle, "title")
    XCTAssertEqual(object.url, url)

    backgroundSaveAndWaitForExpectation {
      object.update(customTitle: newTitle, url: newUrl)
    }

    DataController.viewContext.refreshAllObjects()
    // Make sure only one record was added to DB
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)

    XCTAssertNotEqual(object.url, url)
    XCTAssertEqual(object.url, newUrl)
  }

  func testEditFavoriteName() {
    let url = "http://brave.com"
    let customTitle = "Brave"
    let newTitle = "Updated Title"

    let object = createAndWait(url: URL(string: url), title: "title", customTitle: customTitle)

    XCTAssertEqual(object.displayTitle, customTitle)
    XCTAssertEqual(object.url, url)

    backgroundSaveAndWaitForExpectation {
      object.update(customTitle: newTitle, url: nil)
    }

    DataController.viewContext.refreshAllObjects()
    // Make sure only one record was added to DB
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)

    XCTAssertNotEqual(object.displayTitle, customTitle)
    XCTAssertEqual(object.displayTitle, newTitle)
  }

  // MARK: - Deleting

  func testDeleteFavorite() {
    let bookmarks = makeFavorites(5)

    DataController.viewContext.delete(bookmarks.first!)
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), bookmarks.count - 1)
  }

  func testDeleteAllFavorites() {
    makeFavorites(5)

    let favsPredicate = NSPredicate(format: "isFavorite == true")

    backgroundSaveAndWaitForExpectation {
      Favorite.deleteAll(predicate: favsPredicate)
    }

    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 0)
  }

  // MARK: - Reordering

  private func reorder(_ index: Int, toIndex: Int) {
    backgroundSaveAndWaitForExpectation {
      Favorite.reorder(
        sourceIndexPath: IndexPath(row: index, section: 0),
        destinationIndexPath: IndexPath(row: toIndex, section: 0)
      )
    }
  }

  func testCorrectOrder() {
    makeFavorites(5)

    try! fetchController.performFetch()
    XCTAssertNotNil(fetchController.fetchedObjects)

    // Upon re-order, each now has a given order
    reorder(0, toIndex: 2)
    fetchController.managedObjectContext.refreshAllObjects()
    // Check to see if an order (2) has been given to fetchedObjects index 0
    XCTAssertEqual(fetchController.fetchedObjects![0].order, 2)

    // Verify that order is now taken into account by refetching
    try! fetchController.performFetch()
    XCTAssertNotNil(fetchController.fetchedObjects)
  }

  func testReorderFavorites() {
    makeFavorites(5)

    try! fetchController.performFetch()
    XCTAssertNotNil(fetchController.fetchedObjects)

    let first = fetchController.fetchedObjects![0]
    let second = fetchController.fetchedObjects![1]

    reorder(0, toIndex: 2)
    fetchController.managedObjectContext.refreshAllObjects()
    try! fetchController.performFetch()
    // Check to see if an order (2) has been given to fetchedObjects index 0
    XCTAssertEqual(first.order, 2)
    // The second favorite should have been pushed backwards to 0 since we moved the original 0 to 2
    XCTAssertEqual(second.order, 0)

    // Order is now taken into account by refetching

    XCTAssertNotNil(fetchController.fetchedObjects)

    // Verify that the object is at index 2 in list of fetched objects
    XCTAssertEqual(fetchController.fetchedObjects![2], first)
    // Verify that the first object at index 0 is the original pushed back one
    XCTAssertEqual(fetchController.fetchedObjects![0], second)
    // Verify that the object at index 1 was pushed back
    XCTAssertEqual(fetchController.fetchedObjects![1].order, 1)
  }

  func testMoveToStart() {
    makeFavorites(5)

    try! fetchController.performFetch()
    XCTAssertNotNil(fetchController.fetchedObjects)

    let object = fetchController.fetchedObjects![3]
    let startIndex = fetchController.fetchedObjects!.startIndex

    reorder(3, toIndex: startIndex)
    XCTAssertEqual(object.order, 0)

    // Order is now taken into account by refetching
    try! fetchController.performFetch()
    XCTAssertNotNil(fetchController.fetchedObjects)

    XCTAssertEqual(fetchController.fetchedObjects?.first, object)
  }

  func testMoveToEnd() {
    makeFavorites(5)

    try! fetchController.performFetch()
    XCTAssertNotNil(fetchController.fetchedObjects)

    let first = fetchController.fetchedObjects!.first!
    let endIndex = fetchController.fetchedObjects!.endIndex - 1

    reorder(0, toIndex: endIndex)
    fetchController.managedObjectContext.refreshAllObjects()
    XCTAssertEqual(first.order, 4)

    // Order is now taken into account by refetching
    try! fetchController.performFetch()
    XCTAssertNotNil(fetchController.fetchedObjects)

    XCTAssertEqual(fetchController.fetchedObjects?.last, first)
  }

  // MARK: - Utility

  @discardableResult
  private func makeFavorites(_ count: Int) -> [Favorite] {
    let bookmarks = (0..<count).map { createAndWait(url: URL(string: "http://brave.com/\($0)"), title: "brave") }
    XCTAssertEqual(bookmarks.count, count)
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), bookmarks.count)
    return bookmarks
  }

  @discardableResult
  private func createAndWait(url: URL?, title: String, customTitle: String? = nil) -> Favorite {
    backgroundSaveAndWaitForExpectation {
      Favorite.addInternal(url: url, title: title, customTitle: customTitle, isFavorite: true)
    }
    let bookmark = try! DataController.viewContext.fetch(fetchRequest).first!
    XCTAssertTrue(bookmark.isFavorite)
    return bookmark
  }
}
