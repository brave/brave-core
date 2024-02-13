// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
import TestHelpers
@testable import Data

class RecentlyClosedTests: CoreDataTestCase {

  func testInsert() {
    let url1 = URL(string: "https://brave.com")!
    let title1 = "Brave"

    let object1 = createAndWait(url: url1, title: title1)
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)

    XCTAssertEqual(object1.url, url1.absoluteString)
    XCTAssertEqual(object1.title, title1)
    
    let url2 = URL(string: "https://www.wikipedia.org")!
    let title2 = "Wikipedia"
    
    let object2 = createAndWait(
      url: url2, title: title2)
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 2)

    XCTAssertEqual(object2.url, url2.absoluteString)
    XCTAssertEqual(object2.title, title2)
  }
  
  func testInsertAll() {
    let url1 = URL(string: "https://brave.com")!
    let title1 = "Brave"
    
    let savedObject1 = SavedRecentlyClosed(
      url: url1,
      title: title1,
      dateAdded: Date(),
      interactionState: Data(),
      order: 0)
    
    let url2 = URL(string: "https://www.wikipedia.org")!
    let title2 = "Wikipedia"
    
    let savedObject2 = SavedRecentlyClosed(
      url: url2,
      title: title2,
      dateAdded: Date(),
      interactionState: Data(),
      order: 0)
    
    let savedRecentlyClosedList = [savedObject1, savedObject2]
    
    backgroundSaveAndWaitForExpectation {
      RecentlyClosed.insertAll(savedRecentlyClosedList)
    }
    
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 2)
  }

  func testGetExisting() {
    let unsavedUrl = URL(string: "https://wrong.example.com")!
    let savedUrl = URL(string: "https://brave.com")!

    let title1 = "Brave"
    createAndWait(url: savedUrl, title: title1)

    XCTAssertNil(RecentlyClosed.get(with: unsavedUrl.absoluteString))
    XCTAssertNotNil(RecentlyClosed.get(with: savedUrl.absoluteString))
  }
  
  func testRemove() {
    let url = URL(string: "https://brave.com")!
    let title = "Brave"

    createAndWait(url: url, title: title)

    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)

    backgroundSaveAndWaitForExpectation {
      RecentlyClosed.remove(with: url.absoluteString)
    }

    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 0)
  }
  
  func testRemoveAll() {
    let url1 = URL(string: "https://brave.com")!
    let title1 = "Brave"

    createAndWait(url: url1, title: title1)
    
    let url2 = URL(string: "https://www.wikipedia.org")!
    let title2 = "Wikipedia"
    
    createAndWait(url: url2, title: title2)
    
    backgroundSaveAndWaitForExpectation {
      RecentlyClosed.removeAll()
    }
    
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 0)
  }
  
  func testRemoveAllOlderThan() {
    let url1 = URL(string: "https://brave.com")!
    let title1 = "Brave"

    createAndWait(url: url1, title: title1, date: Date().advanced(by: -10.days))
    
    let url2 = URL(string: "https://www.wikipedia.org")!
    let title2 = "Wikipedia"
    
    createAndWait(url: url2, title: title2, date: Date().advanced(by: -15.days))
    
    let url3 = URL(string: "https://www.blizzard.com")!
    let title3 = "Blizzard"
    
    createAndWait(url: url3, title: title3, date: Date().advanced(by: -2.days))
    
    backgroundSaveAndWaitForExpectation {
      RecentlyClosed.deleteAll(olderThan: 3.days)
    }
    
    XCTAssertEqual(try! DataController.viewContext.count(for: fetchRequest), 1)
  }
  
  @discardableResult
  private func createAndWait(url: URL, title: String, date: Date = Date(), historyIndex: Int = 0) -> RecentlyClosed {
    backgroundSaveAndWaitForExpectation {
      RecentlyClosed.insert(SavedRecentlyClosed(
        url: url,
        title: title,
        interactionState: Data(),
        order: Int32(historyIndex)))
    }

    let predicate = NSPredicate(format: "\(#keyPath(RecentlyClosed.url)) == %@", url.absoluteString)
    return RecentlyClosed.first(where: predicate)!
  }
  
  private let fetchRequest = NSFetchRequest<RecentlyClosed>(entityName: String(describing: RecentlyClosed.self))
}

