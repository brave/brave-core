// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import CoreData
import OrderedCollections
import Shared

// MARK: - HistoryV2FetchResultsDelegate

protocol HistoryV2FetchResultsDelegate: AnyObject {

  func controllerWillChangeContent(_ controller: HistoryV2FetchResultsController)

  func controllerDidChangeContent(_ controller: HistoryV2FetchResultsController)

  func controller(
    _ controller: HistoryV2FetchResultsController, didChange anObject: Any,
    at indexPath: IndexPath?, for type: NSFetchedResultsChangeType, newIndexPath: IndexPath?)

  func controller(
    _ controller: HistoryV2FetchResultsController, didChange sectionInfo: NSFetchedResultsSectionInfo,
    atSectionIndex sectionIndex: Int, for type: NSFetchedResultsChangeType)

  func controllerDidReloadContents(_ controller: HistoryV2FetchResultsController)
}

// MARK: - HistoryV2FetchResultsController

protocol HistoryV2FetchResultsController {

  var delegate: HistoryV2FetchResultsDelegate? { get set }

  var fetchedObjects: [HistoryNode]? { get }

  var fetchedObjectsCount: Int { get }

  var sectionCount: Int { get }

  func performFetch(withQuery: String, _ completion: @escaping () -> Void)

  func object(at indexPath: IndexPath) -> HistoryNode?

  func objectCount(for section: Int) -> Int

  func titleHeader(for section: Int) -> String

}

// MARK: - Historyv2Fetcher

class Historyv2Fetcher: NSObject, HistoryV2FetchResultsController {

  // MARK: Section

  enum Section: Int, CaseIterable {
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

  // MARK: Lifecycle

  init(historyAPI: BraveHistoryAPI) {
    self.historyAPI = historyAPI
    super.init()

    self.historyServiceListener = historyAPI.add(
      HistoryServiceStateObserver { [weak self] _ in
        guard let self = self else { return }

        DispatchQueue.main.async {
          self.delegate?.controllerDidReloadContents(self)
        }
      })
  }

  // MARK: Internal

  weak var delegate: HistoryV2FetchResultsDelegate?

  var fetchedObjects: [HistoryNode]? {
    historyList
  }

  var fetchedObjectsCount: Int {
    historyList.count
  }

  var sectionCount: Int {
    return sectionDetails.elements.filter { $0.value > 0 }.count
  }

  func performFetch(withQuery: String, _ completion: @escaping () -> Void) {
    clearHistoryData()

    historyAPI?.search(
      withQuery: withQuery, maxCount: 200,
      completion: { [weak self] historyNodeList in
        guard let self = self else { return }

        self.historyList = historyNodeList.map { [unowned self] historyItem in
          if let section = self.fetchHistoryTimePeriod(dateAdded: historyItem.dateAdded),
            let numOfItemInSection = self.sectionDetails[section] {
            self.sectionDetails.updateValue(numOfItemInSection + 1, forKey: section)
          }

          return historyItem
        }

        completion()
      })
  }

  func object(at indexPath: IndexPath) -> HistoryNode? {
    let filteredDetails = sectionDetails.elements.filter { $0.value > 0 }
    var totalItemIndex = 0

    for sectionIndex in 0..<indexPath.section {
      totalItemIndex += filteredDetails[safe: sectionIndex]?.value ?? 0
    }

    return fetchedObjects?[safe: totalItemIndex + indexPath.row]
  }

  func objectCount(for section: Int) -> Int {
    let filteredDetails = sectionDetails.elements.filter { $0.value > 0 }
    return filteredDetails[safe: section]?.value ?? 0
  }

  func titleHeader(for section: Int) -> String {
    let filteredDetails = sectionDetails.elements.filter { $0.value > 0 }
    return filteredDetails[safe: section]?.key.title ?? ""
  }

  // MARK: Private

  private var historyServiceListener: HistoryServiceListener?

  private weak var historyAPI: BraveHistoryAPI?

  private var historyList = [HistoryNode]()

  private var sectionDetails: OrderedDictionary<Section, Int> = [
    .today: 0,
    .yesterday: 0,
    .lastWeek: 0,
    .thisMonth: 0,
    .earlier: 0,
  ]

  private func fetchHistoryTimePeriod(dateAdded: Date?) -> Section? {
    let todayOffset = 0
    let yesterdayOffset = -1
    let thisWeekOffset = -7
    let thisMonthOffset = -31

    if dateAdded?.compare(getDate(todayOffset)) == ComparisonResult.orderedDescending {
      return .today
    } else if dateAdded?.compare(getDate(yesterdayOffset)) == ComparisonResult.orderedDescending {
      return .yesterday
    } else if dateAdded?.compare(getDate(thisWeekOffset)) == ComparisonResult.orderedDescending {
      return .lastWeek
    } else if dateAdded?.compare(getDate(thisMonthOffset)) == ComparisonResult.orderedDescending {
      return .thisMonth
    }

    return .earlier
  }

  private func getDate(_ dayOffset: Int) -> Date {
    let calendar = Calendar(identifier: Calendar.Identifier.gregorian)
    let nowComponents = calendar.dateComponents(
      [Calendar.Component.year, Calendar.Component.month, Calendar.Component.day], from: Date())

    guard let today = calendar.date(from: nowComponents) else {
      return Date()
    }

    return (calendar as NSCalendar).date(
      byAdding: NSCalendar.Unit.day, value: dayOffset, to: today, options: []) ?? Date()
  }

  private func clearHistoryData() {
    historyList.removeAll()

    for key in sectionDetails.keys {
      sectionDetails.updateValue(0, forKey: key)
    }
  }

}
