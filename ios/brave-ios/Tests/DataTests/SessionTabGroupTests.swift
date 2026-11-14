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
    DataController.shared.initializeOnce()

    let windowCreated = expectation(forNotification: .NSManagedObjectContextDidSave, object: nil)
    SessionWindow.createWindow(isSelected: true, uuid: windowId)
    wait(for: [windowCreated], timeout: 5)
  }

  // MARK: - Create

  func testCreateGroupWithTabs() {
    let firstTab = createTab()
    let secondTab = createTab()
    let groupId = UUID()

    SessionTabGroup.create(
      groupId: groupId,
      windowId: windowId,
      title: "Group",
      tabIds: [firstTab, secondTab]
    )
    waitUntil { self.groups().count == 1 }

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
    // Operations run in order, so once this group exists the one above has been handled
    createGroup(title: "Regular", tabs: [createTab()])

    XCTAssertEqual(groups().map(\.title), ["Regular"])
    XCTAssertNil(SessionTab.from(tabId: privateTab)?.sessionTabGroup)
  }

  func testCreateGroupIgnoresTabsFromAnotherWindow() {
    let otherWindowId = UUID()
    let windowCreated = expectation(forNotification: .NSManagedObjectContextDidSave, object: nil)
    SessionWindow.createWindow(isSelected: false, uuid: otherWindowId)
    wait(for: [windowCreated], timeout: 5)
    let otherWindowTab = createTab(windowId: otherWindowId)

    SessionTabGroup.create(
      groupId: UUID(),
      windowId: windowId,
      title: "Group",
      tabIds: [otherWindowTab]
    )
    createGroup(title: "Regular", tabs: [createTab()])

    XCTAssertEqual(groups().map(\.title), ["Regular"])
    XCTAssertTrue(SessionTabGroup.all(windowId: otherWindowId).isEmpty)
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

    SessionTabGroup.add(tabIds: [secondTab], toGroup: groupId)
    waitUntil { self.tabIds(inGroup: groupId).count == 2 }

    XCTAssertEqual(tabIds(inGroup: groupId), [firstTab, secondTab])
  }

  func testAddTabsMovesThemAndDeletesPreviousGroupWhenEmpty() {
    let firstTab = createTab()
    let secondTab = createTab()
    let thirdTab = createTab()
    let firstGroupId = createGroup(title: "First", tabs: [firstTab])
    let secondGroupId = createGroup(title: "Second", tabs: [secondTab, thirdTab])

    SessionTabGroup.add(tabIds: [firstTab], toGroup: secondGroupId)
    waitUntil { self.groups().count == 1 }

    XCTAssertFalse(groups().contains(where: { $0.groupId == firstGroupId }))
    XCTAssertEqual(tabIds(inGroup: secondGroupId), [firstTab, secondTab, thirdTab])
  }

  func testAddTabsToUnknownGroupDoesNothing() {
    let tab = createTab()

    SessionTabGroup.add(tabIds: [tab], toGroup: UUID())
    createGroup(title: "Group", tabs: [createTab()])

    XCTAssertNil(SessionTab.from(tabId: tab)?.sessionTabGroup)
    XCTAssertEqual(groups().count, 1)
  }

  // MARK: - Rename

  func testRenameGroup() {
    let groupId = createGroup(title: "Group", tabs: [createTab()])

    SessionTabGroup.rename(groupId: groupId, title: "Renamed")
    waitUntil { self.groups().first?.title == "Renamed" }

    XCTAssertEqual(groups().map(\.title), ["Renamed"])
  }

  // MARK: - Delete

  func testDeleteGroupKeepsItsTabs() {
    let firstTab = createTab()
    let secondTab = createTab()
    let groupId = createGroup(title: "Group", tabs: [firstTab, secondTab])

    SessionTabGroup.delete(groupId: groupId)
    waitUntil { self.groups().isEmpty }

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
    SessionTab.deleteAll(tabIds: [emptiedTab])
    waitUntil { SessionTab.from(tabId: emptiedTab) == nil }
    XCTAssertEqual(groups().count, 2)

    SessionTabGroup.deleteAllEmpty()
    waitUntil { self.groups().count == 1 }

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

  @discardableResult
  private func createTab(isPrivate: Bool = false, windowId: UUID? = nil) -> UUID {
    let tabId = UUID()
    SessionTab.createIfNeeded(
      windowId: windowId ?? self.windowId,
      tabId: tabId,
      title: "Tab",
      tabURL: URL(string: "https://brave.com")!,
      isPrivate: isPrivate
    )
    waitUntil { SessionTab.from(tabId: tabId) != nil }
    return tabId
  }

  /// Creates a group and waits until it has been saved
  @discardableResult
  private func createGroup(title: String, tabs: [UUID]) -> UUID {
    let groupId = UUID()
    SessionTabGroup.create(groupId: groupId, windowId: windowId, title: title, tabIds: tabs)
    waitUntil { self.tabIds(inGroup: groupId).count == tabs.count }
    return groupId
  }

  private func waitUntil(
    file: StaticString = #filePath,
    line: UInt = #line,
    _ condition: @escaping () -> Bool
  ) {
    let expectation = XCTNSPredicateExpectation(
      predicate: NSPredicate { _, _ in condition() },
      object: nil
    )
    let result = XCTWaiter().wait(for: [expectation], timeout: 5)
    XCTAssertEqual(result, .completed, file: file, line: line)
  }
}
