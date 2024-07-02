// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import Data
import DesignSystem
import LocalAuthentication
import OrderedCollections
import Preferences
import ScreenTime

enum HistoryItemSelection {
  case selectTab
  case openInNewTab
  case openInNewPrivateTab
  case copyLink
  case shareLink
}

enum HistorySection: Int, CaseIterable {
  /// History happened Today
  case today
  /// History happened Yesterday
  case yesterday
  /// History happened between yesterday and end of this week
  case lastWeek
  /// History happened between end of this week and end of this month
  case thisMonth
  /// History happened after the end of this month
  case earlier

  /// The list of titles time period
  var title: String {
    switch self {
    case .today:
      return Strings.today
    case .yesterday:
      return Strings.yesterday
    case .lastWeek:
      return Strings.lastWeek
    case .thisMonth:
      return Strings.lastMonth
    case .earlier:
      return Strings.earlier
    }
  }
}

class HistoryModel: NSObject, ObservableObject {
  private let api: BraveHistoryAPI?
  private weak var tabManager: TabManager?
  private weak var toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?
  private var dismiss: () -> Void
  private var askForLocalAuthentication: (AuthViewType, ((Bool, LAError.Code?) -> Void)?) -> Void

  private var listener: HistoryServiceListener?
  private let maxFetchCount: UInt = 200
  private var currentSearchQuery: String?

  @Published
  var isHistoryServiceLoaded = false

  @Published
  var isPrivateBrowsing = false

  @Published
  var items: OrderedDictionary<HistorySection, [HistoryNode]> = [:]

  private var sectionDetails: OrderedDictionary<HistorySection, [HistoryNode]> = [
    .today: [],
    .yesterday: [],
    .lastWeek: [],
    .thisMonth: [],
    .earlier: [],
  ]

  init(
    api: BraveHistoryAPI?,
    tabManager: TabManager?,
    toolbarUrlActionsDelegate: ToolbarUrlActionsDelegate?,
    dismiss: @escaping () -> Void,
    askForAuthentication: @escaping (AuthViewType, ((Bool, LAError.Code?) -> Void)?) -> Void
  ) {
    self.api = api
    self.tabManager = tabManager
    self.toolbarUrlActionsDelegate = toolbarUrlActionsDelegate
    self.dismiss = dismiss
    self.askForLocalAuthentication = askForAuthentication
    self.isPrivateBrowsing = tabManager?.privateBrowsingManager.isPrivateBrowsing == true
    super.init()

    if api?.isBackendLoaded == true {
      isHistoryServiceLoaded = true
      refreshHistory()
    }

    listener = api?.add(
      HistoryStateObserver { [weak self] event in
        guard let self = self else { return }
        if case .modelLoaded = event {
          isHistoryServiceLoaded = true
        }

        refreshHistory()
      }
    )
  }

  deinit {
    listener?.destroy()
  }

  func refreshHistory() {
    refreshHistory(query: currentSearchQuery)
  }

  func refreshHistory(query: String? = nil) {
    currentSearchQuery = query

    Task { @MainActor in
      for key in sectionDetails.keys {
        sectionDetails.updateValue([], forKey: key)
      }

      let options = HistorySearchOptions(
        maxCount: maxFetchCount,
        duplicateHandling: (query ?? "").isEmpty ? .removePerDay : .removeAll
      )

      await api?.search(withQuery: query, options: options).forEach {
        [weak self] historyItem in
        guard let self = self else {
          return
        }

        guard let section = self.fetchHistoryTimePeriod(dateAdded: historyItem.dateAdded) else {
          return
        }

        guard var items = self.sectionDetails[section] else {
          return
        }

        // Update section mapping with the new item
        items.append(historyItem)
        self.sectionDetails.updateValue(items, forKey: section)
      }

      // Filter empty sections
      items = self.sectionDetails.filter { !$1.isEmpty }
    }
  }

  func delete(nodes: [HistoryNode]) {
    api?.removeHistory(for: nodes)

    // Removing a history item should remove its corresponded Recently Closed Item
    for node in nodes {
      RecentlyClosed.remove(with: node.url.absoluteString)
    }

    do {
      let screenTimeHistory = try STWebHistory(bundleIdentifier: Bundle.main.bundleIdentifier!)
      for node in nodes {
        screenTimeHistory.deleteHistory(for: node.url)
      }
    } catch {
      assertionFailure("STWebHistory could not be initialized: \(error)")
    }
  }

  func deleteAll() {
    api?.deleteAll { [weak self] in
      guard let self = self else { return }

      // Clearing Tab History with entire history entry
      self.tabManager?.clearTabHistory { [weak self] in
        self?.refreshHistory()
      }

      // Clearing History should clear Recently Closed
      RecentlyClosed.removeAll()

      // Donate Clear Browser History for suggestions
      let clearBrowserHistoryActivity = ActivityShortcutManager.shared.createShortcutActivity(
        type: .clearBrowsingHistory
      )

      clearBrowserHistoryActivity.becomeCurrent()
    }

    // Asking Sync Engine To Remove Visits
    api?.removeAll { [weak self] in
      self?.refreshHistory()
    }
  }

  func handleHistoryItemSelection(_ selection: HistoryItemSelection, node: HistoryNode) {
    switch selection {
    case .selectTab:
      // Donate Custom Intent Open Website
      if node.url.isSecureWebPage(), !isPrivateBrowsing {
        ActivityShortcutManager.shared.donateCustomIntent(
          for: .openHistory,
          with: node.url.absoluteString
        )
      }

      dismiss()
      toolbarUrlActionsDelegate?.select(url: node.url, isUserDefinedURLNavigation: false)
    case .openInNewTab:
      dismiss()
      toolbarUrlActionsDelegate?.openInNewTab(node.url, isPrivate: false)
    case .openInNewPrivateTab:
      if !isPrivateBrowsing, Preferences.Privacy.privateBrowsingLock.value {
        askForLocalAuthentication(.general) { [weak self] success, error in
          guard let self = self else { return }

          if success {
            self.dismiss()
            self.toolbarUrlActionsDelegate?.openInNewTab(node.url, isPrivate: true)
          }
        }
      } else {
        dismiss()
        toolbarUrlActionsDelegate?.openInNewTab(node.url, isPrivate: true)
      }
    case .copyLink:
      toolbarUrlActionsDelegate?.copy(node.url)
    case .shareLink:
      toolbarUrlActionsDelegate?.share(node.url)
    }
  }

  private func fetchHistoryTimePeriod(dateAdded: Date?) -> HistorySection? {
    let todayOffset = 0
    let yesterdayOffset = -1
    let thisWeekOffset = -7
    let thisMonthOffset = -31

    if dateAdded?.compare(getDate(todayOffset)) == .orderedDescending {
      return .today
    } else if dateAdded?.compare(getDate(yesterdayOffset)) == .orderedDescending {
      return .yesterday
    } else if dateAdded?.compare(getDate(thisWeekOffset)) == .orderedDescending {
      return .lastWeek
    } else if dateAdded?.compare(getDate(thisMonthOffset)) == .orderedDescending {
      return .thisMonth
    }

    return .earlier
  }

  private func getDate(_ dayOffset: Int) -> Date {
    let calendar = Calendar(identifier: Calendar.Identifier.gregorian)
    let nowComponents = calendar.dateComponents(
      [Calendar.Component.year, Calendar.Component.month, Calendar.Component.day],
      from: Date()
    )

    guard let today = calendar.date(from: nowComponents) else {
      return Date()
    }

    return (calendar as NSCalendar).date(
      byAdding: NSCalendar.Unit.day,
      value: dayOffset,
      to: today,
      options: []
    ) ?? Date()
  }
}

private class HistoryStateObserver: NSObject, HistoryServiceObserver {
  private let listener: (StateChange) -> Void

  enum StateChange {
    case modelLoaded
    case nodeVisited(HistoryNode)
    case nodesModified([HistoryNode])
    case nodesDeleted(_ nodes: [HistoryNode], _ isAllHistory: Bool)
  }

  init(_ listener: @escaping (StateChange) -> Void) {
    self.listener = listener
  }

  func historyServiceLoaded() {
    self.listener(.modelLoaded)
  }

  func historyNodeVisited(_ historyNode: HistoryNode) {
    self.listener(.nodeVisited(historyNode))
  }

  func historyNodesModified(_ historyNodeList: [HistoryNode]) {
    self.listener(.nodesModified(historyNodeList))
  }

  func historyNodesDeleted(_ historyNodeList: [HistoryNode], isAllHistory: Bool) {
    self.listener(.nodesDeleted(historyNodeList, isAllHistory))
  }
}
