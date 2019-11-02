/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Foundation
import BraveShared

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
    
    // MARK: - Public interface
    
    public class func getOrCreate(forUrl url: URL, persistent: Bool) -> Domain {
        let context = persistent ? DataController.viewContext : DataController.viewContextInMemory
        return getOrCreateInternal(url, context: context, save: true)
    }
    
    // MARK: Shields

    public class func setBraveShield(forUrl url: URL, shield: BraveShield,
                                     isOn: Bool?, isPrivateBrowsing: Bool) {
        let _context: WriteContext = isPrivateBrowsing ? .new(inMemory: true) : .new(inMemory: false)
        setBraveShieldInternal(forUrl: url, shield: shield, isOn: isOn, context: _context)
    }
    
    /// Whether or not a given shield should be enabled based on domain exceptions and the users global preference
    public func isShieldExpected(_ shield: BraveShield) -> Bool {
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
    
    class func getOrCreateInternal(_ url: URL,
                                   context: NSManagedObjectContext = DataController.viewContext,
                                   save: Bool = true) -> Domain {
        let domainString = url.domainURL.absoluteString
        if let domain = Domain.first(where: NSPredicate(format: "url == %@", domainString), context: context) {
            return domain
        }
        
        // See #409:
        //  A larger refactor is probably wanted here.
        //  This can easily lead to a Domain being created on the `viewContext`
        //  A solution to consider is creating a new background context here, creating, saving, and then re-fetching
        //   that object in the requested context (regardless if it is `viewContext` or not)
        var newDomain: Domain!
        context.performAndWait {
            newDomain = Domain(entity: Domain.entity(context), insertInto: context)
            newDomain.url = domainString
            if save {
                DataController.save(context: context)
            }
        }
        return newDomain
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
            for obj in results {
                // Cascading delete on favicon, it will also get deleted
                context.delete(obj)
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
            let domain = Domain.getOrCreateInternal(url, context: context)
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
        
        // Return the flipped scheme version of `url`
        return Domain.getOrCreateInternal(url, context: context, save: true)
    }
}
