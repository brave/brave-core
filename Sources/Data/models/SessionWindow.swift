// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import CoreData
import os.log

public final class SessionWindow: NSManagedObject, CRUD {
  @NSManaged public var index: Int32
  @NSManaged private(set) public var isPrivate: Bool
  @NSManaged public var isSelected: Bool
  @NSManaged private(set) public var windowId: UUID
  
  @NSManaged private(set) public var sessionTabs: Set<SessionTab>?
  @NSManaged private(set) public var sessionTabGroups: Set<SessionTabGroup>?
  
  @available(*, unavailable)
  public init() {
    fatalError("No Such Initializer: init()")
  }

  @available(*, unavailable)
  public init(context: NSManagedObjectContext) {
    fatalError("No Such Initializer: init(context:)")
  }
  
  @objc
  private override init(entity: NSEntityDescription, insertInto context: NSManagedObjectContext?) {
    super.init(entity: entity, insertInto: context)
  }
  
  public init(context: NSManagedObjectContext,
              index: Int32,
              isPrivate: Bool,
              isSelected: Bool) {
    guard let entity = NSEntityDescription.entity(forEntityName: "SessionWindow", in: context) else {
      fatalError("No such Entity: SessionWindow")
    }
    
    super.init(entity: entity, insertInto: context)
    self.sessionTabs = Set()
    self.sessionTabGroups = Set()
    self.index = index
    self.isPrivate = isPrivate
    self.isSelected = isSelected
    self.windowId = UUID()
  }
}

// MARK: - Public

extension SessionWindow {
  /// Returns the active window
  public static func getActiveWindow(context: NSManagedObjectContext) -> SessionWindow? {
    let predicate = NSPredicate(format: "\(#keyPath(SessionWindow.isSelected)) == true")
    return first(where: predicate, context: context)
  }
  
  /// Returns the tab with the specificed windowId
  public static func from(windowId: UUID) -> SessionWindow? {
    return Self.from(windowId: windowId, in: DataController.viewContext)
  }
  
  public static func from(windowId: UUID, in context: NSManagedObjectContext) -> SessionWindow? {
    let predicate = NSPredicate(format: "\(#keyPath(SessionWindow.windowId)) == %@", windowId.uuidString)
    return first(where: predicate, context: context)
  }
  
  public static func createIfNeeded(index: Int32, isPrivate: Bool, isSelected: Bool) {
    DataController.performOnMainContext { context in
      if SessionWindow.getActiveWindow(context: context) != nil {
        return
      }
      
      _ = SessionWindow(context: context, index: index, isPrivate: isPrivate, isSelected: isSelected)
      
      do {
        try context.save()
      } catch {
        Logger.module.error("performTask save error: \(error.localizedDescription, privacy: .public)")
      }
    }
  }
  
  public static func createWindow(isPrivate: Bool, isSelected: Bool, uuid: UUID) {
    DataController.performOnMainContext { context in
      if let sessionWindow = SessionWindow.from(windowId: uuid, in: context) {
        Self.all().forEach {
          $0.isSelected = false
        }
        
        sessionWindow.isSelected = isSelected
        return
      }
      
      let count = SessionWindow.count(context: context) ?? 0
      let window = SessionWindow(context: context, index: Int32(count), isPrivate: isPrivate, isSelected: isSelected)
      window.windowId = uuid
      
      do {
        try context.save()
      } catch {
        Logger.module.error("performTask save error: \(error.localizedDescription, privacy: .public)")
      }
    }
  }
  
  /// Marks the specified window as selected
  /// Since only one window can be active at a time, all other windows are marked as deselected
  public static func setSelected(windowId: UUID) {
    DataController.perform { context in
      guard let window = Self.from(windowId: windowId, in: context) else { return }

      let predicate = NSPredicate(format: "isSelected == true")
      all(where: predicate, context: context)?.forEach {
        $0.isSelected = false
      }

      window.isSelected = true
    }
  }
  
  public static func all() -> [SessionWindow] {
    let sortDescriptors = [NSSortDescriptor(key: #keyPath(SessionWindow.index), ascending: true)]
    return all(sortDescriptors: sortDescriptors) ?? []
  }
  
  public static func delete(windowId: UUID) {
    DataController.perform { context in
      guard let sessionWindow = SessionWindow.from(windowId: windowId, in: context) else {
        return
      }
      
      sessionWindow.sessionTabs?.forEach {
        $0.delete(context: .existing(context))
      }
      
      sessionWindow.sessionTabGroups?.forEach {
        $0.delete(context: .existing(context))
      }
      
      sessionWindow.delete(context: .existing(context))
    }
  }
  
  public static func deleteAllWindows(privateOnly: Bool) {
    DataController.perform { context in
      guard let sessionWindows = SessionWindow.all(where: NSPredicate(format: "isPrivate == %@", privateOnly ? "true" : "false"), context: context) else {
        return
      }
      
      sessionWindows.forEach { sessionWindow in
        sessionWindow.sessionTabs?.forEach {
          $0.delete(context: .existing(context))
        }
        
        sessionWindow.sessionTabGroups?.forEach {
          $0.delete(context: .existing(context))
        }
        
        sessionWindow.delete(context: .existing(context))
      }
    }
  }
}

// MARK: - Private

extension SessionWindow {
  private static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription? {
    return NSEntityDescription.entity(forEntityName: "SessionWindow", in: context)
  }
}
