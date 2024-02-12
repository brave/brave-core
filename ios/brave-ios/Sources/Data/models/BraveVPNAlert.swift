// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared
import Network
import os.log
import Preferences

/// Stores the alerts data we receive from Brave VPN.
/// The alert is a resource blocked by the VPN service
public final class BraveVPNAlert: NSManagedObject, CRUD, Identifiable {

  /// Currently unused. This is persisted in case we need it in the future.
  public enum Action: Int {
    case drop
    case log
  }

  public enum TrackerType: Int {
    /// A request asking for users location
    case location
    /// A regular tracker
    case app
    /// An email tracker
    case mail
  }

  /// Currently unused. This is persisted in case we need it in the future.
  @NSManaged public var action: Int32
  /// What type of tracker was blocked.
  @NSManaged public var category: Int32
  /// Base domain of the blocked resource. A pair of `host` and `timestamp` must be unique(to prevent duplicates).
  @NSManaged public var host: String
  /// When a given resource was blocked. A pair of `host` and `timestamp` must be unique(to prevent duplicates).
  @NSManaged public var timestamp: Int64
  /// Message about the blocked resource. Note: this is not localized, always in English.
  @NSManaged public var message: String?
  /// Currently unused. This is persisted in case we need it in the future.
  @NSManaged public var title: String?
  /// Unique identifier of the blocked resource. Must be unique.
  @NSManaged public var uuid: String

  /// Category value is stored as a number in the database. This converts it and returns a proper `TrackerType` enum value for it.
  /// Returns nil if there's no category for a given number.
  public var categoryEnum: TrackerType? {
    return .init(rawValue: Int(category))
  }

  public var id: String {
    // UUID is a unique constraint, this should be enough for identifying the alert.
    uuid
  }

  /// Inserts new VPN alerts to the database. VPN alerts that already exist are skipped.
  public static func batchInsertIfNotExists(alerts: [BraveVPNAlertJSONModel]) {
    DataController.perform { context in
      guard let entity = entity(in: context) else {
        Logger.module.error("Error fetching the entity 'BlockedResource' from Managed Object-Model")

        return
      }

      alerts.forEach {
        let vpnAlert = BraveVPNAlert(entity: entity, insertInto: context)

        // UUID is our first unique key
        vpnAlert.uuid = $0.uuid
        // Pair of host and timestamp are our second unique key.
        // This greatly reduces amount of alerts we save and removes many of duplicated entries.
        vpnAlert.host = $0.host
        vpnAlert.timestamp = $0.timestamp

        vpnAlert.category = Int32($0.category.rawValue)
        vpnAlert.message = $0.message

        // Title and action are currently not used
        vpnAlert.title = $0.title
        vpnAlert.action = Int32($0.action.rawValue)
      }
    }
  }

  /// Returns a list of blocked trackers and how many of them were blocked.
  /// Note: Unlike `BlockedRequest` we do not know what app/website contained a certain tracker.
  /// Sometimes multiple trackers from the same host are recorded.
  /// For `BlockedRequest` items we count trackers from a single domain only once.
  public static func allByHostCount(completion: @escaping (Set<CountableEntity>) -> Void) {
    
    DataController.perform { context in
      let fetchRequest = braveVPNAlertFetchRequest(for: context)
      
      fetchRequest.propertiesToFetch = ["host"]
      fetchRequest.propertiesToGroupBy = ["host"]
      fetchRequest.resultType = .dictionaryResultType
      
      do {
        // Returning 1 count per host here because for the VPN alerts we can't rely on count numbers.
        // Reason is a tracker is detected and saved multiple times per domain,
        // we do not have knowledge on what website or app a given tracker was blocked on.
        // This would lead to inflated stats on the Privacy Reports screen.
        //
        // Each tracker is count as one entry, to only bump the number slightly
        // and show a proper UI that this a vpn alert type of tracker blocked.
        completion(.init(try context.fetch(fetchRequest)
                      .compactMap { $0["host"] as? String }
                      .map { .init(name: $0, count: 1) }))
      } catch {
        Logger.module.error("allByHostCount error: \(error.localizedDescription)")
        completion(.init())
      }
    }
  }

  /// Returns the newest recorded alerts. `count` argument tells up to how many records to fetch.
  public static func last(_ count: Int) -> [BraveVPNAlert]? {
    let timestampSort = NSSortDescriptor(keyPath: \BraveVPNAlert.timestamp, ascending: false)
    let nonConsolidatedAlertsPredicate = NSPredicate(format: "\(#keyPath(BraveVPNAlert.timestamp)) > 0")

    return all(where: nonConsolidatedAlertsPredicate, sortDescriptors: [timestampSort], fetchLimit: count)
  }

  /// Returns amount of alerts blocked for each type.
  public static func alertTotals(completion:
                                 @escaping ((trackerCount: Int, locationPingCount: Int, emailTrackerCount: Int)) -> Void) {
    
    DataController.perform { context in
      let fetchRequest = braveVPNAlertFetchRequest(for: context)

      do {
        fetchRequest.predicate = .init(
          format: "category == %d",
          BraveVPNAlert.TrackerType.app.rawValue)
        let trackerCount = try context.count(for: fetchRequest) + Preferences.BraveVPNAlertTotals.consolidatedEmailTrackerCount.value

        fetchRequest.predicate = .init(
          format: "category == %d",
          BraveVPNAlert.TrackerType.location.rawValue)
        let locationPingCount = try context.count(for: fetchRequest) + Preferences.BraveVPNAlertTotals.consolidatedLocationPingCount.value

        fetchRequest.predicate = .init(
          format: "category == %d",
          BraveVPNAlert.TrackerType.mail.rawValue)
        let emailTrackerCount = try context.count(for: fetchRequest) + Preferences.BraveVPNAlertTotals.consolidatedEmailTrackerCount.value

        completion((trackerCount, locationPingCount, emailTrackerCount))
      } catch {
        Logger.module.error("alertTotals error: \(error.localizedDescription)")
        completion((0, 0, 0))
      }
    }
  }
  
  public static func clearData() {
    deleteAll()
  }
  
  public static func consolidateData(olderThan days: Int) {
    
    struct ConsolidatedVPNAlert: Hashable {
      let host: String
      let action: Int32
      let category: Int32
      
      func hash(into hasher: inout Hasher) {
          hasher.combine(host)
      }
    }
    
    DataController.perform { context in
      let timestampKeyPath = #keyPath(BraveVPNAlert.timestamp)
      
      let date = Int64(Date().timeIntervalSince1970.advanced(by: -Double(days * 60 * 60 * 24)))
      
      // 1. Find all older items.
      let predicate = NSPredicate(format: "\(timestampKeyPath) <= %lld AND \(timestampKeyPath) > 0", date)
      let oldItems = all(where: predicate, context: context)
      
      guard let entity = entity(in: context) else {
        Logger.module.error("Error fetching the entity 'BlockedResource' from Managed Object-Model")
        return
      }
      
      var oldTrackerCount = 0
      var oldLocationPingCount = 0
      var oldEmailTrackerCount = 0
      
      // 2. Insert unique trackers only as we do not care about detection date or their count.
      var uniqueTrackers = Set<ConsolidatedVPNAlert>()
      oldItems?.forEach {
        uniqueTrackers.insert(.init(host: $0.host, action: $0.action, category: $0.category))
        
        switch $0.categoryEnum {
        case .app: oldTrackerCount += 1
        case .location: oldLocationPingCount += 1
        case .mail: oldEmailTrackerCount += 1
        case .none: break
        }
      }
      
      // 3. Consolidating total alerts data.
      Preferences.BraveVPNAlertTotals.consolidatedEmailTrackerCount.value += oldTrackerCount
      Preferences.BraveVPNAlertTotals.consolidatedLocationPingCount.value += oldLocationPingCount
      Preferences.BraveVPNAlertTotals.consolidatedEmailTrackerCount.value += oldEmailTrackerCount
      
      // 4. Consolidating vpn trackers, they are later used for all-time lists.
      uniqueTrackers.forEach {
        let consolidatedRecord = BraveVPNAlert(entity: entity, insertInto: context)
        consolidatedRecord.host = $0.host
        // Making uuid the same as host, this will cause our table's unique constraints
        // to prevent adding multiple entires for each tracker.
        consolidatedRecord.uuid = $0.host
        consolidatedRecord.timestamp = -1
      }
      
      // 5. Remove all old items from the database.
      oldItems?.forEach {
        $0.delete(context: .existing(context))
      }
    }
  }

  private static func braveVPNAlertFetchRequest(for context: NSManagedObjectContext) -> NSFetchRequest<NSDictionary> {
    let _fetchRequest = NSFetchRequest<NSDictionary>(entityName: "BraveVPNAlert")
    _fetchRequest.entity = BraveVPNAlert.entity(in: context)
    return _fetchRequest
  }

  private class func entity(in context: NSManagedObjectContext) -> NSEntityDescription? {
    NSEntityDescription.entity(forEntityName: "BraveVPNAlert", in: context)
  }
}
