// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import CoreData
import TestHelpers
import XCTest

@testable import Data

class SessionTabGroupTests: CoreDataTestCase {
  private let windowId = UUID()

  override func setUp() {
    super.setUp()
    createWindow(windowId)
  }

  // MARK: - Create

  func testCreateGroupWithTabs() {
    let firstTab = createTab()
    let secondTab = createTab()
    let groupId = UUID()

    backgroundSaveAndWaitForExpectation {
      SessionTabGroup.create(
        groupId: groupId,
        windowId: windowId,
        title: "Group",
        tabIds: [firstTab, secondTab]
      )
    }

    XCTAssertEqual(groups().count, 1)
    let group = groups()[0]
    XCTAssertEqual(group.groupId, groupId)
    XCTAssertEqual(group.title, "Group")
    XCTAssertEqual(group.index, 0)
    XCTAssertEqual(Set(group.sessionTabs?.map(\.tabId) ?? []), [firstTab, secondTab])
    XCTAssertEqual(SessionTab.from(tabId: firstTab)?.sessionTabGroup?.groupId, groupId)
    XCTAssertEqual(SessionTab.from(tabId: secondTab)?.sessionTabGroup?.groupId, groupId)
  }

  func testCreatedGroupsAreIndexedInOrder() {
    createGroup(title: "First", tabs: [createTab()])
    createGroup(title: "Second", tabs: [createTab()])
    createGroup(title: "Third", tabs: [createTab()])

    XCTAssertEqual(groups().map(\.title), ["First", "Second", "Third"])
    XCTAssertEqual(groups().map(\.index), [0, 1, 2])
  }

  func testCreateGroupIgnoresPrivateAndUnknownTabs() {
    let privateTab = createTab(isPrivate: true)

    SessionTabGroup.create(
      groupId: UUID(),
      windowId: windowId,
      title: "Private",
      tabIds: [privateTab, UUID()]
    )
    waitForPendingWrites()

    XCTAssertTrue(groups().isEmpty)
    XCTAssertNil(SessionTab.from(tabId: privateTab)?.sessionTabGroup)
  }

  func testCreateGroupIgnoresTabsFromAnotherWindow() {
    let otherWindowId = UUID()
    createWindow(otherWindowId)
    let otherWindowTab = createTab(windowId: otherWindowId)

    SessionTabGroup.create(
      groupId: UUID(),
      windowId: windowId,
      title: "Group",
      tabIds: [otherWindowTab]
    )
    waitForPendingWrites()

    XCTAssertTrue(groups().isEmpty)
    XCTAssertTrue(SessionTabGroup.all(windowId: otherWindowId).isEmpty)
  }

  func testCreateGroupInUnknownWindowDoesNothing() {
    let tab = createTab()
    let unknownWindowId = UUID()

    SessionTabGroup.create(
      groupId: UUID(),
      windowId: unknownWindowId,
      title: "Group",
      tabIds: [tab]
    )
    waitForPendingWrites()

    XCTAssertTrue(groups().isEmpty)
    XCTAssertTrue(SessionTabGroup.all(windowId: unknownWindowId).isEmpty)
    XCTAssertNil(SessionTab.from(tabId: tab)?.sessionTabGroup)
  }

  func testCreateGroupWithExistingGroupIdDoesNothing() {
    let firstTab = createTab()
    let secondTab = createTab()
    let groupId = createGroup(title: "Group", tabs: [firstTab])

    SessionTabGroup.create(
      groupId: groupId,
      windowId: windowId,
      title: "Duplicate",
      tabIds: [secondTab]
    )
    waitForPendingWrites()

    XCTAssertEqual(groups().map(\.title), ["Group"])
    XCTAssertEqual(tabIds(inGroup: groupId), [firstTab])
    XCTAssertNil(SessionTab.from(tabId: secondTab)?.sessionTabGroup)
  }

  func testCreateGroupMovesTabsOutOfPreviousGroupAndDeletesItWhenEmpty() {
    let firstTab = createTab()
    let secondTab = createTab()
    let thirdTab = createTab()
    let firstGroupId = createGroup(title: "First", tabs: [firstTab])
    let secondGroupId = createGroup(title: "Second", tabs: [secondTab, thirdTab])

    // Leaves the first group empty, and the second one with a tab
    let newGroupId = createGroup(title: "New", tabs: [firstTab, secondTab])

    XCTAssertFalse(groups().contains(where: { $0.groupId == firstGroupId }))
    XCTAssertEqual(tabIds(inGroup: secondGroupId), [thirdTab])
    XCTAssertEqual(tabIds(inGroup: newGroupId), [firstTab, secondTab])
  }

  // MARK: - Add

  func testAddTabsToGroup() {
    let firstTab = createTab()
    let secondTab = createTab()
    let groupId = createGroup(title: "Group", tabs: [firstTab])

    backgroundSaveAndWaitForExpectation {
      SessionTabGroup.add(tabIds: [secondTab], toGroup: groupId)
    }

    XCTAssertEqual(tabIds(inGroup: groupId), [firstTab, secondTab])
  }

  func testAddTabsMovesThemAndDeletesPreviousGroupWhenEmpty() {
    let firstTab = createTab()
    let secondTab = createTab()
    let thirdTab = createTab()
    let firstGroupId = createGroup(title: "First", tabs: [firstTab])
    let secondGroupId = createGroup(title: "Second", tabs: [secondTab, thirdTab])

    backgroundSaveAndWaitForExpectation {
      SessionTabGroup.add(tabIds: [firstTab], toGroup: secondGroupId)
    }

    XCTAssertFalse(groups().contains(where: { $0.groupId == firstGroupId }))
    XCTAssertEqual(tabIds(inGroup: secondGroupId), [firstTab, secondTab, thirdTab])
  }

  func testAddTabsToUnknownGroupDoesNothing() {
    let tab = createTab()

    SessionTabGroup.add(tabIds: [tab], toGroup: UUID())
    waitForPendingWrites()

    XCTAssertNil(SessionTab.from(tabId: tab)?.sessionTabGroup)
    XCTAssertTrue(groups().isEmpty)
  }

  // MARK: - Rename

  func testRenameGroup() {
    let groupId = createGroup(title: "Group", tabs: [createTab()])

    backgroundSaveAndWaitForExpectation {
      SessionTabGroup.rename(groupId: groupId, title: "Renamed")
    }

    XCTAssertEqual(groups().map(\.title), ["Renamed"])
  }

  // MARK: - Delete

  func testDeleteGroupKeepsItsTabs() {
    let firstTab = createTab()
    let secondTab = createTab()
    let groupId = createGroup(title: "Group", tabs: [firstTab, secondTab])

    backgroundSaveAndWaitForExpectation {
      SessionTabGroup.delete(groupId: groupId)
    }

    XCTAssertTrue(groups().isEmpty)
    XCTAssertNotNil(SessionTab.from(tabId: firstTab))
    XCTAssertNotNil(SessionTab.from(tabId: secondTab))
    XCTAssertNil(SessionTab.from(tabId: firstTab)?.sessionTabGroup)
    XCTAssertNil(SessionTab.from(tabId: secondTab)?.sessionTabGroup)
  }

  func testDeleteAllEmptyDeletesOnlyGroupsWithoutTabs() {
    let emptiedTab = createTab()
    let keptTab = createTab()
    createGroup(title: "Emptied", tabs: [emptiedTab])
    let keptGroupId = createGroup(title: "Kept", tabs: [keptTab])

    // Deleting a tab doesn't delete its group
    backgroundSaveAndWaitForExpectation {
      SessionTab.deleteAll(tabIds: [emptiedTab])
    }
    XCTAssertNil(SessionTab.from(tabId: emptiedTab))
    XCTAssertEqual(groups().count, 2)

    backgroundSaveAndWaitForExpectation {
      SessionTabGroup.deleteAllEmpty()
    }

    XCTAssertEqual(groups().map(\.groupId), [keptGroupId])
    XCTAssertEqual(tabIds(inGroup: keptGroupId), [keptTab])
  }

  // MARK: - Helpers

  private func groups() -> [SessionTabGroup] {
    SessionTabGroup.all(windowId: windowId)
  }

  private func tabIds(inGroup groupId: UUID) -> Set<UUID> {
    let group = groups().first(where: { $0.groupId == groupId })
    return Set(group?.sessionTabs?.map(\.tabId) ?? [])
  }

  /// Operations that are rejected don't save anything, so there is nothing to wait for. Writes run
  /// one after the other though, so once one more write is saved the earlier operation is done.
  private func waitForPendingWrites() {
    createTab()
  }

  private func createWindow(_ windowId: UUID) {
    backgroundSaveAndWaitForExpectation {
      SessionWindow.createWindow(isSelected: true, uuid: windowId)
    }
  }

  @discardableResult
  private func createTab(isPrivate: Bool = false, windowId: UUID? = nil) -> UUID {
    let tabId = UUID()
    backgroundSaveAndWaitForExpectation {
      SessionTab.createIfNeeded(
        windowId: windowId ?? self.windowId,
        tabId: tabId,
        title: "Tab",
        tabURL: URL(string: "https://brave.com")!,
        isPrivate: isPrivate
      )
    }
    return tabId
  }

  @discardableResult
  private func createGroup(title: String, tabs: [UUID]) -> UUID {
    let groupId = UUID()
    backgroundSaveAndWaitForExpectation {
      SessionTabGroup.create(groupId: groupId, windowId: windowId, title: title, tabIds: tabs)
    }
    return groupId
  }
}
