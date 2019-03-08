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
    
    // MARK: - Public interface
    
    public class func getOrCreate(forUrl url: URL) -> Domain {
        return getOrCreateInternal(url)
    }
    
    // MARK: Shields
    
    /// Remove all private browsing shield overrides
    public class func resetPrivateBrowsingShieldOverrides() {
        PrivateBrowsingShieldOverride.privateModeOverrides.removeAll()
    }
    
    public class func setBraveShield(forUrl url: URL, shield: BraveShield,
                                     isOn: Bool?, isPrivateBrowsing: Bool) {
        setBraveShieldInternal(forUrl: url, shield: shield, isOn: isOn, isPrivateBrowsing: isPrivateBrowsing)
    }
    
    /// Whether or not a given shield should be enabled based on domain exceptions and the users global preference
    public func isShieldExpected(_ shield: BraveShield, isPrivateBrowsing: Bool) -> Bool {
        // If we're private browsing we may have private only overrides that the user made during their private
        // session
        if isPrivateBrowsing, let key = url,
            let privateModeOverride = PrivateBrowsingShieldOverride.privateModeOverrides[key]?[shield] {
            return privateModeOverride
        }
        
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
                guard let urlString = domain.url, var urlComponents = URLComponents(string: urlString) else { continue }
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
    
    class func getBraveShield(forUrl url: URL, shield: BraveShield, isPrivateBrowsing: Bool,
                              context: NSManagedObjectContext? = nil) -> Bool? {
        if isPrivateBrowsing {
            return getPrivateShieldForDomainUrl(url.domainURL.absoluteString, shield: shield)
        }
        let domain = Domain.getOrCreateInternal(url)
        return domain.getBraveShield(shield)
    }
    
    class func setBraveShieldInternal(forUrl url: URL, shield: BraveShield, isOn: Bool?, isPrivateBrowsing: Bool, context: WriteContext = .new) {
        
        DataController.perform(context: context) { context in
            if isPrivateBrowsing {
                setPrivateShieldForDomainUrl(url.domainURL.absoluteString, shield: shield, isOn: isOn,
                                             isPrivateBrowsing: isPrivateBrowsing, context: context)
                return
            }
            let domain = Domain.getOrCreateInternal(url, context: context)
            domain.setBraveShield(shield: shield, isOn: isOn, context: context, isPrivateBrowsing: isPrivateBrowsing)
        }
    }
    
    private func setBraveShield(shield: BraveShield, isOn: Bool?,
                                context: NSManagedObjectContext, isPrivateBrowsing: Bool) {
        if isPrivateBrowsing {
            assertionFailure("Domain objects should not be modified while in private mode")
            return
        }
        
        let setting = (isOn == shield.globalPreference ? nil : isOn) as NSNumber?
        switch shield {
        case .AllOff: shield_allOff = setting
        case .AdblockAndTp: shield_adblockAndTp = setting
        case .HTTPSE: shield_httpse = setting
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
    
    /// Set the private shield based on `domainURL`
    private class func setPrivateShieldForDomainUrl(_ domainURL: String, shield: BraveShield, isOn: Bool?,
                                                    isPrivateBrowsing: Bool, context: NSManagedObjectContext) {
        guard let url = URL(string: domainURL) else { return }
        // Remove private mode override if its set to the same value as the Domain's override
        let shieldForUrl = getBraveShield(forUrl: url, shield: shield, isPrivateBrowsing: isPrivateBrowsing,
                                          context: context)
        let setting = (isOn == shieldForUrl ? nil : isOn)
        if let on = setting {
            if let override = PrivateBrowsingShieldOverride.privateModeOverrides[domainURL] {
                override[shield] = on
            } else {
                PrivateBrowsingShieldOverride.privateModeOverrides[domainURL] = PrivateBrowsingShieldOverride(shield: shield, isOn: on)
            }
        } else {
            PrivateBrowsingShieldOverride.privateModeOverrides[domainURL]?[shield] = nil
        }
    }
    
    /// Get the private shield based on `domainURL`
    ///
    /// Assuming no private mode overrides are found, checks if the Domain object exists for this given URL
    /// If it does, use the Domain's shield value, otherwise defualt to the global preference
    private class func getPrivateShieldForDomainUrl(_ domainURL: String, shield: BraveShield) -> Bool? {
        if let privateModeOverride = PrivateBrowsingShieldOverride.privateModeOverrides[domainURL]?[shield] {
            return privateModeOverride
        }
        if let url = URL(string: domainURL), let domain = Domain.getForUrl(url) {
            return domain.getBraveShield(shield)
        }
        return shield.globalPreference
    }
}
