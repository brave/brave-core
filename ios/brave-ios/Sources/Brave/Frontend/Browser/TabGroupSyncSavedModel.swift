// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation

/// Saved tab groups for the sync service.
///
/// The factory takes the object installed before the profile loads and does not look again.

public final class TabGroupSyncSavedModel: NSObject, TabGroupSyncLocalModel {
  public static let shared = TabGroupSyncSavedModel()

  private var pauseCount = 0

  public func localGroupIDs() -> [UUID] {
    read { context in
      let windows = (SessionWindow.all(context: context) ?? []).sorted { $0.index < $1.index }
      return windows.flatMap { window in
        (window.sessionTabGroups ?? [])
          .sorted { $0.index < $1.index }
          .map(\.groupId)
      }
    }
  }

  public func title(forGroupID groupID: UUID) -> String {
    read { context in
      self.group(groupID, in: context)?.title ?? ""
    }
  }

  public func color(forGroupID groupID: UUID) -> Int16 {
    read { context in
      self.group(groupID, in: context)?.resolvedColor ?? 0
    }
  }

  public func isCollapsed(forGroupID groupID: UUID) -> Bool {
    read { context in
      self.group(groupID, in: context)?.collapsed ?? false
    }
  }

  public func syncTabIDs(forGroupID groupID: UUID) -> [NSNumber] {
    read { context in
      guard let group = self.group(groupID, in: context) else { return [] }
      return self.syncTabs(in: group).map { NSNumber(value: $0.syncTabId) }
    }
  }

  public func title(forSyncTabID syncTabID: Int32) -> String {
    guard syncTabID > 0 else { return "" }
    return read { context in
      self.tab(syncTabID, in: context)?.title ?? ""
    }
  }

  public func url(forSyncTabID syncTabID: Int32) -> URL? {
    guard syncTabID > 0 else { return nil }
    return read { context in
      self.tab(syncTabID, in: context)?.url
    }
  }

  public func selectedSyncTabIDs() -> [NSNumber] {
    read { context in
      let tabs =
        SessionTab.all(
          where: NSPredicate(format: "isSelected == YES AND isPrivate == NO AND syncTabId > 0"),
          context: context
        ) ?? []
      return tabs.map { NSNumber(value: $0.syncTabId) }
    }
  }

  public func setLocalObservationPaused(_ paused: Bool) {
    if paused {
      pauseCount += 1
    } else {
      pauseCount = max(0, pauseCount - 1)
    }
  }

  public func isLocalObservationPaused() -> Bool {
    pauseCount > 0
  }

  private func group(_ groupID: UUID, in context: NSManagedObjectContext) -> SessionTabGroup? {
    SessionTabGroup.first(
      where: NSPredicate(format: "groupId == %@", groupID as NSUUID),
      context: context
    )
  }

  private func tab(_ syncTabID: Int32, in context: NSManagedObjectContext) -> SessionTab? {
    SessionTab.first(
      where: NSPredicate(format: "syncTabId == %d AND isPrivate == NO", syncTabID),
      context: context
    )
  }

  /// Regular tabs in the group that have a sync id, in window order.
  private func syncTabs(in group: SessionTabGroup) -> [SessionTab] {
    (group.sessionTabs ?? [])
      .filter { !$0.isPrivate && $0.syncTabId > 0 }
      .sorted { $0.index < $1.index }
  }

  private func read<T>(_ body: (NSManagedObjectContext) -> T) -> T {
    let context = DataController.viewContext
    var value: T?
    context.performAndWait {
      value = body(context)
    }
    return value!
  }
}
