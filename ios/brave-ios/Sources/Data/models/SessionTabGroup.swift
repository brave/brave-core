// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreData
import Foundation
import UIKit
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

  @objc
  private override init(entity: NSEntityDescription, insertInto context: NSManagedObjectContext?) {
    super.init(entity: entity, insertInto: context)
  }

  public init(
    context: NSManagedObjectContext,
    sessionWindow: SessionWindow,
    index: Int32,
    title: String,
    groupId: UUID = UUID()
  ) {
    guard let entity = NSEntityDescription.entity(forEntityName: "SessionTabGroup", in: context)
    else {
      fatalError("No such Entity: SessionTabGroup")
    }

    super.init(entity: entity, insertInto: context)

    self.sessionWindow = sessionWindow
    self.sessionTabs = Set()
    self.index = index
    self.groupId = groupId
    self.title = title
  }

  public static var tabGroupCount: Int {
    count() ?? -1
  }
}

// MARK: - Public

extension SessionTabGroup {
  /// Returns the groups belonging to the given window, in display order
  public static func all(windowId: UUID) -> [SessionTabGroup] {
    let sortDescriptors = [NSSortDescriptor(key: #keyPath(SessionTabGroup.index), ascending: true)]
    return all(where: predicate(windowId: windowId), sortDescriptors: sortDescriptors) ?? []
  }

  /// Creates a group in the given window containing the given tabs. Tabs that are already in
  /// another group are moved, and any group left empty by that is deleted.
  public static func create(groupId: UUID, windowId: UUID, title: String, tabIds: [UUID]) {
    DataController.perform { context in
      guard let window = SessionWindow.from(windowId: windowId, in: context) else { return }
      let tabs = groupableTabs(tabIds, windowId: windowId, in: context)
      guard !tabs.isEmpty else { return }

      let lastIndex = all(where: predicate(windowId: windowId), context: context)?
        .map(\.index).max()
      let group = SessionTabGroup(
        context: context,
        sessionWindow: window,
        index: lastIndex.map { $0 + 1 } ?? 0,
        title: title,
        groupId: groupId
      )
      move(tabs, to: group, in: context)
    }
  }

  /// Moves the given tabs into an existing group. Any group left empty by the move is deleted.
  public static func add(tabIds: [UUID], toGroup groupId: UUID) {
    DataController.perform { context in
      guard let group = from(groupId: groupId, in: context),
        let windowId = group.sessionWindow?.windowId
      else { return }
      move(groupableTabs(tabIds, windowId: windowId, in: context), to: group, in: context)
    }
  }

  /// Removes the group while keeping its tabs
  public static func delete(groupId: UUID) {
    DataController.perform { context in
      guard let group = from(groupId: groupId, in: context) else { return }
      group.sessionTabs?.forEach { $0.sessionTabGroup = nil }
      context.delete(group)
    }
  }

  /// Deletes groups in the given window that no longer contain any tabs
  public static func deleteEmpty(windowId: UUID) {
    DataController.perform { context in
      all(where: predicate(windowId: windowId), context: context)?.forEach {
        deleteIfEmpty($0, in: context)
      }
    }
  }
}

// MARK: - Private

extension SessionTabGroup {
  private static func predicate(windowId: UUID) -> NSPredicate {
    NSPredicate(
      format: "\(#keyPath(SessionTabGroup.sessionWindow)).windowId == %@",
      windowId as NSUUID
    )
  }

  private static func from(groupId: UUID, in context: NSManagedObjectContext) -> SessionTabGroup? {
    first(
      where: NSPredicate(
        format: "\(#keyPath(SessionTabGroup.groupId)) == %@",
        groupId as NSUUID
      ),
      context: context
    )
  }

  /// Tabs with a persisted `SessionTab` that belong to the window, as required by `SessionTab`
  private static func groupableTabs(
    _ tabIds: [UUID],
    windowId: UUID,
    in context: NSManagedObjectContext
  ) -> [SessionTab] {
    tabIds.compactMap { SessionTab.from(tabId: $0, in: context) }
      .filter { $0.sessionWindow?.windowId == windowId && !$0.isPrivate }
  }

  private static func move(
    _ tabs: [SessionTab],
    to group: SessionTabGroup,
    in context: NSManagedObjectContext
  ) {
    let previousGroups = Set(tabs.compactMap(\.sessionTabGroup)).subtracting([group])
    tabs.forEach { $0.sessionTabGroup = group }
    // Inverse relationships are only updated once pending changes are processed
    context.processPendingChanges()
    previousGroups.forEach { deleteIfEmpty($0, in: context) }
  }

  private static func deleteIfEmpty(_ group: SessionTabGroup, in context: NSManagedObjectContext) {
    guard group.sessionTabs?.contains(where: { !$0.isDeleted }) != true else { return }
    context.delete(group)
  }
}
