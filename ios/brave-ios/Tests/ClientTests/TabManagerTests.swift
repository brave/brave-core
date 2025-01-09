// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import Shared
import Storage
import UIKit
import WebKit
import XCTest

@testable import Brave
@testable import Data

open class MockTabManagerStateDelegate: TabManagerStateDelegate {
  var numberOfTabsStored = 0
  public func tabManagerWillStoreTabs(_ tabs: [Tab]) {
    numberOfTabsStored = tabs.count
  }
}

extension TabManager {
  func tabs(withType type: TabType) -> [Tab] {
    assert(Thread.isMainThread)
    return allTabs.filter { $0.type == type }
  }
}

struct MethodSpy {
  let functionName: String
  let method: ((_ tabs: [Tab?]) -> Void)?

  init(functionName: String) {
    self.functionName = functionName
    self.method = nil
  }

  init(functionName: String, method: ((_ tabs: [Tab?]) -> Void)?) {
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

  func testDelegateMethodWithName(_ name: String, tabs: [Tab?]) {
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
    didSelectedTabChange selected: Tab?,
    previous: Tab?
  ) {
    testDelegateMethodWithName(#function, tabs: [selected, previous])
  }

  public func tabManager(_ tabManager: TabManager, didAddTab tab: Tab) {
    testDelegateMethodWithName(#function, tabs: [tab])
  }

  public func tabManager(_ tabManager: TabManager, didRemoveTab tab: Tab) {
    testDelegateMethodWithName(#function, tabs: [tab])
  }

  public func tabManagerDidRestoreTabs(_ tabManager: TabManager) {
    testDelegateMethodWithName(#function, tabs: [])
  }

  public func tabManager(_ tabManager: TabManager, willRemoveTab tab: Tab) {
    testDelegateMethodWithName(#function, tabs: [tab])
  }

  public func tabManager(_ tabManager: TabManager, willAddTab tab: Tab) {
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
    BraveCoreMain.initializeResourceBundleForTesting()
  }

  override func setUp() {
    super.setUp()

    DataController.shared.initializeOnce()
    let profile = MockProfile()
    manager = TabManager(
      windowId: testWindowId,
      prefs: profile.prefs,
      rewards: nil,
      tabGeneratorAPI: nil,
      historyAPI: nil,
      privateBrowsingManager: privateBrowsingManager
    )
    privateBrowsingManager.isPrivateBrowsing = false
  }

  override func tearDown() {
    privateBrowsingManager.isPrivateBrowsing = false
    super.tearDown()
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

    // test that non-private tabs are saved to the db
    // add some non-private tabs to the tab manager
    for _ in 0..<3 {
      let tab = Tab(configuration: configuration, type: .private)
      tab.url = URL(string: "http://yahoo.com")!
      manager.configureTab(
        tab,
        request: URLRequest(url: tab.url!),
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
    manager.addTab(isPrivate: false)
    delegate.verify("Not all delegate methods were called")
  }

  func testDidDeleteLastTab() {
    let delegate = MockTabManagerDelegate()

    //create the tab before adding the mock delegate. So we don't have to check delegate calls we dont care about
    let tab = manager.addTab(isPrivate: false)
    manager.selectTab(tab)
    manager.addDelegate(delegate)

    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      let next = tabs[0]!
      let previous = tabs[1]!
      XCTAssertTrue(previous != next)
      XCTAssertTrue(previous == tab)
      XCTAssertFalse(next.isPrivate)
    }
    delegate.expect([willRemove, didRemove, willAdd, didAdd, didSelect])
    manager.removeTab(tab)
    delegate.verify("Not all delegate methods were called")
  }

  func testDidDeleteLastPrivateTab() {
    let delegate = MockTabManagerDelegate()

    //create the tab before adding the mock delegate. So we don't have to check delegate calls we dont care about
    let tab = manager.addTab(isPrivate: true)
    manager.selectTab(tab)
    manager.addDelegate(delegate)

    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      let next = tabs[0]!
      let previous = tabs[1]!
      XCTAssertTrue(previous != next)
      XCTAssertTrue(previous == tab)
      XCTAssertTrue(next.isPrivate)
    }
    delegate.expect([willRemove, didRemove, willAdd, didAdd, didSelect])
    manager.removeTab(tab)
    delegate.verify("Not all delegate methods were called")
  }

  func testDeletePrivateTabsPersistenceOnExit() {
    setPersistentPrivateMode(true)

    // create one private and one normal tab
    let tab = manager.addTab(isPrivate: false)
    manager.selectTab(tab)
    manager.selectTab(manager.addTab(isPrivate: true))

    XCTAssertEqual(
      TabType.of(manager.selectedTab).isPrivate,
      true,
      "The selected tab should be the private tab"
    )
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
      1,
      "There should only be one private tab"
    )

    manager.selectTab(tab)
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
      1,
      "If the normal tab is selected the private tab should NOT be deleted"
    )
    XCTAssertEqual(
      manager.tabs(withType: .regular).count,
      1,
      "The regular tab should stil be around"
    )

    manager.selectTab(manager.addTab(isPrivate: true))
    XCTAssertEqual(manager.tabs(withType: .private).count, 2, "There should be two private tabs")
    manager.willSwitchTabMode(leavingPBM: true)
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
      2,
      "After willSwitchTabMode there should be 2 private tabs"
    )

    manager.selectTab(manager.addTab(isPrivate: true))
    manager.selectTab(manager.addTab(isPrivate: true))
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
      4,
      "Private tabs should not be deleted when another one is added"
    )
    manager.selectTab(manager.addTab(isPrivate: false))
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
      4,
      "But once we add a normal tab we've switched out of private mode. Private tabs should be be persistent"
    )
    XCTAssertEqual(
      manager.tabs(withType: .regular).count,
      2,
      "The original normal tab and the new one should both still exist"
    )

    setPersistentPrivateMode(false)
  }

  func testDeletePrivateTabsOnNonPersistenceExit() {
    setPersistentPrivateMode(false)

    // create one private and one normal tab
    let tab = manager.addTab(isPrivate: false)
    manager.selectTab(tab)
    manager.selectTab(manager.addTab(isPrivate: true))

    XCTAssertEqual(
      TabType.of(manager.selectedTab).isPrivate,
      true,
      "The selected tab should be the private tab"
    )
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
      1,
      "There should only be one private tab"
    )

    manager.selectTab(tab)
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
      0,
      "If the normal tab is selected the private tab should have been deleted"
    )
    XCTAssertEqual(
      manager.tabs(withType: .regular).count,
      1,
      "The regular tab should stil be around"
    )

    manager.selectTab(manager.addTab(isPrivate: true))
    XCTAssertEqual(manager.tabs(withType: .private).count, 1, "There should be one new private tab")
    manager.willSwitchTabMode(leavingPBM: true)
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
      0,
      "After willSwitchTabMode there should be no more private tabs"
    )

    manager.selectTab(manager.addTab(isPrivate: true))
    manager.selectTab(manager.addTab(isPrivate: true))
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
      2,
      "Private tabs should not be deleted when another one is added"
    )
    manager.selectTab(manager.addTab(isPrivate: false))
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
      0,
      "But once we add a normal tab we've switched out of private mode. Private tabs should be deleted"
    )
    XCTAssertEqual(
      manager.tabs(withType: .regular).count,
      2,
      "The original normal tab and the new one should both still exist"
    )
  }

  func testTogglePBMDeletePersistent() {
    setPersistentPrivateMode(true)

    let tab = manager.addTab(isPrivate: false)
    manager.selectTab(tab)
    manager.selectTab(manager.addTab(isPrivate: false))
    manager.selectTab(manager.addTab(isPrivate: true))

    manager.willSwitchTabMode(leavingPBM: false)
    XCTAssertEqual(manager.tabs(withType: .private).count, 1, "There should be 1 private tab")
    manager.willSwitchTabMode(leavingPBM: true)
    XCTAssertEqual(manager.tabs(withType: .private).count, 1, "There should be 1 private tab")
    manager.removeTab(tab)
    XCTAssertEqual(manager.tabs(withType: .regular).count, 1, "There should be 1 normal tab")

    setPersistentPrivateMode(false)
  }

  func testTogglePBMDeleteNonPersistent() {
    setPersistentPrivateMode(false)

    let tab = manager.addTab(isPrivate: false)
    manager.selectTab(tab)
    manager.selectTab(manager.addTab(isPrivate: false))
    manager.selectTab(manager.addTab(isPrivate: true))

    manager.willSwitchTabMode(leavingPBM: false)
    XCTAssertEqual(manager.tabs(withType: .private).count, 1, "There should be 1 private tab")
    manager.willSwitchTabMode(leavingPBM: true)
    XCTAssertEqual(manager.tabs(withType: .private).count, 0, "There should be 0 private tab")
    manager.removeTab(tab)
    XCTAssertEqual(manager.tabs(withType: .regular).count, 1, "There should be 1 normal tab")
  }

  func testDeleteNonSelectedTab() {
    let delegate = MockTabManagerDelegate()

    //create the tab before adding the mock delegate. So we don't have to check delegate calls we dont care about
    let tab = manager.addTab(isPrivate: false)
    manager.selectTab(tab)
    manager.addTab(isPrivate: false)
    let deleteTab = manager.addTab(isPrivate: false)
    manager.addDelegate(delegate)

    delegate.expect([willRemove, didRemove])
    manager.removeTab(deleteTab)

    delegate.verify("Not all delegate methods were called")
  }

  func testDeleteSelectedTab() {
    let delegate = MockTabManagerDelegate()

    func addTab(_ visit: Bool) -> Tab {
      let tab = manager.addTab(isPrivate: false)
      if visit {
        tab.lastExecutedTime = Date.now()
      }
      return tab
    }

    let tab0 = addTab(false)  // not visited
    let tab1 = addTab(true)
    let tab2 = addTab(true)
    let tab3 = addTab(true)
    let tab4 = addTab(false)  // not visited

    // starting at tab1, we should be selecting
    // [ tab3, tab4, tab2, tab0 ]

    manager.selectTab(tab1)
    tab1.parent = tab3
    manager.removeTab(manager.selectedTab!)
    // Rule: parent tab if it was the most recently visited
    XCTAssertEqual(manager.selectedTab, tab3)

    manager.removeTab(manager.selectedTab!)
    // Rule: next to the right.
    XCTAssertEqual(manager.selectedTab, tab4)

    manager.removeTab(manager.selectedTab!)
    // Rule: next to the left, when none to the right
    XCTAssertEqual(manager.selectedTab, tab2)

    manager.removeTab(manager.selectedTab!)
    // Rule: last one left.
    XCTAssertEqual(manager.selectedTab, tab0)
  }

  func testDeleteLastTab() {
    let delegate = MockTabManagerDelegate()

    //create the tab before adding the mock delegate. So we don't have to check delegate calls we dont care about
    (0..<10).forEach { _ in manager.addTab(isPrivate: false) }
    manager.selectTab(manager.allTabs.last)
    let deleteTab = manager.allTabs.last
    let newSelectedTab = manager.allTabs[8]
    manager.addDelegate(delegate)

    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      let next = tabs[0]!
      let previous = tabs[1]!
      XCTAssertEqual(deleteTab, previous)
      XCTAssertEqual(next, newSelectedTab)
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
    let tab = manager.addTab(isPrivate: false)
    let newTab = manager.addTab(isPrivate: false)
    manager.selectTab(tab)
    manager.selectTab(manager.addTab(isPrivate: true))
    manager.addDelegate(delegate)

    // Double check a few things
    XCTAssertEqual(
      TabType.of(manager.selectedTab).isPrivate,
      true,
      "The selected tab should be the private tab"
    )
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
      1,
      "There should only be one private tab"
    )

    // switch to normal mode. Which should not delete the private tabs
    manager.willSwitchTabMode(leavingPBM: true)

    //make sure tabs are NOT cleared properly and indexes are reset
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
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

    setPersistentPrivateMode(false)
  }

  func testDelegatesCalledWhenRemovingPrivateTabsNonPersistence() {
    setPersistentPrivateMode(false)

    //setup
    let delegate = MockTabManagerDelegate()

    // create one private and one normal tab
    let tab = manager.addTab(isPrivate: false)
    let newTab = manager.addTab(isPrivate: false)
    manager.selectTab(tab)
    manager.selectTab(manager.addTab(isPrivate: true))
    manager.addDelegate(delegate)

    // Double check a few things
    XCTAssertEqual(
      TabType.of(manager.selectedTab).isPrivate,
      true,
      "The selected tab should be the private tab"
    )
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
      1,
      "There should only be one private tab"
    )

    // switch to normal mode. Which should delete the private tabs
    manager.willSwitchTabMode(leavingPBM: true)

    //make sure tabs are cleared properly and indexes are reset
    XCTAssertEqual(
      manager.tabs(withType: .private).count,
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
    (0..<10).forEach { _ in manager.addTab(isPrivate: false) }
    manager.selectTab(manager.allTabs.first)
    let deleteTab = manager.allTabs.first
    let newSelectedTab = manager.allTabs[1]
    manager.addDelegate(delegate)

    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      let next = tabs[0]!
      let previous = tabs[1]!
      XCTAssertEqual(deleteTab, previous)
      XCTAssertEqual(next, newSelectedTab)
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
    manager.addTab(isPrivate: false)
    let newSelected = manager.addTab(isPrivate: false)
    manager.addTab(isPrivate: true)
    let deleted = manager.addTab(isPrivate: false)
    manager.selectTab(manager.allTabs.last)
    manager.addDelegate(delegate)

    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      let next = tabs[0]!
      let previous = tabs[1]!
      XCTAssertEqual(deleted, previous)
      XCTAssertEqual(next, newSelected)
    }
    delegate.expect([willRemove, didRemove, didSelect])
    manager.removeTab(manager.allTabs.last!)

    delegate.verify("Not all delegate methods were called")
  }

  func testTabsIndexClosingFirst() {
    let delegate = MockTabManagerDelegate()

    // We add 2 tabs. Then a private one before adding another normal tab and selecting the first.
    // Make sure that when the last one is deleted we dont switch to the private tab
    let deleted = manager.addTab(isPrivate: false)
    let newSelected = manager.addTab(isPrivate: false)
    manager.addTab(isPrivate: true)
    manager.addTab(isPrivate: false)
    manager.selectTab(manager.allTabs.first)
    manager.addDelegate(delegate)

    let didSelect = MethodSpy(functionName: "tabManager(_:didSelectedTabChange:previous:)") {
      tabs in
      let next = tabs[0]!
      let previous = tabs[1]!
      XCTAssertEqual(deleted, previous)
      XCTAssertEqual(next, newSelected)
    }
    delegate.expect([willRemove, didRemove, didSelect])
    manager.removeTab(manager.allTabs.first!)
    delegate.verify("Not all delegate methods were called")
  }

  func testRemoveOnlyTab() {
    let delegate = MockTabManagerDelegate()

    let tab = manager.addTab(isPrivate: false)

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
    (0..<10).forEach { _ in manager.addTab(isPrivate: false) }
    manager.removeAll()

    XCTAssertFalse(manager.allTabs.isEmpty, "Should create a new tab when all others are removed")
    XCTAssertFalse(manager.allTabs.first!.isPrivate, "The new tab should be a regular tab")
  }

  func testRemoveAllOtherTabs() {
    (0..<10).forEach { index in manager.addTab(isPrivate: false) }
    manager.removeAllForCurrentMode(isActiveTabIncluded: false)

    XCTAssertFalse(
      manager.allTabs.count == 1,
      "The active tab should not be removed and no new tab should be created"
    )
    XCTAssertFalse(
      manager.allTabs.first!.isPrivate,
      "The last remaining tab should be a regular tab"
    )
  }

  func testMoveTabToEnd() {
    let firstTab = manager.addTabAndSelect(isPrivate: false)
    let secondTab = manager.addTab(isPrivate: false)
    manager.addTab(isPrivate: true)
    let thirdTab = manager.addTab(isPrivate: false)

    manager.moveTab(firstTab, toIndex: manager.tabs(withType: .regular).count - 1)

    let reorderedTabs = manager.tabs(withType: .regular)
    XCTAssertEqual(firstTab, reorderedTabs.last, "First tab should now be the last tab")
    XCTAssertEqual(manager.selectedTab, firstTab, "First tab should still be selected")
    XCTAssertEqual(
      [secondTab, thirdTab, firstTab],
      reorderedTabs,
      "Tabs should shift and the have the correct order ignoring the private tab"
    )
  }

  func testMoveTabToStart() {
    let firstTab = manager.addTabAndSelect(isPrivate: false)
    let secondTab = manager.addTab(isPrivate: false)
    manager.addTab(isPrivate: true)
    let thirdTab = manager.addTab(isPrivate: false)

    manager.moveTab(thirdTab, toIndex: 0)

    let reorderedTabs = manager.tabs(withType: .regular)
    XCTAssertEqual(thirdTab, reorderedTabs.first, "Last tab should now be the first tab")
    XCTAssertEqual(manager.selectedTab, firstTab, "First tab should still be selected")
    XCTAssertEqual(
      [thirdTab, firstTab, secondTab],
      reorderedTabs,
      "Tabs should shift and the have the correct order ignoring the private tab"
    )
  }

  func testMoveTabToMiddle() {
    let firstTab = manager.addTab(isPrivate: false)
    let secondTab = manager.addTab(isPrivate: false)
    manager.addTab(isPrivate: true)
    let thirdTab = manager.addTabAndSelect(isPrivate: false)
    let forthTab = manager.addTab(isPrivate: false)

    manager.moveTab(forthTab, toIndex: 1)

    let reorderedTabs = manager.tabs(withType: .regular)
    XCTAssertEqual(forthTab, reorderedTabs[1], "Last tab should now be at index 1")
    XCTAssertEqual(manager.selectedTab, thirdTab, "Third tab should still be selected")
    XCTAssertEqual(
      [firstTab, forthTab, secondTab, thirdTab],
      reorderedTabs,
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
    SessionWindow.createWindow(isPrivate: false, isSelected: true, uuid: testWindowId)
    wait(for: [windowCreateExpectation], timeout: 5)

    delegate.expect([willAdd, didAdd])
    let tabAddExpectation = expectation(
      forNotification: .NSManagedObjectContextDidSave,
      object: nil
    )
    let tab = manager.addTab(isPrivate: false)
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
    manager.addTab(isPrivate: true)
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
    SessionWindow.createWindow(isPrivate: false, isSelected: true, uuid: testWindowId)
    wait(for: [windowCreateExpectation], timeout: 5)

    delegate.expect([willAdd, didAdd, willAdd, didAdd])
    manager.addTab(isPrivate: true)
    let tabAddExpectation = expectation(
      forNotification: .NSManagedObjectContextDidSave,
      object: nil
    )
    let tab = manager.addTab(isPrivate: false)
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
}
