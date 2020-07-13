// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
import Shared
@testable import Data

class BraveTodayFeedItemMOTests: CoreDataTestCase {
    
    let fetchRequest = NSFetchRequest<BraveTodayFeedItemMO>(entityName: String(describing: BraveTodayFeedItemMO.self))
    
    private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: String(describing: BraveTodayFeedItemMO.self), in: context)!
    }
    
    func testInsert() {
        XCTAssertEqual(BraveTodayFeedItemMO.all()!.count, 0)
        
        let publisherId = "brave_pub"
        
        backgroundSaveAndWaitForExpectation {
            BraveTodaySourceMO.insert(publisherID: publisherId, publisherLogo: nil, publisherName: "Brave")
        }
        
        let bridge =
            BraveTodayFeedItemBridge(category: "cat", publishTime: Date(), url: "https://example.com",
                                     imageURL: "https://example.com/image", title: "Test item",
                                     itemDescription: "Test item description", contentType: "Article",
                                     publisherID: publisherId,
                                     publisherLogo: nil, urlHash: "random_hash")
        backgroundSaveAndWaitForExpectation {
            BraveTodayFeedItemMO.insert(item: bridge)
        }
        XCTAssertEqual(BraveTodayFeedItemMO.all()!.count, 1)
        
        guard let item = BraveTodayFeedItemMO.first() else {
            XCTFail("Failed to get BraveTodayFeedItemMO")
            return
        }
        
        // Properties save check
        XCTAssertEqual(item.category, bridge.category)
        XCTAssertEqual(item.publishTime, bridge.publishTime)
        XCTAssertEqual(item.url, bridge.url)
        XCTAssertEqual(item.imageURL, bridge.imageURL)
        XCTAssertEqual(item.title, bridge.title)
        XCTAssertEqual(item.itemDescription, bridge.itemDescription)
        XCTAssertEqual(item.contentType, bridge.contentType)
        XCTAssertEqual(item.publisherID, bridge.publisherID)
        XCTAssertEqual(item.publisherLogo, bridge.publisherLogo)
        XCTAssertEqual(item.urlHash, bridge.urlHash)
        
        XCTAssertEqual(item.viewed, false)
        XCTAssert(item.created > bridge.publishTime)
    }
    
    func testInsertFromList() {
        let count = 10000
        var bridgeItems = [BraveTodayFeedItemBridge]()
        
        let publisherId = "brave_pub"
        
        backgroundSaveAndWaitForExpectation {
            BraveTodaySourceMO.insert(publisherID: "\(publisherId)_1", publisherLogo: nil, publisherName: "Brave1")
        }
        
        backgroundSaveAndWaitForExpectation {
            BraveTodaySourceMO.insert(publisherID: "\(publisherId)_2", publisherLogo: nil, publisherName: "Brave2")
        }
        
        backgroundSaveAndWaitForExpectation {
            BraveTodaySourceMO.insert(publisherID: "\(publisherId)_3", publisherLogo: nil, publisherName: "Brave3")
        }
        
        for i in 1...count {
            let publisherId = "\(publisherId)_\(Int.random(in: 1...3))"
            
            let bridge =
                BraveTodayFeedItemBridge(category: "Category", publishTime: Date(),
                                         url: "https://example.com/\(i)",
                                         imageURL: "https://example.com/image", title: "Test item",
                                         itemDescription: "Test item description", contentType: "Article",
                                         publisherID: publisherId, publisherLogo: nil,
                                         urlHash: "random_hash_\(i)")
            
            bridgeItems.append(bridge)
        }
        
        let startTime = CFAbsoluteTimeGetCurrent()
        backgroundSaveAndWaitForExpectation {
            BraveTodayFeedItemMO.insert(from: bridgeItems)
        }
        
        let endTime = CFAbsoluteTimeGetCurrent() - startTime
        print("Adding \(count) feed items took :\(String(format:"%.2f", endTime)) seconds")
        
        XCTAssertEqual(BraveTodayFeedItemMO.all()!.count, count)
    }
    
    func testInsertWithoutPublisher() {
        XCTAssertEqual(BraveTodayFeedItemMO.all()!.count, 0)
        
        let fakeId = BraveTodaySourceMO(context: DataController.viewContext).objectID
       
        // `insertInternal()` test
        backgroundSaveAndWaitForExpectation(inverted: true) {
            BraveTodayFeedItemMO
                .insertInternal(category: "test", publishTime: dateFrom(string: "2020-07-01 23:59:59"),
                                url: nil, imageURL: nil, title: "title",
                                itemDescription: "itemDescription", contentType: "contentType",
                                publisherID: "publisherID", publisherLogo: nil, urlHash: UUID().uuidString,
                                sourceObjectId: fakeId)
            
        }
        
        XCTAssertEqual(BraveTodayFeedItemMO.all()!.count, 0)
        
        // public insert test
        let bridge =
            BraveTodayFeedItemBridge(category: "Category", publishTime: Date(),
                                     url: "https://example.com/",
                                     imageURL: "https://example.com/image", title: "Test item",
                                     itemDescription: "Test item description", contentType: "Article",
                                     publisherID: "NULL", publisherLogo: nil, urlHash: "random_hash")
        
        backgroundSaveAndWaitForExpectation(inverted: true) {
            BraveTodayFeedItemMO.insert(item: bridge)
        }
        
        XCTAssertEqual(BraveTodayFeedItemMO.all()!.count, 0)
        
        // public insert from list test
        backgroundSaveAndWaitForExpectation(inverted: true) {
            BraveTodayFeedItemMO.insert(from: [bridge])
        }
        
        XCTAssertEqual(BraveTodayFeedItemMO.all()!.count, 0)
    }
    
    func testInsertFromListOneEmptyPublisher() {
        backgroundSaveAndWaitForExpectation {
            BraveTodaySourceMO.insert(publisherID: "brave_pub", publisherLogo: nil, publisherName: "Brave")
        }
        
        let pubExists =
            BraveTodayFeedItemBridge(category: "Category", publishTime: Date(),
                                     url: "https://example.com/",
                                     imageURL: "https://example.com/image", title: "Test item",
                                     itemDescription: "Test item description", contentType: "Article",
                                     publisherID: "brave_pub",
                                     publisherLogo: nil, urlHash: "random_hash1")
        
        let pubDoesntExists =
            BraveTodayFeedItemBridge(category: "Category", publishTime: Date(),
                                     url: "https://example.com/",
                                     imageURL: "https://example.com/image", title: "Test item",
                                     itemDescription: "Test item description", contentType: "Article",
                                     publisherID: "FAKE_PUB",
                                     publisherLogo: nil, urlHash: "random_hash2")
        
        backgroundSaveAndWaitForExpectation() {
            BraveTodayFeedItemMO.insert(from: [pubExists, pubDoesntExists])
        }
        
        XCTAssertEqual(BraveTodayFeedItemMO.all()!.count, 1)
    }

    // MARK: - Helpers
    
    @discardableResult
    private func createAndWait(category: String = "test",
                               publishTimeString: String = "2020-07-01 23:59:59",
                               url: String? = nil,
                               imageURL: String? = nil,
                               title: String = "Brave title",
                               itemDescription: String = "Description",
                               contentType: String = "article",
                               publisherID: String = "BravePub",
                               publisherLogo: String? = nil,
                               urlHash: String = UUID().uuidString,
                               createSource: Bool = false) -> BraveTodayFeedItemMO {
        
        if createSource {
            backgroundSaveAndWaitForExpectation {
                BraveTodaySourceMO.insert(publisherID: publisherID, publisherLogo: nil,
                                          publisherName: "Brave\(publisherID)")
            }
        }
        
        let sourcePublisherIdKeyPath = #keyPath(BraveTodaySourceMO.publisherID)
        let sourcePredicate = NSPredicate(format: "\(sourcePublisherIdKeyPath) == %@", publisherID)
        
        guard let sourceObjectId = BraveTodaySourceMO.first(where: sourcePredicate)?.objectID else {
            XCTFail("Failed to get BraveTodaySourceMO")
            fatalError()
        }
        
        let publishTime = dateFrom(string: publishTimeString)
        
        backgroundSaveAndWaitForExpectation {
            BraveTodayFeedItemMO
                .insertInternal(category: category, publishTime: publishTime, url: url,
                                imageURL: imageURL, title: title, itemDescription: itemDescription,
                                contentType: contentType, publisherID: publisherID, publisherLogo: publisherLogo,
                                urlHash: urlHash, sourceObjectId: sourceObjectId)
            
        }
        
        let sort = NSSortDescriptor(key: "created", ascending: false)
        
        return BraveTodayFeedItemMO.first(sortDescriptors: [sort])!
    }
    
    private func dateFrom(string: String, format: String? = nil) -> Date {
        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = format ?? "yyyy-MM-dd HH:mm:ss"
        dateFormatter.timeZone = TimeZone(abbreviation: "GMT")!
        
        return dateFormatter.date(from: string)!
    }
}
