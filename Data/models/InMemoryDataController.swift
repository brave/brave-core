// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import CoreData
import Shared
import XCGLogger

public class InMemoryDataController: DataController {
    
    override init() {
        super.init()
        
        let description = NSPersistentStoreDescription()
        description.type = NSInMemoryStoreType
        
        container.persistentStoreDescriptions = [description]
        
        // Calling `initialize` in constructor.
        // Initialize code in constructor can't happen in persistent database
        // because we have to check for migration code first, see #3425
        initialize()
    }
}
