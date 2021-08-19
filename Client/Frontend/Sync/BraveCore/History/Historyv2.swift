// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Data
import BraveCore
import BraveShared
import CoreData
import Shared

private let log = Logger.browserLogger

// A Lightweight wrapper around BraveCore history
// with the same layout/interface as `History (from CoreData)`
class Historyv2: WebsitePresentable {
    
    /// Sections in History List to be displayed
    enum Section: Int, CaseIterable {
        /// History happened Today
        case today
        /// History happened Yesterday
        case yesterday
        /// History happened between yesterday and end of this week
        case lastWeek
        /// History happened between end of this week and end of this month
        case thisMonth
        
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
            }
        }
    }
    
    // MARK: Lifecycle
    
    init(with node: HistoryNode) {
        self.historyNode = node
    }
    
    // MARK: Internal
    
    public var url: String? {
        historyNode.url.absoluteString
    }
    
    public var title: String? {
        historyNode.title
    }
    
    public var created: Date? {
        get {
            return historyNode.dateAdded
        }
        
        set {
            historyNode.dateAdded = newValue ?? Date()
        }
    }
    
    public var domain: Domain? {
        return Domain.getOrCreate(forUrl: historyNode.url, persistent: !PrivateBrowsingManager.shared.isPrivateBrowsing)
    }
    
    public var sectionID: Section? {
        fetchHistoryTimePeriod(visited: created)
    }
    
    // MARK: Private
    
    private let historyNode: HistoryNode
    private static var observer: HistoryServiceListener?
    private static let historyAPI = (UIApplication.shared.delegate as? AppDelegate)?.braveCore?.historyAPI

    private func fetchHistoryTimePeriod(visited: Date?) -> Section? {
        let todayOffset = 0
        let yesterdayOffset = -1
        let thisWeekOffset = -7
        let thisMonthOffset = -31
        
        if created?.compare(getDate(todayOffset)) == ComparisonResult.orderedDescending {
            return .today
        } else if created?.compare(getDate(yesterdayOffset)) == ComparisonResult.orderedDescending {
            return .yesterday
        } else if created?.compare(getDate(thisWeekOffset)) == ComparisonResult.orderedDescending {
            return .lastWeek
        } else if created?.compare(getDate(thisMonthOffset))  == ComparisonResult.orderedDescending {
            return .thisMonth
        }
        
        return nil
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
}

// MARK: History Fetching

extension Historyv2 {

    public class func add(url: URL, title: String, dateAdded: Date, isURLTyped: Bool = true) {
        guard let historyAPI = Historyv2.historyAPI else {
            return
        }

        let historyNode = HistoryNode(url: url, title: title, dateAdded: dateAdded)
        historyAPI.addHistory(historyNode, isURLTyped: isURLTyped)
    }
    
    public static func frc() -> HistoryV2FetchResultsController? {
        guard let historyAPI = Historyv2.historyAPI else {
            return nil
        }
        
        return Historyv2Fetcher(historyAPI: historyAPI)
    }
    
    public func delete() {
        guard let historyAPI = Historyv2.historyAPI else {
            return
        }
        
        historyAPI.removeHistory(historyNode)
    }
    
    public class func deleteAll(_ completion: @escaping () -> Void) {
        guard let historyAPI = Historyv2.historyAPI else {
            return
        }
        
        historyAPI.removeAll {
            completion()
        }
    }
    
    public class func suffix(_ maxLength: Int, _ completion: @escaping ([Historyv2]) -> Void) {
        guard let historyAPI = Historyv2.historyAPI else {
            return
        }
        
        historyAPI.search(withQuery: nil, maxCount: UInt(max(20, maxLength)), completion: { historyResults in
            completion(historyResults.map { Historyv2(with: $0) })
        })
    }

    public static func byFrequency(query: String? = nil, _ completion: @escaping ([WebsitePresentable]) -> Void) {
        guard let query = query, !query.isEmpty,
              let historyAPI = Historyv2.historyAPI else {
            return
        }
        
        historyAPI.search(withQuery: query, maxCount: 200, completion: { historyResults in
            completion(historyResults.map { Historyv2(with: $0) })
        })
    }
    
    public func update(customTitle: String?, dateAdded: Date?) {
        if let title = customTitle {
            historyNode.title = title
        }
        
        if let date = dateAdded {
            historyNode.dateAdded = date
        }
    }
}

// MARK: Brave-Core Only

extension Historyv2 {
    
    public static func waitForHistoryServiceLoaded(_ completion: @escaping () -> Void) {
        guard let historyAPI = Historyv2.historyAPI else {
            return
        }
        
        if historyAPI.isBackendLoaded {
            DispatchQueue.main.async {
                completion()
            }
        } else {
            observer = historyAPI.add(HistoryServiceStateObserver({
                if case .serviceLoaded = $0 {
                    observer?.destroy()
                    observer = nil
                    
                    DispatchQueue.main.async {
                        completion()
                    }
                }
            }))
        }
    }
}
