/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Foundation
import BraveShared
import Shared

private let log = Logger.browserLogger

public final class Domain: NSManagedObject, CRUD {
    
    @NSManaged public var url: String?
    @NSManaged public var visits: Int32
    @NSManaged public var topsite: Bool // not currently used. Should be used once proper frecency code is in.
    @NSManaged public var blockedFromTopSites: Bool // don't show ever on top sites
    @NSManaged public var favicon: FaviconMO?

    @NSManaged public var shield_allOff: NSNumber?
    @NSManaged public var shield_adblockAndTp: NSNumber?
    @NSManaged public var shield_httpse: NSNumber?
    @NSManaged public var shield_noScript: NSNumber?
    @NSManaged public var shield_fpProtection: NSNumber?
    @NSManaged public var shield_safeBrowsing: NSNumber?

    @NSManaged public var historyItems: NSSet?
    @NSManaged public var bookmarks: NSSet?
    
    private var urlComponents: URLComponents? {
        return URLComponents(string: url ?? "")
    }
    
    /// A domain can be created in many places,
    /// different save strategies are used depending on its relationship(eg. attached to a Bookmark) or browsing mode.
    enum SaveStrategy {
        /// Immediately saves to the persistent store.
        case persistentStore
        /// Targets persistent store but the database save will occur at other place in code, for example after a whole Bookmark is saved.
        case delayedPersistentStore
        /// Saves to an in-memory store. Usually only used when in private browsing mode.
        case inMemory
        
        fileprivate var saveContext: NSManagedObjectContext {
            switch self {
            case .persistentStore, .delayedPersistentStore:
                return DataController.newBackgroundContext()
            case .inMemory:
                return DataController.newBackgroundContextInMemory()
            }
        }
    }
    
    // MARK: - Public interface
    
    public class func getOrCreate(forUrl url: URL, persistent: Bool) -> Domain {
        let context = persistent ? DataController.viewContext : DataController.viewContextInMemory
        let saveStrategy: SaveStrategy = persistent ? .persistentStore : .inMemory
        
        return getOrCreateInternal(url, context: context, saveStrategy: saveStrategy)
    }
    
    /// Returns saved Domain for url or nil if it doesn't exist.
    /// Always called on main thread context.
    public class func getPersistedDomain(for url: URL) -> Domain? {
        Domain.first(where: NSPredicate(format: "url == %@", url.domainURL.absoluteString))
    }
    
    // MARK: Shields

    public class func setBraveShield(forUrl url: URL, shield: BraveShield,
                                     isOn: Bool?, isPrivateBrowsing: Bool) {
        let _context: WriteContext = isPrivateBrowsing ? .new(inMemory: true) : .new(inMemory: false)
        setBraveShieldInternal(forUrl: url, shield: shield, isOn: isOn, context: _context)
    }
    
    /// Whether or not a given shield should be enabled based on domain exceptions and the users global preference
    public func isShieldExpected(_ shield: BraveShield, considerAllShieldsOption: Bool) -> Bool {
        let isShieldOn = { () -> Bool in
            switch shield {
            case .AllOff:
                return self.shield_allOff?.boolValue ?? false
            case .AdblockAndTp:
                return self.shield_adblockAndTp?.boolValue ?? Preferences.Shields.blockAdsAndTracking.value
            case .HTTPSE:
                return self.shield_httpse?.boolValue ?? Preferences.Shields.httpsEverywhere.value
            case .SafeBrowsing:
                return self.shield_safeBrowsing?.boolValue ?? Preferences.Shields.blockPhishingAndMalware.value
            case .FpProtection:
                return self.shield_fpProtection?.boolValue ?? Preferences.Shields.fingerprintingProtection.value
            case .NoScript:
                return self.shield_noScript?.boolValue ?? Preferences.Shields.blockScripts.value
            }
        }
        
        let isAllShieldsOff = Bool(truncating: shield_allOff ?? NSNumber(value: 0))
        let isSpecificShieldOn = isShieldOn()
        return considerAllShieldsOption ? !isAllShieldsOff && isSpecificShieldOn : isSpecificShieldOn
    }
    
    public static func migrateShieldOverrides() {
        // 1.6 had an unfortunate bug that caused shield overrides to create new Domain objects using `http` regardless
        // which would lead to a duplicated Domain object using http that held custom shield overides settings for that
        // domain
        //
        // Therefore we need to migrate any `http` Domains shield overrides to its sibiling `https` domain if it exists
        let allHttpPredicate = NSPredicate(format: "url BEGINSWITH[cd] 'http://'")
        
        DataController.perform { context in
            guard let httpDomains = Domain.all(where: allHttpPredicate, context: context) else {
                return
            }
            
            for domain in httpDomains {
                guard var urlComponents = domain.urlComponents else { continue }
                urlComponents.scheme = "https"
                guard let httpsUrl = urlComponents.url?.absoluteString else { continue }
                if let httpsDomain = Domain.first(where: NSPredicate(format: "url == %@", httpsUrl), context: context) {
                    httpsDomain.shield_allOff = domain.shield_allOff
                    httpsDomain.shield_adblockAndTp = domain.shield_adblockAndTp
                    httpsDomain.shield_noScript = domain.shield_noScript
                    httpsDomain.shield_fpProtection = domain.shield_fpProtection
                    httpsDomain.shield_safeBrowsing = domain.shield_safeBrowsing
                    httpsDomain.shield_httpse = domain.shield_httpse
                    // Could call `domain.delete()` here (or add to batch to delete)
                }
            }
        }
    }
    
    public static func clearInMemoryDomains() {
        Domain.deleteAll(predicate: nil, context: .new(inMemory: true))
    }
}

// MARK: - Internal implementations

extension Domain {
    // Currently required, because not `syncable`
    static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
        return NSEntityDescription.entity(forEntityName: "Domain", in: context)!
    }
    
    /// Returns a Domain for given URL or creates a new object if it doesn't exist.
    /// Note: Save operation can block main thread.
    class func getOrCreateInternal(_ url: URL,
                                   context: NSManagedObjectContext = DataController.viewContext,
                                   saveStrategy: SaveStrategy) -> Domain {
        let domainString = url.domainURL.absoluteString
        if let domain = Domain.first(where: NSPredicate(format: "url == %@", domainString), context: context) {
            return domain
        }

        var newDomain: Domain!

        // Domains are usually accesed on view context, but when the Domain doesn't exist,
        // we have to switch to a background context to avoid writing on view context(bad practice).
        let writeContext = context.concurrencyType ==
            .mainQueueConcurrencyType ? saveStrategy.saveContext : context
        
        writeContext.performAndWait {
            newDomain = Domain(entity: Domain.entity(writeContext), insertInto: writeContext)
            newDomain.url = domainString
            
            let shouldSave = saveStrategy == .persistentStore || saveStrategy == .inMemory
            
            if shouldSave && writeContext.hasChanges {
                do {
                    try writeContext.save()
                } catch {
                    log.error("Domain save error: \(error)")
                }
            }
        }
        
        guard let domainOnCorrectContext = context.object(with: newDomain.objectID) as? Domain else {
            assertionFailure("Could not retrieve domain on correct context")
            return newDomain
        }
        
        return domainOnCorrectContext
    }
    
    class func deleteNonBookmarkedAndClearSiteVisits(context: NSManagedObjectContext,
                                                     _ completionOnMain: @escaping () -> Void) {
        
        let fetchRequest = NSFetchRequest<Domain>()
        fetchRequest.entity = Domain.entity(context)
        do {
            let results = try context.fetch(fetchRequest)
            results.forEach {
                if let bms = $0.bookmarks, bms.count > 0 {
                    // Clear visit count
                    $0.visits = 0
                } else {
                    // Delete
                    context.delete($0)
                }
            }
        } catch {
            let fetchError = error as NSError
            print(fetchError)
        }
        
        DispatchQueue.main.async {
            completionOnMain()
        }
    }
    
    class func getForUrl(_ url: URL) -> Domain? {
        let domainString = url.domainURL.absoluteString
        return Domain.first(where: NSPredicate(format: "url == %@", domainString))
    }
    
    // MARK: Shields
    
    class func setBraveShieldInternal(forUrl url: URL, shield: BraveShield, isOn: Bool?, context: WriteContext = .new(inMemory: false)) {
        DataController.perform(context: context) { context in
            // Not saving here, save happens in `perform` method.
            let domain = Domain.getOrCreateInternal(url, context: context,
                                                    saveStrategy: .delayedPersistentStore)
            domain.setBraveShield(shield: shield, isOn: isOn, context: context)
        }
    }
    
    private func setBraveShield(shield: BraveShield, isOn: Bool?,
                                context: NSManagedObjectContext) {
        
        let setting = (isOn == shield.globalPreference ? nil : isOn) as NSNumber?
        switch shield {
        case .AllOff: shield_allOff = setting
        case .AdblockAndTp: shield_adblockAndTp = setting
        case .HTTPSE:
          shield_httpse = setting
            
          // HTTPSE must be scheme indepedent or user may get stuck not being able to access the http version
          //  of a website (turning off httpse for an upgraded-https domain does not allow access to http version)
          self.domainForInverseHttpScheme(context: context)?.shield_httpse = setting
        case .SafeBrowsing: shield_safeBrowsing = setting
        case .FpProtection: shield_fpProtection = setting
        case .NoScript: shield_noScript = setting
        }
    }
    
    /// Get whether or not a shield override is set for a given shield.
    private func getBraveShield(_ shield: BraveShield) -> Bool? {
        switch shield {
        case .AllOff:
            return self.shield_allOff?.boolValue
        case .AdblockAndTp:
            return self.shield_adblockAndTp?.boolValue
        case .HTTPSE:
            return self.shield_httpse?.boolValue
        case .SafeBrowsing:
            return self.shield_safeBrowsing?.boolValue
        case .FpProtection:
            return self.shield_fpProtection?.boolValue
        case .NoScript:
            return self.shield_noScript?.boolValue
        }
    }
    
    /// Returns `url` but switches the scheme from `http` <-> `https`
    private func domainForInverseHttpScheme(context: NSManagedObjectContext) -> Domain? {
        
        guard var urlComponents = self.urlComponents else { return nil }
        
        // Flip the scheme if valid
        
        switch urlComponents.scheme {
        case "http": urlComponents.scheme = "https"
        case "https": urlComponents.scheme = "http"
        default: return nil
        }
        
        guard let url = urlComponents.url else { return nil }
        
        // Return the flipped scheme version of `url`.
        // Not saving here, save happens in at higher level in `perform` method.
        return Domain.getOrCreateInternal(url, context: context, saveStrategy: .delayedPersistentStore)
    }
}
