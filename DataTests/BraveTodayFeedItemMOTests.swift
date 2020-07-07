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
    
    // TODO: FIX
//    func testSimpleInsert() {
//        XCTAssertEqual(BraveTodayFeedItemMO.all()!.count, 0)
//        createAndWait()
//        XCTAssertEqual(BraveTodayFeedItemMO.all()!.count, 1)
//    }
    
    func testInsertWithoutPublisher() {
        XCTAssertEqual(BraveTodayFeedItemMO.all()!.count, 0)
        
        backgroundSaveAndWaitForExpectation(inverted: true) {
            BraveTodayFeedItemMO
                .insertInternal(category: "test", publishTime: dateFrom(string: "2020-07-01 23:59:59"),
                                url: nil, imageURL: nil, title: "title",
                                itemDescription: "itemDescription", contentType: "contentType",
                                publisherID: "publisherID", publisherName: "publisherName",
                                publisherLogo: nil, urlHash: UUID().uuidString)
            
        }
        
        XCTAssertEqual(BraveTodayFeedItemMO.all()!.count, 0)
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
                               publisherName: String = "Brave",
                               publisherLogo: String? = nil,
                               urlHash: String = UUID().uuidString) -> BraveTodayFeedItemMO {
        
        let publishTime = dateFrom(string: publishTimeString)
        
        backgroundSaveAndWaitForExpectation {
            BraveTodayFeedItemMO
                .insertInternal(category: category, publishTime: publishTime, url: url,
                                imageURL: imageURL, title: title, itemDescription: itemDescription,
                                contentType: contentType, publisherID: publisherID,
                                publisherName: publisherName, publisherLogo: publisherLogo,
                                urlHash: urlHash)
            
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
