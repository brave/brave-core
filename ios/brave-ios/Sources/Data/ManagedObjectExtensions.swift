// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreData
import Foundation

extension NSManagedObject {
  /// Returns true if the object still exists in the persistent store.
  public func existsInPersistentStore() -> Bool {
    // `isDeleted` is set to true if the object is going to be deleted
    // on the next context save.
    // After the save happens, managedObjectContext is set to nil.
    // This conditional should catch both deletion states.
    return !isDeleted && managedObjectContext != nil
  }
}
