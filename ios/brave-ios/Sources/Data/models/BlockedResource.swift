// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import CoreData
import Shared
import os.log

func getDate(_ dayOffset: Int) -> Date {
  let calendar = Calendar(identifier: Calendar.Identifier.gregorian)
  let nowComponents = calendar.dateComponents([Calendar.Component.year, Calendar.Component.month, Calendar.Component.day], from: Date())
  let today = calendar.date(from: nowComponents)!
  return (calendar as NSCalendar).date(byAdding: NSCalendar.Unit.day, value: dayOffset, to: today, options: [])!
}

public enum BlockedResourceType: Int32 {
  case ad = 0
  case tracker = 1
}

public final class BlockedResource: NSManagedObject, CRUD {
  /// Domain where a `host` was blocked on, for example 'example.com'.
  /// Base domain is used to help with grouping trackers to a single website.
  @NSManaged public var domain: String
  /// Full url of the `domain`. This is te reuse existing fucntionality to fetch correct favicons.
  @NSManaged public var faviconUrl: String
  /// Host of the blocked tracker, for example doubleclick.net
  @NSManaged public var host: String
  
  /// When a given tracker was blocked on a website.
  /// If this timestamp is nil it means that's a consolidated record.
  ///
  /// Older blocked resources are consolidated after 30 days and stored without timestamp.
  /// This is to avoid having data heavy database and for privacy reasons.
  /// Consolidated records are view to list all-time blocked items data.
  @NSManaged public var timestamp: Date?
  
  private static let entityName = "BlockedResource"
  private static let hostKeyPath = #keyPath(BlockedResource.host)
  private static let domainKeyPath = #keyPath(BlockedResource.domain)
  private static let timestampKeyPath = #keyPath(BlockedResource.timestamp)
  
  public static func batchInsert(items: [(host: String, domain: URL, date: Date)]) {
    
    DataController.perform { context in
      guard let entity = entity(in: context) else {
        Logger.module.error("Error fetching the entity 'BlockedResource' from Managed Object-Model")
        return
      }
      
      items.forEach {
        guard let baseDomain = $0.domain.baseDomain else {
          return
        }
        
        let blockedResource = BlockedResource(entity: entity, insertInto: context)
        blockedResource.host = $0.host
        blockedResource.domain = baseDomain
        blockedResource.faviconUrl = $0.domain.domainURL.absoluteString
        blockedResource.timestamp = $0.date
      }
    }
  }
  
  /// Returns a name and count of the most blocked tracker for given timeframe.
  /// If the `days` parameter is nil, all data is considered including consolidated records.
  /// /// Returns nil in case of error or if nothing was found.
  public static func mostBlockedTracker(inLastDays days: Int?, completion: @escaping (CountableEntity?) -> Void) {
    
    DataController.perform { context in
      var mostFrequentTracker = CountableEntity(name: "", count: 0)
      
      do {
        let results = try groupByFetch(property: hostKeyPath, daysRange: days, context: context)
        
        for result in results {
          guard let host = result[hostKeyPath] as? String else {
            continue
          }
          
          // Finding on how many websites a given tracker was found.
          // Due to CD limitations this can't be fetched with a single query.
          let result = try distinctValues(property: hostKeyPath,
                                          propertyToFetch: domainKeyPath,
                                          value: host,
                                          daysRange: days,
                                          context: context).count
          
          if result > mostFrequentTracker.count {
            mostFrequentTracker = .init(name: host, count: result)
          }
        }
        
        completion(mostFrequentTracker.count > 0 ? mostFrequentTracker : nil)
      } catch {
        Logger.module.error("\(error.localizedDescription)")
        completion(nil)
      }
    }
  }
  
  /// Returns a Set of all blocked trackers and on how many websites they were detected.
  public static func allTimeMostFrequentTrackers(completion: @escaping  (Set<CountableEntity>) -> Void) {
    
    DataController.perform { context in
      var allTrackers = Set<CountableEntity>()
      
      do {
        let results = try groupByFetch(property: hostKeyPath, daysRange: nil, context: context)
        
        for result in results {
          guard let host = result[hostKeyPath] as? String else {
            continue
          }
          
          // Finding on how many websites a given tracker was found.
          // Due to CD limitations this can't be fetched with a single query.
          let shieldsCount = try distinctValues(property: hostKeyPath,
                                                propertyToFetch: domainKeyPath,
                                                value: host,
                                                daysRange: nil,
                                                context: context).count
          
          allTrackers.insert(.init(name: host, count: shieldsCount))
        }
        
        completion(allTrackers)
      } catch {
        Logger.module.error("\(error.localizedDescription)")
        completion(allTrackers)
      }
    }
  }
  
  /// Returns a name and count of a website that contained the highest number of unique trackers for a given timeframe.
  /// If the `days` parameter is nil, all data is considered including consolidated records.
  /// Returns nil in case of error or if nothing was found.
  public static func riskiestWebsite(inLastDays days: Int?, completion: @escaping (CountableEntity?) -> Void) {
    
    DataController.perform { context in
      var mostRiskyWebsite = CountableEntity(name: "", count: 0)
      
      do {
        let results = try groupByFetch(property: domainKeyPath, daysRange: days, context: context)
        
        for result in results {
          guard let domain = result[domainKeyPath] as? String else {
            continue
          }
          
          // Finding how many trackers each website contains.
          // Due to CD limitations this can't be fetched with a single query.
          let result = try distinctValues(property: domainKeyPath,
                                          propertyToFetch: hostKeyPath,
                                          value: domain,
                                          daysRange: days,
                                          context: context).count
          
          if result > mostRiskyWebsite.count {
            mostRiskyWebsite = .init(name: domain, count: result)
          }
        }
        
        completion(mostRiskyWebsite.count > 0 ? mostRiskyWebsite : nil)
      } catch {
        Logger.module.error("\(error.localizedDescription)")
        completion(nil)
      }
    }
  }
  
  /// Returns an array of websites with detected trackers, and how many unique trackers were detected on it.
  public static func allTimeMostRiskyWebsites(completion:
                                              @escaping ([(domain: String, faviconUrl: String, count: Int)]) -> Void) {
    
    DataController.perform { context in
      var allWebsites = [(domain: String, faviconUrl: String, count: Int)]()
      
      do {
        let fetchRequest = NSFetchRequest<NSDictionary>(entityName: entityName)
        fetchRequest.entity = BlockedResource.entity(in: context)
        
        let expression = NSExpressionDescription()
        expression.name = "favicon"
        // Hack: Core Data does not allow you to add property to fetch that is not used in GROUP BY sql argument.
        // The only was to bypass this is to pass a sqlite function like lowercase:, sum: etc.
        // For this case we do not care about faviconUrl capitalzation and lowercase function is used only
        // to select additional table field.
        expression.expression = .init(forFunction: "lowercase:", arguments: [NSExpression(forKeyPath: "faviconUrl")])
        expression.expressionResultType = .stringAttributeType
        
        fetchRequest.propertiesToFetch = ["domain", expression]
        fetchRequest.propertiesToGroupBy = ["domain"]
        fetchRequest.resultType = .dictionaryResultType
        
        let results = try context.fetch(fetchRequest)
        
        for result in results {
          guard let domain = result[domainKeyPath] as? String, let favicon = result["favicon"] as? String else {
            continue
          }
          
          // Finding how many trackers each website contains.
          // Due to CD limitations this can't be fetched with a single query.
          let result = try distinctValues(property: domainKeyPath,
                                          propertyToFetch: hostKeyPath,
                                          value: domain,
                                          daysRange: nil,
                                          context: context).count
          
          allWebsites.append((domain, favicon, result))
        }
        
        completion(allWebsites.sorted(by: { $0.count > $1.count }))
      } catch {
        Logger.module.error("\(error.localizedDescription)")
        completion([])
      }
    }
  }
  
  public static func consolidateData(olderThan days: Int) {
    
    struct DomainTrackerPair: Hashable {
      let domain: String
      let tracker: String
      let faviconUrl: String
      
      func hash(into hasher: inout Hasher) {
        hasher.combine(domain)
        hasher.combine(tracker)
      }
    }
    
    DataController.perform { context in
      // 1. Find all older items.
      let predicate = NSPredicate(format: "\(timestampKeyPath) <= %@ AND \(timestampKeyPath) != nil",
                                  getDate(-days) as CVarArg)
      let oldItems = all(where: predicate, context: context)
      
      guard let entity = entity(in: context) else {
        Logger.module.error("Error fetching the entity 'BlockedResource' from Managed Object-Model")
        return
      }
      
      // 2. Preserve unique items only. Since this is used for the all-time lists
      // multiple entires of the same domain-tracker pair are not needed.
      var set = Set<DomainTrackerPair>()
      oldItems?.forEach {
        set.insert(DomainTrackerPair(domain: $0.domain, tracker: $0.host, faviconUrl: $0.faviconUrl))
      }
      
      set.forEach {
        let predicate =
        NSPredicate(format: "\(domainKeyPath) == %@ AND \(hostKeyPath) == %@ AND \(timestampKeyPath) = nil",
                    $0.domain, $0.tracker)
        
        // 3. Adding new consolidated records only if not added already
        if first(where: predicate, context: context) == nil {
          let consolidatedRecord = BlockedResource(entity: entity, insertInto: context)
          consolidatedRecord.host = $0.tracker
          consolidatedRecord.domain = $0.domain
          consolidatedRecord.faviconUrl = $0.faviconUrl
          consolidatedRecord.timestamp = nil
        }
      }
      
      // 4. Remove all old items from the database.
      oldItems?.forEach {
        $0.delete(context: .existing(context))
      }
    }
  }
  
  /// A helper method for to group up elements.
  /// - Parameters:
  ///     - property: What property we group by for.
  ///     - daysRange: How old items to look for. If this value is nil, all data is considered including consolidated records.
  private static func groupByFetch(property: String,
                                   daysRange days: Int?,
                                   context: NSManagedObjectContext) throws -> [NSDictionary] {
    
    let fetchRequest = NSFetchRequest<NSDictionary>(entityName: entityName)
    fetchRequest.entity = BlockedResource.entity(in: context)
    
    fetchRequest.propertiesToFetch = [property]
    fetchRequest.resultType = .dictionaryResultType
    fetchRequest.returnsDistinctResults = true
    
    if let days = days {
      fetchRequest.predicate = NSPredicate(format: "\(timestampKeyPath) >= %@", getDate(-days) as CVarArg)
    }
    
    let results = try context.fetch(fetchRequest)
    return results
  }
  
  /// Clears the BlockedResources database.
  public static func clearData() {
    deleteAll()
  }
  
  /// Helper method which returns unique values for a given query.
  /// - Parameters:
  ///     - property: What property do we query for.
  ///     - propertyToFetch: What property we do want to fetch from our model.
  ///     - value: Value of property we query for.
  ///     - daysRange: If not nil it constraits returned results, retrieves results no older than this param. If nil, we check for all entries.
  private static func distinctValues(
    property: String,
    propertyToFetch: String,
    value: String,
    daysRange days: Int?,
    context: NSManagedObjectContext
  ) throws -> [NSFetchRequestResult] {
    var predicate: NSPredicate?
    if let days = days {
      predicate = NSPredicate(
        format: "\(timestampKeyPath) >= %@ AND \(property) == %@", getDate(-days) as CVarArg, value)
    } else {
      predicate = NSPredicate(format: "\(property) == %@", value)
    }
    
    let fetchRequest = NSFetchRequest<NSFetchRequestResult>(entityName: entityName)
    fetchRequest.entity = BlockedResource.entity(in: context)
    
    fetchRequest.propertiesToFetch = [propertyToFetch]
    fetchRequest.propertiesToGroupBy = [propertyToFetch]
    fetchRequest.resultType = .dictionaryResultType
    fetchRequest.predicate = predicate
    
    let result = try context.fetch(fetchRequest)
    return result
  }
  
  private class func entity(in context: NSManagedObjectContext) -> NSEntityDescription? {
    NSEntityDescription.entity(forEntityName: entityName, in: context)
  }
}
