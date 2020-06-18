// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CoreData
import Shared
@testable import Data

class BraveTodayFeedItemMOTests: CoreDataTestCase {
    
    let fetchRequest = NSFetchRequest<History>(entityName: String(describing: BraveTodayFeedItemMO.self))
    
    private func entity(for context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: String(describing: BraveTodayFeedItemMO.self), in: context)!
    }

    

}
