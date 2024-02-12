/* This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import CoreData
import Foundation
import BraveShields
import Shared
import BraveCore
import os.log
import Preferences

public final class Domain: NSManagedObject, CRUD {

  @NSManaged public var url: String?
  @NSManaged public var visits: Int32
  @NSManaged public var topsite: Bool  // not currently used. Should be used once proper frecency code is in.
  @NSManaged public var blockedFromTopSites: Bool  // don't show ever on top sites

  @NSManaged public var shield_allOff: NSNumber?
  @NSManaged public var shield_adblockAndTp: NSNumber?

  @available(*, deprecated, message: "Per domain HTTPSE shield is currently unused.")
  @NSManaged public var shield_httpse: NSNumber?

  @NSManaged public var shield_noScript: NSNumber?
  @NSManaged public var shield_fpProtection: NSNumber?
  @NSManaged public var shield_safeBrowsing: NSNumber?

  @NSManaged public var bookmarks: NSSet?

  @NSManaged public var wallet_permittedAccounts: String?
  @NSManaged public var zoom_level: NSNumber?
  @NSManaged public var wallet_solanaPermittedAcccounts: String?

  private var urlComponents: URLComponents? {
    return URLComponents(string: url ?? "")
  }
  
  // TODO: @JS Replace this with the 1st party ad-block list
  // https://github.com/brave/brave-ios/issues/7611
  /// A list of etld+1s that are always aggressive
  private let alwaysAggressiveETLDs: Set<String> = ["youtube.com"]
  
  /// Return the shield level for this domain.
  ///
  /// - Warning: This does not consider the "all off" setting
  /// This also takes into consideration certain domains that are always aggressive.
  @MainActor public var blockAdsAndTrackingLevel: ShieldLevel {
    guard isShieldExpected(.AdblockAndTp, considerAllShieldsOption: false) else { return .disabled }
    let globalLevel = ShieldPreferences.blockAdsAndTrackingLevel
    
    switch globalLevel {
    case .standard:
      guard let urlString = self.url else { return globalLevel }
      guard let url = URL(string: urlString) else { return globalLevel }
      guard let etldP1 = url.baseDomain else { return globalLevel }
      
      if alwaysAggressiveETLDs.contains(etldP1) {
        return .aggressive
      } else {
        return globalLevel
      }
    case .disabled, .aggressive:
      return globalLevel
    }
  }
  
  /// Return the finterprinting protection level for this domain.
  ///
  /// - Warning: This does not consider the "all off" setting
  @MainActor public var finterprintProtectionLevel: ShieldLevel {
    guard isShieldExpected(.FpProtection, considerAllShieldsOption: false) else { return .disabled }
    // We don't have aggressive finterprint protection in iOS
    return .standard
  }
  
  private static let containsEthereumPermissionsPredicate = NSPredicate(format: "wallet_permittedAccounts != nil && wallet_permittedAccounts != ''")
  private static let containsSolanaPermissionsPredicate = NSPredicate(format: "wallet_solanaPermittedAcccounts != nil && wallet_solanaPermittedAcccounts != ''")
  
  @MainActor public var areAllShieldsOff: Bool {
    return shield_allOff?.boolValue ?? false
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
  
  public class func frc() -> NSFetchedResultsController<Domain> {
    let context = DataController.viewContext
    let fetchRequest = NSFetchRequest<Domain>()
    fetchRequest.entity = Domain.entity(context)
    fetchRequest.sortDescriptors = [NSSortDescriptor(key: "url", ascending: false)]

    return NSFetchedResultsController(
      fetchRequest: fetchRequest,
      managedObjectContext: context,
      sectionNameKeyPath: nil,
      cacheName: nil
    )
  }

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

  public class func setBraveShield(
    forUrl url: URL, shield: BraveShield,
    isOn: Bool?, isPrivateBrowsing: Bool
  ) {
    let _context: WriteContext = isPrivateBrowsing ? .new(inMemory: true) : .new(inMemory: false)
    setBraveShieldInternal(forUrl: url, shield: shield, isOn: isOn, context: _context)
  }

  /// Whether or not a given shield should be enabled based on domain exceptions and the users global preference
  @MainActor public func isShieldExpected(_ shield: BraveShield, considerAllShieldsOption: Bool) -> Bool {
    let isShieldOn = { () -> Bool in
      switch shield {
      case .AllOff:
        return self.shield_allOff?.boolValue ?? false
      case .AdblockAndTp:
        return self.shield_adblockAndTp?.boolValue ?? ShieldPreferences.blockAdsAndTrackingLevel.isEnabled
      case .FpProtection:
        return self.shield_fpProtection?.boolValue ?? Preferences.Shields.fingerprintingProtection.value
      case .NoScript:
        return self.shield_noScript?.boolValue ?? Preferences.Shields.blockScripts.value
      }
    }()

    let isAllShieldsOff = self.shield_allOff?.boolValue ?? false
    let isSpecificShieldOn = isShieldOn
    return considerAllShieldsOption ? !isAllShieldsOff && isSpecificShieldOn : isSpecificShieldOn
  }

  public static func clearInMemoryDomains() {
    Domain.deleteAll(predicate: nil, context: .new(inMemory: true))
  }
  
  public class func totalDomainsWithAdblockShieldsLoweredFromGlobal() -> Int {
    guard ShieldPreferences.blockAdsAndTrackingLevel.isEnabled,
          let domains = Domain.all(where: NSPredicate(format: "shield_adblockAndTp != nil")) else {
      return 0 // Can't be lower than off
    }
    return domains.filter({ $0.shield_adblockAndTp?.boolValue == false }).count
  }
  
  public class func totalDomainsWithAdblockShieldsIncreasedFromGlobal() -> Int {
    guard !ShieldPreferences.blockAdsAndTrackingLevel.isEnabled,
          let domains = Domain.all(where: NSPredicate(format: "shield_adblockAndTp != nil")) else {
      return 0 // Can't be higher than on
    }
    return domains.filter({ $0.shield_adblockAndTp?.boolValue == true }).count
  }
  
  public class func totalDomainsWithFingerprintingProtectionLoweredFromGlobal() -> Int {
    guard Preferences.Shields.fingerprintingProtection.value,
          let domains = Domain.all(where: NSPredicate(format: "shield_fpProtection != nil")) else {
      return 0 // Can't be lower than off
    }
    return domains.filter({ $0.shield_fpProtection?.boolValue == false }).count
  }
  
  public class func totalDomainsWithFingerprintingProtectionIncreasedFromGlobal() -> Int {
    guard !Preferences.Shields.fingerprintingProtection.value,
          let domains = Domain.all(where: NSPredicate(format: "shield_fpProtection != nil")) else {
      return 0 // Can't be higher than on
    }
    return domains.filter({ $0.shield_fpProtection?.boolValue == true }).count
  }

  // MARK: Wallet
  
  public class func setWalletPermissions(
    forUrl url: URL,
    coin: BraveWallet.CoinType,
    accounts: [String],
    grant: Bool
  ) {
    // no dapps support in private browsing mode
    let _context: WriteContext = .new(inMemory: false)
    setWalletPermissions(
      forUrl: url,
      coin: coin,
      accounts: accounts,
      grant: grant,
      context: _context
    )
  }
  
  public class func walletPermissions(forUrl url: URL, coin: BraveWallet.CoinType) -> [String]? {
    let domain = getOrCreateInternal(url, saveStrategy: .persistentStore)
    switch coin {
    case .eth:
      return domain.wallet_permittedAccounts?.split(separator: ",").map(String.init)
    case .sol:
      return domain.wallet_solanaPermittedAcccounts?.split(separator: ",").map(String.init)
    case .fil:
      return nil
    case .btc:
      return nil
    @unknown default:
      return nil
    }
  }
  
  public func walletPermissions(for coin: BraveWallet.CoinType, account: String) -> Bool {
    switch coin {
    case .eth:
      if let permittedAccount = wallet_permittedAccounts {
        return permittedAccount.components(separatedBy: ",").contains(account)
      }
    case .sol:
      if let permittedAccount = wallet_solanaPermittedAcccounts {
        return permittedAccount.components(separatedBy: ",").contains(account)
      }
    case .fil:
      break
    case .btc:
      break
    @unknown default:
      break
    }
    return false
  }
  
  public class func allDomainsWithWalletPermissions(for coin: BraveWallet.CoinType, context: NSManagedObjectContext? = nil) -> [Domain] {
    switch coin {
    case .eth:
      let predicate = Domain.containsEthereumPermissionsPredicate
      return all(where: predicate, context: context ?? DataController.viewContext) ?? []
    case .sol:
      let predicate = Domain.containsSolanaPermissionsPredicate
      return all(where: predicate, context: context ?? DataController.viewContext) ?? []
    case .fil:
      break
    case .btc:
      break
    @unknown default:
      break
    }
    return []
  }
  
  public static func clearAllWalletPermissions(
    for coin: BraveWallet.CoinType,
    _ completionOnMain: (() -> Void)? = nil
  ) {
    DataController.perform { context in
      let fetchRequest = NSFetchRequest<Domain>()
      fetchRequest.entity = Domain.entity(context)
      do {
        let results = try context.fetch(fetchRequest)
        results.forEach {
          switch coin {
          case .eth:
            $0.wallet_permittedAccounts = nil
          case .sol:
            $0.wallet_solanaPermittedAcccounts = nil
          case .fil:
            break
          case .btc:
            break
          @unknown default:
            break
          }
        }
      } catch {
        Logger.module.error("Clear coin(\(coin.rawValue)) accounts permissions error: \(error.localizedDescription)")
      }

      DispatchQueue.main.async {
        completionOnMain?()
      }
    }
  }

  @MainActor public static func clearAllWalletPermissions(for coin: BraveWallet.CoinType) async {
    await withCheckedContinuation { continuation in
      Domain.clearAllWalletPermissions(for: coin) {
        continuation.resume()
      }
    }
  }
}

// MARK: - Internal implementations

extension Domain {
  // Currently required, because not `syncable`
  public static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
    return NSEntityDescription.entity(forEntityName: "Domain", in: context)!
  }

  /// Returns a Domain for given URL or creates a new object if it doesn't exist.
  /// Note: Save operation can block main thread.
  class func getOrCreateInternal(
    _ url: URL,
    context: NSManagedObjectContext = DataController.viewContext,
    saveStrategy: SaveStrategy
  ) -> Domain {
    let domainString = url.domainURL.absoluteString
    if let domain = Domain.first(where: NSPredicate(format: "url == %@", domainString), context: context) {
      return domain
    }

    var newDomain: Domain!

    // Domains are usually accesed on view context, but when the Domain doesn't exist,
    // we have to switch to a background context to avoid writing on view context(bad practice).
    let writeContext = context.concurrencyType == .mainQueueConcurrencyType ? saveStrategy.saveContext : context

    writeContext.performAndWait {
      newDomain = Domain(entity: Domain.entity(writeContext), insertInto: writeContext)
      newDomain.url = domainString

      let shouldSave = saveStrategy == .persistentStore || saveStrategy == .inMemory

      if shouldSave && writeContext.hasChanges {
        do {
          try writeContext.save()
        } catch {
          Logger.module.error("Domain save error: \(error.localizedDescription)")
        }
      }
    }

    guard let domainOnCorrectContext = context.object(with: newDomain.objectID) as? Domain else {
      assertionFailure("Could not retrieve domain on correct context")
      return newDomain
    }

    return domainOnCorrectContext
  }

  public class func deleteNonBookmarkedAndClearSiteVisits(_ completionOnMain: @escaping () -> Void) {
    DataController.perform { context in
      let fetchRequest = NSFetchRequest<Domain>()
      fetchRequest.entity = Domain.entity(context)
      do {
        let results = try context.fetch(fetchRequest)
        results.forEach {
          if let bms = $0.bookmarks, bms.count > 0 {
            // Clear visit count and clear the shield settings
            $0.visits = 0
            $0.shield_allOff = nil
            $0.shield_adblockAndTp = nil
            $0.shield_noScript = nil
            $0.shield_fpProtection = nil
            $0.shield_safeBrowsing = nil
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
  }

  class func getForUrl(_ url: URL) -> Domain? {
    let domainString = url.domainURL.absoluteString
    return Domain.first(where: NSPredicate(format: "url == %@", domainString))
  }

  // MARK: Shields

  class func setBraveShieldInternal(forUrl url: URL, shield: BraveShield, isOn: Bool?, context: WriteContext = .new(inMemory: false)) {
    DataController.perform(context: context) { context in
      // Not saving here, save happens in `perform` method.
      let domain = Domain.getOrCreateInternal(
        url, context: context,
        saveStrategy: .delayedPersistentStore)
      domain.setBraveShield(shield: shield, isOn: isOn, context: context)
    }
  }

  private func setBraveShield(
    shield: BraveShield, isOn: Bool?,
    context: NSManagedObjectContext
  ) {

    let setting = (isOn == shield.globalPreference ? nil : isOn) as NSNumber?
    switch shield {
    case .AllOff: shield_allOff = setting
    case .AdblockAndTp: shield_adblockAndTp = setting
    case .FpProtection: shield_fpProtection = setting
    case .NoScript: shield_noScript = setting
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

  // MARK: Wallet
  
  class func setWalletPermissions(
    forUrl url: URL,
    coin: BraveWallet.CoinType,
    accounts: [String],
    grant: Bool,
    context: WriteContext = .new(inMemory: false)
  ) {
    DataController.perform(context: context) { context in
      for account in accounts {
        // Not saving here, save happens in `perform` method.
        let domain = Domain.getOrCreateInternal(
          url, context: context,
          saveStrategy: .persistentStore)
        domain.setWalletDappPermission(
          for: coin,
          account: account,
          grant: grant,
          context: context
        )
      }
    }
  }
  
  private func setWalletDappPermission(
    for coin: BraveWallet.CoinType,
    account: String,
    grant: Bool,
    context: NSManagedObjectContext
  ) {
    if grant {
      switch coin {
      case .eth:
        if let permittedAccounts = wallet_permittedAccounts {
          // make sure stored `wallet_permittedAccounts` does not contain this `account`
          // make sure this `account` does not contain any comma
          if !permittedAccounts.contains(account), !account.contains(",") {
            wallet_permittedAccounts = [permittedAccounts, account].joined(separator: ",")
          }
        } else {
          wallet_permittedAccounts = account
        }
      case .sol:
        if let permittedAccounts = wallet_solanaPermittedAcccounts {
          // make sure stored `wallet_solanaPermittedAcccounts` does not contain this `account`
          // make sure this `account` does not contain any comma
          if !permittedAccounts.contains(account), !account.contains(",") {
            wallet_solanaPermittedAcccounts = [permittedAccounts, account].joined(separator: ",")
          }
        } else {
          wallet_solanaPermittedAcccounts = account
        }
      case .fil:
        break
      case .btc:
        break
      @unknown default:
        break
      }
    } else {
      switch coin {
      case .eth:
        if var accounts = wallet_permittedAccounts?.components(separatedBy: ","),
           let index = accounts.firstIndex(of: account) {
          accounts.remove(at: index)
          wallet_permittedAccounts = accounts.joined(separator: ",")
        }
      case .sol:
        if var accounts = wallet_solanaPermittedAcccounts?.components(separatedBy: ","),
           let index = accounts.firstIndex(of: account) {
          accounts.remove(at: index)
          wallet_solanaPermittedAcccounts = accounts.joined(separator: ",")
        }
      case .fil:
        break
      case .btc:
        break
      @unknown default:
        break
      }
    }
  }
}
