// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared
import os.log

public final class DataSaved: NSManagedObject, CRUD {
  @NSManaged public var savedUrl: String
  @NSManaged public var amount: String

  public class func get(with savedUrl: String) -> DataSaved? {
    return getInternal(with: savedUrl)
  }

  public class func all() -> [DataSaved] {
    all() ?? []
  }

  public class func delete(with savedUrl: String) {
    DataController.perform { context in
      if let item = getInternal(with: savedUrl, context: context) {
        item.delete(context: .existing(context))
      }
    }
  }

  public class func insert(savedUrl: String, amount: String) {
    DataController.perform { context in
      guard let entity = entity(in: context) else {
        Logger.module.error("Error fetching the entity 'DataSaved' from Managed Object-Model")

        return
      }

      let source = DataSaved(entity: entity, insertInto: context)
      source.savedUrl = savedUrl
      source.amount = amount
    }
  }

  private class func entity(in context: NSManagedObjectContext) -> NSEntityDescription? {
    NSEntityDescription.entity(forEntityName: "DataSaved", in: context)
  }

  private class func getInternal(
    with savedUrl: String,
    context: NSManagedObjectContext = DataController.viewContext
  ) -> DataSaved? {
    let predicate = NSPredicate(format: "\(#keyPath(DataSaved.savedUrl)) == %@", savedUrl)

    return first(where: predicate, context: context)
  }
}
