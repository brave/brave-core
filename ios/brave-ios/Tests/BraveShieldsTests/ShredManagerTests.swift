// Copyright 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CoreData
import Preferences
import TestHelpers
import Web
import WebKit
import XCTest

@testable import BraveShields
@testable import Data

@MainActor
private final class FakeShredTabContainer: ShredTabContainer {
  let windowId = UUID()
  var isPrivateBrowsing: Bool
  var allTabs: [any TabState]
  var dataStore: WKWebsiteDataStore = .nonPersistent()
  private(set) var removeAllTabsCalls: [Bool] = []

  init(tabs: [any TabState], isPrivateBrowsing: Bool = false) {
    self.allTabs = tabs
    self.isPrivateBrowsing = isPrivateBrowsing
  }

  @discardableResult
  func removeTabsToShred(_ tabs: [any TabState]) -> [any TabState] {
    allTabs.removeAll { tab in tabs.contains { $0 === tab } }
    return tabs
  }

  func removeAllTabsToShred(isPrivate: Bool) {
    removeAllTabsCalls.append(isPrivate)
    removeTabsToShred(allTabs.filter { $0.isPrivate == isPrivate })
  }

  func websiteDataStore(isPrivate: Bool) -> WKWebsiteDataStore {
    dataStore
  }

  func contains(_ tab: any TabState) -> Bool {
    allTabs.contains { $0 === tab }
  }
}

@MainActor
class ShredManagerTests: CoreDataTestCase {
  private let profile = FakeProfile()
  /// The windows whose scene is connected, registered with the `ShredManager` on creation.
  private var containers: [FakeShredTabContainer] = []
  private var dormantWindowIds: [UUID] = []
  private lazy var dataStore: WKWebsiteDataStore = .nonPersistent()

  override func setUp() {
    super.setUp()
    // Keeps forgetting data from saving history removals, which would interfere with waiting on
    // the persisted tab deletions.
    Preferences.Shields.shredHistoryItems.value = false
  }

  override func tearDown() {
    Preferences.Shields.shredHistoryItems.reset()
    super.tearDown()
  }

  private func makeShredManager(siteClosedShredDelay: TimeInterval = 0) -> ShredManager {
    let shredManager = ShredManager(
      braveCore: nil,
      profile: profile,
      connectedWindowIds: { [unowned self] in containers.map(\.windowId) },
      dormantWindowIds: { [unowned self] in dormantWindowIds },
      isShredFeatureEnabled: true,
      siteClosedShredDelay: siteClosedShredDelay
    )
    for container in containers {
      container.dataStore = dataStore
      shredManager.register(container)
    }
    return shredManager
  }

  /// Sets the Auto Shred level of the site displayed in `tab`.
  private func setShredLevel(_ shredLevel: SiteShredLevel, for tab: FakeTabState) {
    let braveShieldsHelper = BraveShieldsTabHelper(
      tab: tab,
      braveShieldsSettings: nil,
      isBraveShieldsContentSettingsEnabled: false
    )
    tab.braveShieldsHelper = braveShieldsHelper
    braveShieldsHelper.setShredLevel(shredLevel, for: tab.visibleURL)
  }

  private func navigate(
    _ tab: FakeTabState,
    to urlString: String,
    shredManager: ShredManager
  ) {
    let url = URL(string: urlString)!
    shredManager.navigated(from: tab.visibleURL, to: url, in: tab)
    tab.visibleURL = url
  }

  private func makeTab(_ urlString: String, isPrivate: Bool = false) -> FakeTabState {
    let tab = FakeTabState()
    tab.profile = isPrivate ? profile.offTheRecordProfile : profile
    tab.visibleURL = URL(string: urlString)
    let configuration = WKWebViewConfiguration()
    configuration.websiteDataStore = dataStore
    tab.configuration = configuration
    return tab
  }

  private func setCookie(forDomain domain: String) async {
    let cookie = HTTPCookie(properties: [
      .domain: domain,
      .path: "/",
      .name: "shred",
      .value: "1",
      .secure: "TRUE",
      .expires: Date.now.addingTimeInterval(3600),
    ])!
    await dataStore.httpCookieStore.setCookie(cookie)
  }

  private func dataRecordDomains() async -> Set<String> {
    let records = await dataStore.dataRecords(ofTypes: WKWebsiteDataStore.allWebsiteDataTypes())
    return Set(records.map(\.displayName))
  }

  private func insertRecentlyClosed(_ urlString: String) {
    backgroundSaveAndWaitForExpectation {
      RecentlyClosed.insert(
        SavedRecentlyClosed(
          url: URL(string: urlString)!,
          title: urlString,
          dateAdded: .now,
          interactionState: nil,
          order: 0
        )
      )
    }
  }

  private func waitForCompletion(of task: Task<Void, Never>?) {
    let completed = expectation(description: "task completed")
    Task {
      await task?.value
      completed.fulfill()
    }
    wait(for: [completed], timeout: 5)
  }

  /// Fetches from a new context, as background deletions aren't merged into the view context.
  private func recentlyClosedURLs() -> Set<String> {
    let context = DataController.newBackgroundContext()
    return context.performAndWait {
      let request = NSFetchRequest<RecentlyClosed>(entityName: "RecentlyClosed")
      return Set(((try? context.fetch(request)) ?? []).map(\.url))
    }
  }

  // MARK: - shredData(for:in:) with a URL

  func testShredDataForURLClosesSiteTabsInAllWindows() {
    let tab = makeTab("https://brave.com")
    let subdomainTab = makeTab("https://search.brave.com/search?q=shred")
    let otherSiteTab = makeTab("https://example.com")
    let otherWindowTab = makeTab("https://account.brave.com")
    let otherWindowOtherSiteTab = makeTab("https://example.org")
    let window = FakeShredTabContainer(tabs: [tab, subdomainTab, otherSiteTab])
    let otherWindow = FakeShredTabContainer(tabs: [otherWindowTab, otherWindowOtherSiteTab])
    containers = [window, otherWindow]

    makeShredManager().shredData(for: tab.visibleURL!, in: tab)

    XCTAssertFalse(window.contains(tab))
    XCTAssertFalse(window.contains(subdomainTab))
    XCTAssertTrue(window.contains(otherSiteTab))
    XCTAssertFalse(otherWindow.contains(otherWindowTab))
    XCTAssertTrue(otherWindow.contains(otherWindowOtherSiteTab))
  }

  func testShredDataForURLOnlyClosesTabsInSameMode() {
    let privateTab = makeTab("https://brave.com", isPrivate: true)
    let regularTab = makeTab("https://brave.com")
    let otherWindowPrivateTab = makeTab("https://brave.com", isPrivate: true)
    let otherWindowRegularTab = makeTab("https://brave.com")
    let window = FakeShredTabContainer(tabs: [privateTab, regularTab], isPrivateBrowsing: true)
    let otherWindow = FakeShredTabContainer(tabs: [otherWindowPrivateTab, otherWindowRegularTab])
    containers = [window, otherWindow]

    makeShredManager().shredData(for: privateTab.visibleURL!, in: privateTab)

    XCTAssertFalse(window.contains(privateTab))
    XCTAssertTrue(window.contains(regularTab))
    XCTAssertFalse(otherWindow.contains(otherWindowPrivateTab))
    XCTAssertTrue(otherWindow.contains(otherWindowRegularTab))
  }

  func testShredDataForNonShreddableURLDoesNothing() {
    let tab = makeTab("about:blank")
    let window = FakeShredTabContainer(tabs: [tab])
    containers = [window]

    makeShredManager().shredData(for: tab.visibleURL!, in: tab)

    XCTAssertTrue(window.contains(tab))
  }

  func testShredDataIgnoresRegisteredWindowThatIsNotConnected() {
    let tab = makeTab("https://brave.com")
    let disconnectedWindowTab = makeTab("https://brave.com")
    let window = FakeShredTabContainer(tabs: [tab])
    let disconnectedWindow = FakeShredTabContainer(tabs: [disconnectedWindowTab])
    containers = [window, disconnectedWindow]
    let shredManager = makeShredManager()
    containers = [window]

    shredManager.shredData(for: tab.visibleURL!, in: tab)

    XCTAssertFalse(window.contains(tab))
    XCTAssertTrue(disconnectedWindow.contains(disconnectedWindowTab))
  }

  // MARK: - shredData(for:in:) with tabs

  func testShredDataForTabsReturnsTabsClosedInContainer() {
    let selectedTab = makeTab("https://brave.com")
    let sameURLTab = makeTab("https://brave.com")
    let otherSiteTab = makeTab("https://example.com")
    let otherWindowTab = makeTab("https://brave.com")
    let window = FakeShredTabContainer(tabs: [selectedTab, sameURLTab, otherSiteTab])
    let otherWindow = FakeShredTabContainer(tabs: [otherWindowTab])
    containers = [window, otherWindow]

    let closedTabIds = makeShredManager().shredData(for: [selectedTab], in: window)

    XCTAssertEqual(closedTabIds, [selectedTab.id, sameURLTab.id])
    XCTAssertTrue(window.contains(otherSiteTab))
    XCTAssertFalse(otherWindow.contains(otherWindowTab))
  }

  func testShredDataForTabsUsesContainerMode() {
    let privateTab = makeTab("https://brave.com", isPrivate: true)
    let regularTab = makeTab("https://brave.com")
    let window = FakeShredTabContainer(tabs: [privateTab, regularTab], isPrivateBrowsing: true)
    containers = [window]

    let closedTabIds = makeShredManager().shredData(for: [privateTab], in: window)

    XCTAssertEqual(closedTabIds, [privateTab.id])
    XCTAssertTrue(window.contains(regularTab))
  }

  // MARK: - shredAllTabs(in:)

  func testShredAllTabsClosesContainerAndMatchingSitesInOtherWindows() {
    let tab = makeTab("https://brave.com")
    let otherSiteTab = makeTab("https://example.com")
    let privateTab = makeTab("https://brave.com", isPrivate: true)
    let otherWindowTab = makeTab("https://search.brave.com")
    let otherWindowUnrelatedTab = makeTab("https://example.org")
    let otherWindowPrivateTab = makeTab("https://brave.com", isPrivate: true)
    let window = FakeShredTabContainer(tabs: [tab, otherSiteTab, privateTab])
    let otherWindow = FakeShredTabContainer(
      tabs: [otherWindowTab, otherWindowUnrelatedTab, otherWindowPrivateTab]
    )
    containers = [window, otherWindow]

    makeShredManager().shredAllTabs(in: window)

    XCTAssertEqual(window.removeAllTabsCalls, [false])
    XCTAssertEqual(otherWindow.removeAllTabsCalls, [])
    XCTAssertFalse(window.contains(tab))
    XCTAssertFalse(window.contains(otherSiteTab))
    XCTAssertTrue(window.contains(privateTab))
    XCTAssertFalse(otherWindow.contains(otherWindowTab))
    XCTAssertTrue(otherWindow.contains(otherWindowUnrelatedTab))
    XCTAssertTrue(otherWindow.contains(otherWindowPrivateTab))
  }

  // MARK: - Dormant windows

  func testShredDataDeletesPersistedTabsInDormantWindows() {
    let dormantWindowId = UUID()
    SessionWindow.createWindow(isSelected: false, uuid: dormantWindowId)
    backgroundSaveAndWaitForExpectation {
      SessionTab.createIfNeeded(
        windowId: dormantWindowId,
        tabId: UUID(),
        title: "Brave",
        tabURL: URL(string: "https://search.brave.com")!,
        isPrivate: false
      )
    }
    backgroundSaveAndWaitForExpectation {
      SessionTab.createIfNeeded(
        windowId: dormantWindowId,
        tabId: UUID(),
        title: "Example",
        tabURL: URL(string: "https://example.com")!,
        isPrivate: false
      )
    }
    dormantWindowIds = [dormantWindowId]

    let tab = makeTab("https://brave.com")
    containers = [FakeShredTabContainer(tabs: [tab])]

    backgroundSaveAndWaitForExpectation {
      makeShredManager().shredData(for: tab.visibleURL!, in: tab)
    }

    XCTAssertEqual(
      SessionTab.allURLs(isPrivate: false, inWindows: [dormantWindowId]),
      [URL(string: "https://example.com")!]
    )
  }

  // MARK: - Forgetting data

  func testShredDataForURLForgetsSiteData() async {
    await setCookie(forDomain: "brave.com")
    await setCookie(forDomain: "example.com")
    let tab = makeTab("https://search.brave.com")
    containers = [FakeShredTabContainer(tabs: [tab])]

    await makeShredManager().shredData(for: tab.visibleURL!, in: tab)?.value

    let domains = await dataRecordDomains()
    XCTAssertEqual(domains, ["example.com"])
  }

  func testShredAllTabsForgetsDataForEverySite() async {
    await setCookie(forDomain: "brave.com")
    await setCookie(forDomain: "example.com")
    await setCookie(forDomain: "wikipedia.org")
    let window = FakeShredTabContainer(tabs: [
      makeTab("https://brave.com"), makeTab("https://example.com"),
    ])
    containers = [window]

    await makeShredManager().shredAllTabs(in: window).value

    let domains = await dataRecordDomains()
    XCTAssertEqual(domains, ["wikipedia.org"])
  }

  func testShredDataRemovesRecentlyClosedWhenShreddingHistory() {
    Preferences.Shields.shredHistoryItems.value = true
    insertRecentlyClosed("https://search.brave.com")
    insertRecentlyClosed("https://example.com")
    let tab = makeTab("https://brave.com")
    containers = [FakeShredTabContainer(tabs: [tab])]

    makeShredManager().shredData(for: tab.visibleURL!, in: tab)

    let removed = expectation(
      for: NSPredicate { [unowned self] _, _ in
        MainActor.assumeIsolated { recentlyClosedURLs() == ["https://example.com"] }
      },
      evaluatedWith: nil
    )
    wait(for: [removed], timeout: 5)
  }

  func testShredDataKeepsRecentlyClosedWhenNotShreddingHistory() {
    insertRecentlyClosed("https://brave.com")
    let tab = makeTab("https://brave.com")
    containers = [FakeShredTabContainer(tabs: [tab])]

    waitForCompletion(of: makeShredManager().shredData(for: tab.visibleURL!, in: tab))

    XCTAssertEqual(recentlyClosedURLs(), ["https://brave.com"])
  }

  // MARK: - Shred when site tabs are closed

  func testNavigatingAwayShredsPreviousSiteWithoutClosingTab() async throws {
    await setCookie(forDomain: "brave.com")
    await setCookie(forDomain: "example.com")
    let tab = makeTab("https://brave.com")
    setShredLevel(.whenSiteClosed, for: tab)
    let window = FakeShredTabContainer(tabs: [tab])
    containers = [window]
    let shredManager = makeShredManager()

    navigate(tab, to: "https://example.com", shredManager: shredManager)

    let forgetTask = try XCTUnwrap(shredManager.forgetTasks[false]?["brave.com"])
    try await forgetTask.value
    let domains = await dataRecordDomains()
    XCTAssertEqual(domains, ["example.com"])
    XCTAssertTrue(window.contains(tab))
    XCTAssertEqual(tab.deleteWebViewCallCount, 0)
    XCTAssertNil(shredManager.forgetTasks[false]?["brave.com"])
  }

  func testNavigatingWithinSiteDoesNotShred() {
    let tab = makeTab("https://brave.com")
    setShredLevel(.whenSiteClosed, for: tab)
    containers = [FakeShredTabContainer(tabs: [tab])]
    let shredManager = makeShredManager()

    navigate(tab, to: "https://search.brave.com", shredManager: shredManager)

    XCTAssertNil(shredManager.forgetTasks[false]?["brave.com"])
  }

  func testNavigatingAwayDoesNotShredSiteOpenInAnotherWindow() {
    let tab = makeTab("https://brave.com")
    setShredLevel(.whenSiteClosed, for: tab)
    containers = [
      FakeShredTabContainer(tabs: [tab]),
      FakeShredTabContainer(tabs: [makeTab("https://search.brave.com")]),
    ]
    let shredManager = makeShredManager()

    navigate(tab, to: "https://example.com", shredManager: shredManager)

    XCTAssertNil(shredManager.forgetTasks[false]?["brave.com"])
  }

  func testNavigatingAwayShredsSiteOpenOnlyInAnotherMode() {
    let tab = makeTab("https://brave.com")
    setShredLevel(.whenSiteClosed, for: tab)
    containers = [
      FakeShredTabContainer(tabs: [tab, makeTab("https://brave.com", isPrivate: true)])
    ]
    let shredManager = makeShredManager()

    navigate(tab, to: "https://example.com", shredManager: shredManager)

    XCTAssertNotNil(shredManager.forgetTasks[false]?["brave.com"])
    XCTAssertNil(shredManager.forgetTasks[true]?["brave.com"])
  }

  func testNavigatingAwayDoesNotShredSiteNotSetToShredWhenClosed() {
    let tab = makeTab("https://brave.com")
    setShredLevel(.appExit, for: tab)
    containers = [FakeShredTabContainer(tabs: [tab])]
    let shredManager = makeShredManager()

    navigate(tab, to: "https://example.com", shredManager: shredManager)

    XCTAssertNil(shredManager.forgetTasks[false]?["brave.com"])
  }

  func testNavigatingBackToSiteCancelsShred() async throws {
    await setCookie(forDomain: "brave.com")
    let tab = makeTab("https://brave.com")
    setShredLevel(.whenSiteClosed, for: tab)
    containers = [FakeShredTabContainer(tabs: [tab])]
    let shredManager = makeShredManager(siteClosedShredDelay: 60)

    navigate(tab, to: "https://example.com", shredManager: shredManager)
    let forgetTask = try XCTUnwrap(shredManager.forgetTasks[false]?["brave.com"])
    navigate(tab, to: "https://brave.com", shredManager: shredManager)

    XCTAssertTrue(forgetTask.isCancelled)
    XCTAssertNil(shredManager.forgetTasks[false]?["brave.com"])
    let domains = await dataRecordDomains()
    XCTAssertEqual(domains, ["brave.com"])
  }

  func testNavigatingBackToSiteInAnotherWindowCancelsShred() throws {
    let tab = makeTab("https://brave.com")
    setShredLevel(.whenSiteClosed, for: tab)
    let otherWindowTab = makeTab("https://example.com")
    containers = [
      FakeShredTabContainer(tabs: [tab]),
      FakeShredTabContainer(tabs: [otherWindowTab]),
    ]
    let shredManager = makeShredManager(siteClosedShredDelay: 60)

    navigate(tab, to: "https://example.com", shredManager: shredManager)
    let forgetTask = try XCTUnwrap(shredManager.forgetTasks[false]?["brave.com"])
    navigate(otherWindowTab, to: "https://brave.com", shredManager: shredManager)

    XCTAssertTrue(forgetTask.isCancelled)
  }

  func testClosingLastSiteTabShredsAndDeletesWebView() async throws {
    await setCookie(forDomain: "brave.com")
    let tab = makeTab("https://brave.com")
    setShredLevel(.whenSiteClosed, for: tab)
    let window = FakeShredTabContainer(tabs: [tab])
    containers = [window]
    let shredManager = makeShredManager()

    window.removeTabsToShred([tab])
    shredManager.siteClosed(tab.visibleURL!, in: tab)

    let forgetTask = try XCTUnwrap(shredManager.forgetTasks[false]?["brave.com"])
    try await forgetTask.value
    let domains = await dataRecordDomains()
    XCTAssertEqual(domains, [])
    XCTAssertEqual(tab.deleteWebViewCallCount, 1)
  }
}
