// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData

public final class RSSFeedSource: NSManagedObject, CRUD {
  @NSManaged public var title: String?
  @NSManaged public var feedUrl: String

  public class func get(with feedUrl: String) -> RSSFeedSource? {
    return getInternal(feedUrl: feedUrl, context: DataController.viewContext)
  }

  public class func all() -> [RSSFeedSource] {
    all() ?? []
  }

  public class func delete(with feedUrl: String) {
    let context = DataController.viewContext
    if let item = get(with: feedUrl) {
      item.delete(context: .existing(context))
      if context.hasChanges {
        try? context.save()
      }
    }
  }

  public class func insert(title: String?, feedUrl: String) {
    let context = DataController.viewContext
    let source = RSSFeedSource(entity: entity(in: context), insertInto: context)
    source.title = title
    source.feedUrl = feedUrl
    if context.hasChanges {
      try? context.save()
    }
  }
  
  public class func update(feedUrl: String, title: String) {
    DataController.perform { context in
      if let source = getInternal(feedUrl: feedUrl, context: context) {
        source.title = title
      }
    }
  }
  
  private class func getInternal(feedUrl: String, context: NSManagedObjectContext) -> RSSFeedSource? {
    let predicate = NSPredicate(format: "\(#keyPath(RSSFeedSource.feedUrl)) == %@", feedUrl)
    return first(where: predicate, context: context)
  }

  private class func entity(in context: NSManagedObjectContext) -> NSEntityDescription {
    NSEntityDescription.entity(forEntityName: "RSSFeedSource", in: context)!
  }
}
