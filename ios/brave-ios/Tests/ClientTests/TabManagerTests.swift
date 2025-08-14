// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import Shared
import Storage
import UIKit
import Web
import WebKit
import XCTest

@testable import Brave
@testable import Data

open class MockTabManagerStateDelegate: TabManagerStateDelegate {
  var numberOfTabsStored = 0
  public func tabManagerWillStoreTabs(_ tabs: [any TabState]) {
    numberOfTabsStored = tabs.count
  }
}

extension TabManager {
  func tabs(isPrivate: Bool) -> [any TabState] {
    assert(Thread.isMainThread)
    return allTabs.filter { $0.isPrivate == isPrivate }
  }
  @discardableResult
  @MainActor fileprivate func addTestTab(isPrivate: Bool) -> any TabState {
    return addTab(zombie: true, isPrivate: isPrivate)
  }
}

struct MethodSpy {
  let functionName: String
  let method: ((_ tabs: [TabState?]) -> Void)?

  init(functionName: String) {
    self.functionName = functionName
    self.method = nil
  }

  init(functionName: String, method: ((_ tabs: [TabState?]) -> Void)?) {
    self.functionName = functionName
    self.method = method
  }
}

open class MockTabManagerDelegate: TabManagerDelegate {

  //this array represents the order in which delegate methods should be called.
  //each delegate method will pop the first struct from the array. If the method name doesn't match the struct then the order is incorrect
  //Then it evaluates the method closure which will return true/false depending on if the tabs are correct
  var methodCatchers: [MethodSpy] = []

  func expect(_ methods: [MethodSpy]) {
    self.methodCatchers = methods
  }

  func verify(_ message: String) {
    XCTAssertTrue(methodCatchers.isEmpty, message)
  }

  func testDelegateMethodWithName(_ name: String, tabs: [TabState?]) {
    guard let spy = self.methodCatchers.first else {
      XCTAssert(
        false,
        "No method was availible in the queue. For the delegate method \(name) to use"
      )
      return
    }
    XCTAssertEqual(spy.functionName, name)
    if let methodCheck = spy.method {
      methodCheck(tabs)
    }
    methodCatchers.removeFirst()
  }

  public func tabManager(
    _ tabManager: TabManager,
    didSelectedTabChange selected: (any TabState)?,
    previous: (any TabState)?
  ) {
    testDelegateMethodWithName(#function, tabs: [selected, previous])
  }

  public func tabManager(_ tabManager: TabManager, didAddTab tab: some TabState) {
    testDelegateMethodWithName(#function, tabs: [tab])
  }

  public func tabManager(_ tabManager: TabManager, didRemoveTab tab: some TabState) {
    testDelegateMethodWithName(#function, tabs: [tab])
  }

  public func tabManagerDidRestoreTabs(_ tabManager: TabManager) {
    testDelegateMethodWithName(#function, tabs: [])
  }

  public func tabManager(_ tabManager: TabManager, willRemoveTab tab: some TabState) {
    testDelegateMethodWithName(#function, tabs: [tab])
  }

  public func tabManager(_ tabManager: TabManager, willAddTab tab: some TabState) {
    testDelegateMethodWithName(#function, tabs: [tab])
  }

  public func tabManagerDidAddTabs(_ tabManager: TabManager) {
    testDelegateMethodWithName(#function, tabs: [])
  }

  public func tabManagerDidRemoveAllTabs(_ tabManager: TabManager, toast: ButtonToast?) {
    testDelegateMethodWithName(#function, tabs: [])
  }
}

@MainActor class TabManagerTests: XCTestCase {

  let willRemove = MethodSpy(functionName: "tabManager(_:willRemoveTab:)")
  let didRemove = MethodSpy(functionName: "tabManager(_:didRemoveTab:)")
  let willAdd = MethodSpy(functionName: "tabManager(_:willAddTab:)")
  let didAdd = MethodSpy(functionName: "tabManager(_:didAddTab:)")

  var manager: TabManager!
  private let privateBrowsingManager = PrivateBrowsingManager()
  private let testWindowId = UUID()

  override class func setUp() {
    Preferences.UserScript.translate.value = false
  }

  override func setUp() {
    super.setUp()

    DataController.shared.initializeOnce()
    manager = TabManager(
      windowId: testWindowId,
      rewards: nil,
      braveCore: nil,
      privateBrowsingManager: privateBrowsingManager
    )
    privateBrowsingManager.isPrivateBrowsing = false
    Preferences.Privacy.persistentPrivateBrowsing.reset()
    Preferences.Privacy.privateBrowsingOnly.reset()
  }

  private func setPersistentPrivateMode(_ isPersistent: Bool) {
    Preferences.Privacy.persistentPrivateBrowsing.value = isPersistent

    if isPersistent {
      Preferences.Privacy.privateBrowsingOnly.value = false
    }
  }

  func testTabManagerDoesNotCallTabManagerStateDelegateOnStoreChangesWithPrivateTabs() {
    let stateDelegate = MockTabManagerStateDelegate()
    manager.stateDelegate = stateDelegate
    let configuration = WKWebViewConfiguration()
    configuration.processPool = WKProcessPool()
    configuration.websiteDataStore = .nonPersistent()

    // test that non-private tabs are saved to the db
    // add some non-private tabs to the tab manager
    for _ in 0..<3 {
      let tab = TabStateFactory.create(with: .init(initialConfiguration: configuration))
      tab.setVirtualURL(URL(string: "http://yahoo.com")!)
      manager.configureTab(
        tab,
        request: URLRequest(url: tab.visibleURL!),
        flushToDisk: false,
        zombie: false
      )
    }

    XCTAssertEqual(
      stateDelegate.numberOfTabsStored,
      0,
      "Expected state delegate to have been called with 3 tabs, but called with \(stateDelegate.numberOfTabsStored)"
    )
  }

  func testAddTab() {
    let delegate = MockTabManagerDelegate()
    manager.addDelegate(delegate)

    delegate.expect([willAdd, didAdd])
    manager.addTestTab(isPrivate: false)
    delegate.verify("Not all delegate methods were called")
  }

  func testDidDeleteLastTab() {
    let delegate = MockTabManagerDelegate()

    //create the tab before adding the mock delegate. So we don't have to check delegate calls we dont care about
    let tab = manager.addTestTab(isPrivate: false)
    manager.selectTab(tab)
    manager.addDelegate(delegate)

    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      let next = tabs[0]!
      let previous = tabs[1]!
      XCTAssertTrue(previous !== next)
      XCTAssertTrue(previous === tab)
      XCTAssertFalse(next.isPrivate)
    }
    delegate.expect([willRemove, didRemove, willAdd, didAdd, didSelect])
    manager.removeTab(tab)
    delegate.verify("Not all delegate methods were called")
  }

  func testDidDeleteLastPrivateTab() {
    let delegate = MockTabManagerDelegate()

    //create the tab before adding the mock delegate. So we don't have to check delegate calls we dont care about
    let tab = manager.addTestTab(isPrivate: true)
    manager.selectTab(tab)
    manager.addDelegate(delegate)

    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      let next = tabs[0]!
      let previous = tabs[1]!
      XCTAssertTrue(previous !== next)
      XCTAssertTrue(previous === tab)
      XCTAssertTrue(next.isPrivate)
    }
    delegate.expect([willRemove, didRemove, willAdd, didAdd, didSelect])
    manager.removeTab(tab)
    delegate.verify("Not all delegate methods were called")
  }

  func testDeletePrivateTabsPersistenceOnExit() {
    setPersistentPrivateMode(true)

    // create one private and one normal tab
    let tab = manager.addTestTab(isPrivate: false)
    manager.selectTab(tab)
    manager.selectTab(manager.addTestTab(isPrivate: true))

    XCTAssertEqual(
      manager.selectedTab!.isPrivate,
      true,
      "The selected tab should be the private tab"
    )
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      1,
      "There should only be one private tab"
    )

    manager.selectTab(tab)
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      1,
      "If the normal tab is selected the private tab should NOT be deleted"
    )
    XCTAssertEqual(
      manager.tabs(isPrivate: false).count,
      1,
      "The regular tab should stil be around"
    )

    manager.selectTab(manager.addTestTab(isPrivate: true))
    XCTAssertEqual(manager.tabs(isPrivate: true).count, 2, "There should be two private tabs")
    manager.willSwitchTabMode(leavingPBM: true)
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      2,
      "After willSwitchTabMode there should be 2 private tabs"
    )

    manager.selectTab(manager.addTestTab(isPrivate: true))
    manager.selectTab(manager.addTestTab(isPrivate: true))
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      4,
      "Private tabs should not be deleted when another one is added"
    )
    manager.selectTab(manager.addTestTab(isPrivate: false))
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      4,
      "But once we add a normal tab we've switched out of private mode. Private tabs should be be persistent"
    )
    XCTAssertEqual(
      manager.tabs(isPrivate: false).count,
      2,
      "The original normal tab and the new one should both still exist"
    )
  }

  func testDeletePrivateTabsOnNonPersistenceExit() {
    // create one private and one normal tab
    let tab = manager.addTestTab(isPrivate: false)
    manager.selectTab(tab)
    manager.selectTab(manager.addTestTab(isPrivate: true))

    XCTAssertEqual(
      manager.selectedTab?.isPrivate,
      true,
      "The selected tab should be the private tab"
    )
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      1,
      "There should only be one private tab"
    )

    manager.selectTab(tab)
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      0,
      "If the normal tab is selected the private tab should have been deleted"
    )
    XCTAssertEqual(
      manager.tabs(isPrivate: false).count,
      1,
      "The regular tab should stil be around"
    )

    manager.selectTab(manager.addTestTab(isPrivate: true))
    XCTAssertEqual(manager.tabs(isPrivate: true).count, 1, "There should be one new private tab")
    manager.willSwitchTabMode(leavingPBM: true)
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      0,
      "After willSwitchTabMode there should be no more private tabs"
    )

    manager.selectTab(manager.addTestTab(isPrivate: true))
    manager.selectTab(manager.addTestTab(isPrivate: true))
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      2,
      "Private tabs should not be deleted when another one is added"
    )
    manager.selectTab(manager.addTestTab(isPrivate: false))
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      0,
      "But once we add a normal tab we've switched out of private mode. Private tabs should be deleted"
    )
    XCTAssertEqual(
      manager.tabs(isPrivate: false).count,
      2,
      "The original normal tab and the new one should both still exist"
    )
  }

  func testTogglePBMDeletePersistent() {
    setPersistentPrivateMode(true)

    let tab = manager.addTestTab(isPrivate: false)
    manager.selectTab(tab)
    manager.selectTab(manager.addTestTab(isPrivate: false))
    manager.selectTab(manager.addTestTab(isPrivate: true))

    manager.willSwitchTabMode(leavingPBM: false)
    XCTAssertEqual(manager.tabs(isPrivate: true).count, 1, "There should be 1 private tab")
    manager.willSwitchTabMode(leavingPBM: true)
    XCTAssertEqual(manager.tabs(isPrivate: true).count, 1, "There should be 1 private tab")
    manager.removeTab(tab)
    XCTAssertEqual(manager.tabs(isPrivate: false).count, 1, "There should be 1 normal tab")
  }

  func testTogglePBMDeleteNonPersistent() {
    let tab = manager.addTestTab(isPrivate: false)
    manager.selectTab(tab)
    manager.selectTab(manager.addTestTab(isPrivate: false))
    manager.selectTab(manager.addTestTab(isPrivate: true))

    manager.willSwitchTabMode(leavingPBM: false)
    XCTAssertEqual(manager.tabs(isPrivate: true).count, 1, "There should be 1 private tab")
    manager.willSwitchTabMode(leavingPBM: true)
    XCTAssertEqual(manager.tabs(isPrivate: true).count, 0, "There should be 0 private tab")
    manager.removeTab(tab)
    XCTAssertEqual(manager.tabs(isPrivate: false).count, 1, "There should be 1 normal tab")
  }

  func testDeleteNonSelectedTab() {
    let delegate = MockTabManagerDelegate()

    //create the tab before adding the mock delegate. So we don't have to check delegate calls we dont care about
    let tab = manager.addTestTab(isPrivate: false)
    manager.selectTab(tab)
    manager.addTestTab(isPrivate: false)
    let deleteTab = manager.addTestTab(isPrivate: false)
    manager.addDelegate(delegate)

    delegate.expect([willRemove, didRemove])
    manager.removeTab(deleteTab)

    delegate.verify("Not all delegate methods were called")
  }

  func testDeleteSelectedTab() {
    func addTab(_ visit: Bool) -> TabState {
      return manager.addTab(
        zombie: true,
        lastActiveTime: visit ? nil : .distantPast,
        isPrivate: false
      )
    }

    let tab0 = addTab(false)  // not visited
    let tab1 = addTab(true)
    let tab2 = addTab(true)
    let tab3 = addTab(true)
    let tab4 = addTab(false)  // not visited

    // starting at tab1, we should be selecting
    // [ tab3, tab4, tab2, tab0 ]

    manager.selectTab(tab1)
    tab1.opener = tab3
    manager.removeTab(manager.selectedTab!)
    // Rule: parent tab if it was the most recently visited
    XCTAssertEqual(manager.selectedTab?.id, tab3.id)

    manager.removeTab(manager.selectedTab!)
    // Rule: next to the right.
    XCTAssertEqual(manager.selectedTab?.id, tab4.id)

    manager.removeTab(manager.selectedTab!)
    // Rule: next to the left, when none to the right
    XCTAssertEqual(manager.selectedTab?.id, tab2.id)

    manager.removeTab(manager.selectedTab!)
    // Rule: last one left.
    XCTAssertEqual(manager.selectedTab?.id, tab0.id)
  }

  func testDeleteLastTab() {
    let delegate = MockTabManagerDelegate()

    //create the tab before adding the mock delegate. So we don't have to check delegate calls we dont care about
    (0..<10).forEach { _ in manager.addTestTab(isPrivate: false) }
    manager.selectTab(manager.allTabs.last)
    let deleteTab = manager.allTabs.last
    let newSelectedTab = manager.allTabs[8]
    manager.addDelegate(delegate)

    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      let next = tabs[0]!
      let previous = tabs[1]!
      XCTAssertEqual(deleteTab?.id, previous.id)
      XCTAssertEqual(next.id, newSelectedTab.id)
    }
    delegate.expect([willRemove, didRemove, didSelect])
    manager.removeTab(manager.allTabs.last!)

    delegate.verify("Not all delegate methods were called")
  }

  func testDelegatesCalledWhenRemovingPrivateTabsPersistence() {
    setPersistentPrivateMode(true)

    //setup
    let delegate = MockTabManagerDelegate()

    // create one private and one normal tab
    let tab = manager.addTestTab(isPrivate: false)
    let newTab = manager.addTestTab(isPrivate: false)
    manager.selectTab(tab)
    manager.selectTab(manager.addTestTab(isPrivate: true))
    manager.addDelegate(delegate)

    // Double check a few things
    XCTAssertEqual(
      manager.selectedTab?.isPrivate,
      true,
      "The selected tab should be the private tab"
    )
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      1,
      "There should only be one private tab"
    )

    // switch to normal mode. Which should not delete the private tabs
    manager.willSwitchTabMode(leavingPBM: true)

    //make sure tabs are NOT cleared properly and indexes are reset
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      1,
      "Private tab should not have been deleted"
    )
    XCTAssertNotEqual(manager.selectedIndex, -1, "The selected index should have been reset")

    // didSelect should still be called when switching between a tab
    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      XCTAssertNotNil(tabs[1], "there should be a previous tab")
      let next = tabs[0]!
      XCTAssertFalse(next.isPrivate)
    }

    // make sure delegate method is actually called
    delegate.expect([didSelect])

    // select the new tab to trigger the delegate methods
    manager.selectTab(newTab)

    // check
    delegate.verify("Not all delegate methods were called")
  }

  func testDelegatesCalledWhenRemovingPrivateTabsNonPersistence() {
    //setup
    let delegate = MockTabManagerDelegate()

    // create one private and one normal tab
    let tab = manager.addTestTab(isPrivate: false)
    let newTab = manager.addTestTab(isPrivate: false)
    manager.selectTab(tab)
    manager.selectTab(manager.addTestTab(isPrivate: true))
    manager.addDelegate(delegate)

    // Double check a few things
    XCTAssertEqual(
      manager.selectedTab?.isPrivate,
      true,
      "The selected tab should be the private tab"
    )
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      1,
      "There should only be one private tab"
    )

    // switch to normal mode. Which should delete the private tabs
    manager.willSwitchTabMode(leavingPBM: true)

    //make sure tabs are cleared properly and indexes are reset
    XCTAssertEqual(
      manager.tabs(isPrivate: true).count,
      0,
      "Private tab should have been deleted"
    )
    XCTAssertEqual(manager.selectedIndex, -1, "The selected index should have been reset")

    // didSelect should still be called when switching between a nil tab
    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      XCTAssertNil(tabs[1], "there should be no previous tab")
      let next = tabs[0]!
      XCTAssertFalse(next.isPrivate)
    }

    // make sure delegate method is actually called
    delegate.expect([didSelect])

    // select the new tab to trigger the delegate methods
    manager.selectTab(newTab)

    // check
    delegate.verify("Not all delegate methods were called")
  }

  func testDeleteFirstTab() {
    let delegate = MockTabManagerDelegate()

    //create the tab before adding the mock delegate. So we don't have to check delegate calls we dont care about
    (0..<10).forEach { _ in manager.addTestTab(isPrivate: false) }
    manager.selectTab(manager.allTabs.first)
    let deleteTab = manager.allTabs.first
    let newSelectedTab = manager.allTabs[1]
    manager.addDelegate(delegate)

    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      let next = tabs[0]!
      let previous = tabs[1]!
      XCTAssertEqual(deleteTab?.id, previous.id)
      XCTAssertEqual(next.id, newSelectedTab.id)
    }
    delegate.expect([willRemove, didRemove, didSelect])
    manager.removeTab(manager.allTabs.first!)
    delegate.verify("Not all delegate methods were called")
  }

  // Private tabs and regular tabs are in the same tabs array.
  // Make sure that when a private tab is added inbetween regular tabs it isnt accidently selected when removing a regular tab
  func testTabsIndex() {
    let delegate = MockTabManagerDelegate()

    // We add 2 tabs. Then a private one before adding another normal tab and selecting it.
    // Make sure that when the last one is deleted we dont switch to the private tab
    manager.addTestTab(isPrivate: false)
    let newSelected = manager.addTestTab(isPrivate: false)
    manager.addTestTab(isPrivate: true)
    let deleted = manager.addTestTab(isPrivate: false)
    manager.selectTab(manager.allTabs.last)
    manager.addDelegate(delegate)

    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      let next = tabs[0]!
      let previous = tabs[1]!
      XCTAssertEqual(deleted.id, previous.id)
      XCTAssertEqual(next.id, newSelected.id)
    }
    delegate.expect([willRemove, didRemove, didSelect])
    manager.removeTab(manager.allTabs.last!)

    delegate.verify("Not all delegate methods were called")
  }

  func testTabsIndexClosingFirst() {
    let delegate = MockTabManagerDelegate()

    // We add 2 tabs. Then a private one before adding another normal tab and selecting the first.
    // Make sure that when the last one is deleted we dont switch to the private tab
    let deleted = manager.addTestTab(isPrivate: false)
    let newSelected = manager.addTestTab(isPrivate: false)
    manager.addTestTab(isPrivate: true)
    manager.addTestTab(isPrivate: false)
    manager.selectTab(manager.allTabs.first)
    manager.addDelegate(delegate)

    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      let next = tabs[0]!
      let previous = tabs[1]!
      XCTAssertEqual(deleted.id, previous.id)
      XCTAssertEqual(next.id, newSelected.id)
    }
    delegate.expect([willRemove, didRemove, didSelect])
    manager.removeTab(manager.allTabs.first!)
    delegate.verify("Not all delegate methods were called")
  }

  func testRemoveOnlyTab() {
    let delegate = MockTabManagerDelegate()

    let tab = manager.addTestTab(isPrivate: false)

    manager.addDelegate(delegate)
    delegate.expect([
      willRemove, didRemove, willAdd, didAdd,
      MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)"),
    ])
    manager.removeTab(tab)

    delegate.verify("Not all delegate methods were called")

    XCTAssertFalse(manager.allTabs.isEmpty, "Should create a new tab when all others are removed")
    XCTAssertFalse(manager.allTabs.first!.isPrivate, "The new tab should be a regular tab")
  }

  func testRemoveAllTabs() {
    (0..<10).forEach { _ in manager.addTestTab(isPrivate: false) }
    manager.removeAll()

    XCTAssertFalse(manager.allTabs.isEmpty, "Should create a new tab when all others are removed")
    XCTAssertFalse(manager.allTabs.first!.isPrivate, "The new tab should be a regular tab")
  }

  func testRemoveAllFromCurrentMode() {
    (0..<10).forEach { index in manager.addTestTab(isPrivate: false) }
    manager.selectTab(manager.allTabs.first!)
    manager.removeAllForCurrentMode(isActiveTabIncluded: false)

    XCTAssertEqual(manager.allTabs.count, 1, "The active tab should not be removed")
    XCTAssertFalse(
      manager.allTabs.first!.isPrivate,
      "The last remaining tab should be a regular tab"
    )
  }

  func testRemoveAllWithFilter() {
    (0..<10).forEach { index in manager.addTestTab(isPrivate: false) }
    (0..<10).forEach { index in manager.addTestTab(isPrivate: true) }
    manager.removeAllTabsForPrivateMode(isPrivate: true)

    XCTAssertEqual(
      manager.tabsCountForMode(isPrivate: false),
      10,
      "Only private tabs should be removed"
    )
  }

  func testMoveTabToEnd() {
    let firstTab = manager.addTestTab(isPrivate: false)
    manager.selectTab(firstTab)
    let secondTab = manager.addTestTab(isPrivate: false)
    manager.addTestTab(isPrivate: true)
    let thirdTab = manager.addTestTab(isPrivate: false)

    manager.moveTab(firstTab, toIndex: manager.tabs(isPrivate: false).count - 1)

    let reorderedTabs = manager.tabs(isPrivate: false)
    XCTAssertEqual(firstTab.id, reorderedTabs.last?.id, "First tab should now be the last tab")
    XCTAssertEqual(manager.selectedTab?.id, firstTab.id, "First tab should still be selected")
    XCTAssertEqual(
      [secondTab, thirdTab, firstTab].map(\.id),
      reorderedTabs.map(\.id),
      "Tabs should shift and the have the correct order ignoring the private tab"
    )
  }

  func testMoveTabToStart() {
    let firstTab = manager.addTestTab(isPrivate: false)
    manager.selectTab(firstTab)
    let secondTab = manager.addTestTab(isPrivate: false)
    manager.addTestTab(isPrivate: true)
    let thirdTab = manager.addTestTab(isPrivate: false)

    manager.moveTab(thirdTab, toIndex: 0)

    let reorderedTabs = manager.tabs(isPrivate: false)
    XCTAssertEqual(thirdTab.id, reorderedTabs.first?.id, "Last tab should now be the first tab")
    XCTAssertEqual(manager.selectedTab?.id, firstTab.id, "First tab should still be selected")
    XCTAssertEqual(
      [thirdTab, firstTab, secondTab].map(\.id),
      reorderedTabs.map(\.id),
      "Tabs should shift and the have the correct order ignoring the private tab"
    )
  }

  func testMoveTabToMiddle() {
    let firstTab = manager.addTestTab(isPrivate: false)
    let secondTab = manager.addTestTab(isPrivate: false)
    manager.addTestTab(isPrivate: true)
    let thirdTab = manager.addTabAndSelect(isPrivate: false)
    let forthTab = manager.addTestTab(isPrivate: false)

    manager.moveTab(forthTab, toIndex: 1)

    let reorderedTabs = manager.tabs(isPrivate: false)
    XCTAssertEqual(forthTab.id, reorderedTabs[1].id, "Last tab should now be at index 1")
    XCTAssertEqual(manager.selectedTab?.id, thirdTab.id, "Third tab should still be selected")
    XCTAssertEqual(
      [firstTab, forthTab, secondTab, thirdTab].map(\.id),
      reorderedTabs.map(\.id),
      "Tabs should shift and the have the correct order ignoring the private tab"
    )
  }

  func testQueryAddedSessionTabs() {
    let delegate = MockTabManagerDelegate()
    manager.addDelegate(delegate)
    DataController.shared = InMemoryDataController()
    DataController.shared.initializeOnce()

    // Create a session window for SessionTab's to be added to
    let windowCreateExpectation = expectation(
      forNotification: NSNotification.Name.NSManagedObjectContextDidSave,
      object: nil
    )
    SessionWindow.createWindow(isSelected: true, uuid: testWindowId)
    wait(for: [windowCreateExpectation], timeout: 5)

    delegate.expect([willAdd, didAdd])
    let tabAddExpectation = expectation(
      forNotification: .NSManagedObjectContextDidSave,
      object: nil
    )
    let tab = manager.addTestTab(isPrivate: false)
    wait(for: [tabAddExpectation], timeout: 5)
    delegate.verify("Not all delegate methods were called")

    let storedTabs = SessionTab.all()
    XCTAssertNotNil(storedTabs.first(where: { $0.tabId == tab.id }))
    XCTAssertEqual(storedTabs.count, 1)
  }

  func testQueryAddedSessionPrivateTabs() {
    let delegate = MockTabManagerDelegate()
    manager.addDelegate(delegate)
    DataController.shared = InMemoryDataController()
    DataController.shared.initializeOnce()

    delegate.expect([willAdd, didAdd])
    manager.addTestTab(isPrivate: true)
    delegate.verify("Not all delegate methods were called")

    let storedTabs = SessionTab.all()
    // Shouldn't be storing any private tabs
    XCTAssertTrue(storedTabs.isEmpty)
  }

  func testQueryAddedSessionMixedTabs() {
    let delegate = MockTabManagerDelegate()
    manager.addDelegate(delegate)
    DataController.shared = InMemoryDataController()
    DataController.shared.initializeOnce()

    // Create a session window for SessionTab's to be added to
    let windowCreateExpectation = expectation(
      forNotification: .NSManagedObjectContextDidSave,
      object: nil
    )
    SessionWindow.createWindow(isSelected: true, uuid: testWindowId)
    wait(for: [windowCreateExpectation], timeout: 5)

    delegate.expect([willAdd, didAdd, willAdd, didAdd])
    manager.addTestTab(isPrivate: true)
    let tabAddExpectation = expectation(
      forNotification: .NSManagedObjectContextDidSave,
      object: nil
    )
    let tab = manager.addTestTab(isPrivate: false)
    wait(for: [tabAddExpectation], timeout: 5)
    delegate.verify("Not all delegate methods were called")

    let storedTabs = SessionTab.all()
    XCTAssertNotNil(
      storedTabs.first(where: { $0.tabId == tab.id }),
      "Couldn't find added tab: \(tab) in stored tabs: \(storedTabs)"
    )
    // Shouldn't be storing any private tabs
    XCTAssertEqual(storedTabs.count, 1)
  }

  func testGetTabForURL() {
    // mock all tabs 0-4 in standard mode, 5-9 in private mode
    (0..<10).forEach { index in
      guard let url = URL(string: "https://www.\(index).com") else { return }
      let urlRequest = URLRequest(url: url)
      manager.addTab(urlRequest, zombie: true, isPrivate: index > 4)
    }
    // add one more tab that existed in standard mode in private mode
    guard let url = URL(string: "https://www.0.com") else { return }
    let urlRequest = URLRequest(url: url)
    manager.addTab(urlRequest, zombie: true, isPrivate: true)

    if let urlZero = URL(string: "https://www.0.com") {
      // looking for a tab exists in both standard mode and private mode
      let standardTab = manager.getTabForURL(urlZero, isPrivate: false)
      XCTAssertNotNil(standardTab)
      let privateTab = manager.getTabForURL(urlZero, isPrivate: true)
      XCTAssertNotNil(privateTab)
    }
    // looking for a tab only exists in standard mode
    if let urlOne = URL(string: "https://www.1.com") {
      let standardTab = manager.getTabForURL(urlOne, isPrivate: false)
      XCTAssertNotNil(standardTab)
      let privateTab = manager.getTabForURL(urlOne, isPrivate: true)
      XCTAssertNil(privateTab)
    }
    // looking for a tab only exists in private mode
    if let urlFive = URL(string: "https://www.5.com") {
      let standardTab = manager.getTabForURL(urlFive, isPrivate: false)
      XCTAssertNil(standardTab)
      let privateTab = manager.getTabForURL(urlFive, isPrivate: true)
      XCTAssertNotNil(privateTab)
    }
  }

  func testMoveMultipleTabsToEnd() {
    let firstTab = manager.addTestTab(isPrivate: false)
    let secondTab = manager.addTestTab(isPrivate: false)
    let thirdTab = manager.addTestTab(isPrivate: false)
    let fourthTab = manager.addTestTab(isPrivate: false)
    manager.selectTab(firstTab)

    // Move first and second tabs to the end (index 2 in current mode)
    manager.moveTabs([firstTab.id, secondTab.id], toIndex: 2)

    let reorderedTabs = manager.tabs(isPrivate: false)
    XCTAssertEqual(
      [thirdTab, fourthTab, firstTab, secondTab].map(\.id),
      reorderedTabs.map(\.id),
      "First two tabs should now be at the end"
    )
    XCTAssertEqual(manager.selectedTab?.id, firstTab.id, "First tab should still be selected")
  }

  func testMoveMultipleTabsToBeginning() {
    let firstTab = manager.addTestTab(isPrivate: false)
    let secondTab = manager.addTestTab(isPrivate: false)
    let thirdTab = manager.addTestTab(isPrivate: false)
    let fourthTab = manager.addTestTab(isPrivate: false)
    manager.selectTab(thirdTab)

    // Move third and fourth tabs to the beginning
    manager.moveTabs([thirdTab.id, fourthTab.id], toIndex: 0)

    let reorderedTabs = manager.tabs(isPrivate: false)
    XCTAssertEqual(
      [thirdTab, fourthTab, firstTab, secondTab].map(\.id),
      reorderedTabs.map(\.id),
      "Last two tabs should now be at the beginning"
    )
    XCTAssertEqual(manager.selectedTab?.id, thirdTab.id, "Third tab should still be selected")
  }

  func testMoveMultipleTabsToMiddle() {
    let firstTab = manager.addTestTab(isPrivate: false)
    let secondTab = manager.addTestTab(isPrivate: false)
    let thirdTab = manager.addTestTab(isPrivate: false)
    let fourthTab = manager.addTestTab(isPrivate: false)
    let fifthTab = manager.addTestTab(isPrivate: false)
    manager.selectTab(firstTab)

    // Move first and second tabs to index 2 (between third and fourth)
    manager.moveTabs([firstTab.id, secondTab.id], toIndex: 2)

    let reorderedTabs = manager.tabs(isPrivate: false)
    XCTAssertEqual(
      [thirdTab, fourthTab, firstTab, secondTab, fifthTab].map(\.id),
      reorderedTabs.map(\.id),
      "First two tabs should be inserted at index 2"
    )
    XCTAssertEqual(manager.selectedTab?.id, firstTab.id, "First tab should still be selected")
  }

  func testMoveTabsWithPrivateMixed() {
    setPersistentPrivateMode(true)

    let firstTab = manager.addTestTab(isPrivate: false)
    let privateTab = manager.addTestTab(isPrivate: true)
    let secondTab = manager.addTestTab(isPrivate: false)
    let thirdTab = manager.addTestTab(isPrivate: false)
    manager.selectTab(firstTab)

    // Move first tab to the end, should ignore private tab
    manager.moveTabs([firstTab.id], toIndex: 2)

    let regularTabs = manager.tabs(isPrivate: false)
    XCTAssertEqual(
      [secondTab, thirdTab, firstTab].map(\.id),
      regularTabs.map(\.id),
      "First tab should move to end of regular tabs"
    )

    // Private tab should remain in its original position in allTabs
    let allTabIds = manager.allTabs.map(\.id)
    XCTAssertTrue(allTabIds.contains(privateTab.id), "Private tab should still exist")
    XCTAssertEqual(manager.selectedTab?.id, firstTab.id, "First tab should still be selected")
  }

  func testMoveTabsOffByOneScenario() {
    // Test the specific scenario that was failing: moving tab from index 0 to index 2
    let firstTab = manager.addTestTab(isPrivate: false)
    let secondTab = manager.addTestTab(isPrivate: false)
    let thirdTab = manager.addTestTab(isPrivate: false)
    manager.selectTab(firstTab)

    // Move first tab to index 2 (should end up at the end)
    manager.moveTabs([firstTab.id], toIndex: 2)

    let reorderedTabs = manager.tabs(isPrivate: false)
    XCTAssertEqual(
      [secondTab, thirdTab, firstTab].map(\.id),
      reorderedTabs.map(\.id),
      "First tab should be moved to the end, not middle"
    )
    XCTAssertEqual(manager.selectedTab?.id, firstTab.id, "First tab should still be selected")
  }

  func testMoveTabsEmptyArray() {
    manager.addTestTab(isPrivate: false)
    manager.addTestTab(isPrivate: false)
    let originalOrder = manager.tabs(isPrivate: false).map(\.id)

    // Moving empty array should not change anything
    manager.moveTabs([], toIndex: 1)

    let newOrder = manager.tabs(isPrivate: false).map(\.id)
    XCTAssertEqual(originalOrder, newOrder, "Tab order should remain unchanged")
  }

  func testMoveTabsInvalidIndex() {
    let firstTab = manager.addTestTab(isPrivate: false)
    let secondTab = manager.addTestTab(isPrivate: false)

    // Move to index beyond array bounds should place at end
    manager.moveTabs([firstTab.id], toIndex: 100)

    let reorderedTabs = manager.tabs(isPrivate: false)
    XCTAssertEqual(
      [secondTab, firstTab].map(\.id),
      reorderedTabs.map(\.id),
      "Tab should be moved to the end when index is out of bounds"
    )
  }
}
