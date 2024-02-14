// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import CoreData
import os.log

public final class SessionTabGroup: NSManagedObject, CRUD {
  @NSManaged public var title: String
  @NSManaged public var index: Int32
  @NSManaged private(set) public var groupId: UUID
  
  @NSManaged private(set) public var sessionWindow: SessionWindow?
  @NSManaged private(set) public var sessionTabs: Set<SessionTab>?
  
  @available(*, unavailable)
  public init() {
    fatalError("No Such Initializer: init()")
  }

  @available(*, unavailable)
  public init(context: NSManagedObjectContext) {
    fatalError("No Such Initializer: init(context:)")
  }
  
  public init(context: NSManagedObjectContext,
              sessionWindow: SessionWindow,
              index: Int32,
              title: String) {
    guard let entity = NSEntityDescription.entity(forEntityName: "SessionTabGroup", in: context) else {
      fatalError("No such Entity: SessionTabGroup")
    }
    
    super.init(entity: entity, insertInto: context)
    
    self.sessionWindow = sessionWindow
    self.sessionTabs = Set()
    self.index = index
    self.groupId = groupId
    self.title = title
  }
}
