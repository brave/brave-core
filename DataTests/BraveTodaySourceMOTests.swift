// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
import Shared
@testable import Data

class BraveTodaySourceMOTests: CoreDataTestCase {

    let fetchRequest = NSFetchRequest<BraveTodaySourceMO>(entityName: String(describing: BraveTodaySourceMO.self))
    
    private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: String(describing: BraveTodaySourceMO.self), in: context)!
    }
    
    func testSimpleInsert() {
        XCTAssertEqual(BraveTodaySourceMO.all()!.count, 0)
        createAndWait()
        XCTAssertEqual(BraveTodaySourceMO.all()!.count, 1)
    }
    
    @discardableResult
    private func createAndWait(enabled: Bool = true,
                               publisherID: String = "BravePub",
                               publisherLogo: String? = nil,
                               publisherName: String = "Pub Name") -> BraveTodaySourceMO {
        
        backgroundSaveAndWaitForExpectation {
            BraveTodaySourceMO.insertInternal(enabled: enabled, publisherID: publisherID,
                                              publisherLogo: publisherLogo, publisherName: publisherName)
        }
        
        
        
        let predicate = NSPredicate(format: "\(#keyPath(BraveTodaySourceMO.publisherID)) == %@", publisherID)
        
        return BraveTodaySourceMO.first(where: predicate)!
    }
}
