// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import NetworkExtension
import Data
import GuardianConnect

private let log = Logger.browserLogger

/// A static class to handle all things related to the Brave VPN service.
public class BraveVPN {

  private static let housekeepingApi = GRDHousekeepingAPI()
  private static let helper = GRDVPNHelper.sharedInstance()
  private static let serverManager = GRDServerManager()
  
  public static let iapObserver = IAPObserver()

  /// List of regions the VPN can connect to.
  /// This list is not static and should be refetched every now and then.
  static var regions: [GRDRegion] = []
  
  // Non translatable
  private static let connectionName = "Brave Firewall + VPN"

  // MARK: - Initialization

  /// This class is supposed to act as a namespace, disabling possibility of creating an instance of it.
  @available(*, unavailable)
  init() {}

  /// Initialize the vpn service. It should be called even if the user hasn't bought the vpn yet.
  /// This function can have side effects if the receipt has expired(removes the vpn connection then).
  public static func initialize() {
    func clearConfiguration() {
      GRDVPNHelper.clearVpnConfiguration()
      clearCredentials()

      NEVPNManager.shared().removeFromPreferences { error in
        if let error = error {
          logAndStoreError("Remove vpn error: \(error)")
        }
      }
    }
    
    helper.verifyMainCredentials { valid, error in
      logAndStoreError("Initialize credentials valid: \(valid)")
      if let error = error {
        logAndStoreError("Initialize credentials error: \(error)")
      }
    }

    helper.dummyDataForDebugging = !AppConstants.buildChannel.isPublic
    helper.tunnelLocalizedDescription = connectionName

    if case .notPurchased = vpnState {
      // Unlikely if user has never bought the vpn, we clear vpn config here for safety.
      clearConfiguration()
      return
    }
    
    // We validate the current receipt at the start to know if the subscription has expirerd.
    BraveVPN.validateReceipt() { expired in
      if expired == true {
        clearConfiguration()
        logAndStoreError("Receipt expired")
        return
      }
    }
  }
  
  /// Connects to Guardian's server to validate locally stored receipt.
  /// Returns true if the receipt expired, false if not or nil if expiration status can't be determined.
  public static func validateReceipt(receiptHasExpired: ((Bool?) -> Void)? = nil) {
    guard let receiptUrl = Bundle.main.appStoreReceiptURL,
          let receipt = try? Data(contentsOf: receiptUrl).base64EncodedString,
          let bundleId = Bundle.main.bundleIdentifier else {
      receiptHasExpired?(nil)
      return
    }

    housekeepingApi.verifyReceipt(receipt, bundleId: bundleId) { validSubscriptions, success, error in
      if !success {
        // Api call for receipt verification failed,
        // we do not know if the receipt has expired or not.
        receiptHasExpired?(nil)
        logAndStoreError("Api call for receipt verification failed: \(error ?? "unknown error")")
        return
      }

      guard let validSubscriptions = validSubscriptions,
            let newestReceipt = validSubscriptions.sorted(by: { $0.expiresDate > $1.expiresDate }).first else {
        // Setting super old date to force expiration logic in the UI.
        Preferences.VPN.expirationDate.value = Date(timeIntervalSince1970: 1)
        receiptHasExpired?(true)
        logAndStoreError("vpn expired", printToConsole: false)
        return
      }

      Preferences.VPN.expirationDate.value = newestReceipt.expiresDate
      Preferences.VPN.freeTrialUsed.value = !newestReceipt.isTrialPeriod
      populateRegionDataIfNecessary()
      
      GRDSubscriptionManager.setIsPayingUser(true)
      receiptHasExpired?(false)
    }
  }

  // MARK: - STATE

  /// Lock to prevent user from spamming connect/disconnect button.
  public static var reconnectPending = false

  /// A state in which the vpn can be.
  public enum State {
    case notPurchased
    case purchased(enabled: Bool)

    case expired

    /// What view controller to show once user taps on `Enable VPN` button at one of places in the app.
    public var enableVPNDestinationVC: UIViewController? {
      switch self {
      case .notPurchased, .expired: return BuyVPNViewController(iapObserver: iapObserver)
      // Show nothing, the `Enable` button will now be used to connect and disconnect the vpn.
      case .purchased: return nil
      }
    }
  }

  /// Current state ot the VPN service.
  public static var vpnState: State {
    // User hasn't bought or restored the vpn yet.
    // If vpn plan expired, this preference is not set to nil but the date is set to year 1970
    // to force the UI to show expired state.
    if hasExpired == nil { return .notPurchased }
    
    if hasExpired == true {
      return .expired
    }

    return .purchased(enabled: isConnected)
  }

  /// Returns true if the user is connected to Brave's vpn at the moment.
  /// This will return true if the user is connected to other VPN.
  public static var isConnected: Bool {
    helper.isConnected()
  }

  /// Returns the last used hostname for the vpn configuration.
  /// Returns nil if the hostname string is empty(due to some error when configuring it for example).
  public static var hostname: String? {
    helper.mainCredential?.hostname
  }

  /// Whether the vpn subscription has expired.
  /// Returns nil if there has been no subscription yet (user never bought the vpn).
  public static var hasExpired: Bool? {
    guard let expirationDate = Preferences.VPN.expirationDate.value else { return nil }

    return expirationDate < Date()
  }

  /// Location of last used server for the vpn configuration.
  public static var serverLocation: String? {
    helper.mainCredential?.hostnameDisplayValue
  }

  /// Name of the purchased vpn plan.
  public static var subscriptionName: String {
    guard let credential = GRDSubscriberCredential.current() else {
      logAndStoreError("subscriptionName: failed to retrieve subscriber credentials")
      return ""
    }
    let productId = credential.subscriptionType
    
    switch productId {
    case VPNProductInfo.ProductIdentifiers.monthlySub:
      return Strings.VPN.vpnSettingsMonthlySubName
    case VPNProductInfo.ProductIdentifiers.yearlySub:
      return Strings.VPN.vpnSettingsYearlySubName
    default:
      assertionFailure("Can't get product id")
      return ""
    }
  }
  
  // MARK: - Actions

  /// Reconnects to the vpn.
  /// The vpn must be configured prior to that otherwise it does nothing.
  public static func reconnect() {
    if reconnectPending {
      logAndStoreError("Can't reconnect the vpn while another reconnect is pending.")
      return
    }

    reconnectPending = true

    connectToVPN()
  }

  /// Disconnects the vpn.
  /// The vpn must be configured prior to that otherwise it does nothing.
  public static func disconnect() {
    if reconnectPending {
      logAndStoreError("Can't disconnect the vpn while reconnect is still pending.")
      return
    }

    helper.disconnectVPN()
  }

  public static func connectToVPN(completion: ((Bool) -> Void)? = nil) {
    if isConnected {
      helper.disconnectVPN()
    }

    // 1. Existing user, check if credentials and connection are available.
    if GRDVPNHelper.activeConnectionPossible() {
      // just configure & connect, no need for 'first user' setup
      helper.configureAndConnectVPN { error, status in
        if let error = error {
          logAndStoreError("configureAndConnectVPN: \(error)")
        }
        
        reconnectPending = false
        completion?(status == .success)
      }
    } else {
      // New user or no credentials and have to remake them.
      helper.configureFirstTimeUserPostCredential(nil) { success, error in
        if let error = error {
          logAndStoreError("configureFirstTimeUserPostCredential \(error)")
        }
        
        reconnectPending = false
        completion?(success)
      }
    }
  }
  
  /// Attempts to reconfigure the vpn by migrating to a new server.
  /// The new hostname is chosen randomly.
  /// Automatic server is always used after the reset.
  /// This method disconnects from the vpn before reconfiguration is happening
  /// and reconnects automatically after reconfiguration is done.
  public static func reconfigureVPN(completion: ((Bool) -> Void)? = nil) {
    helper.forceDisconnectVPNIfNecessary()
    GRDVPNHelper.clearVpnConfiguration()

    connectToVPN() { status in
      completion?(status)
    }
  }

  public static func clearCredentials() {
    GRDKeychain.removeGuardianKeychainItems()
    GRDKeychain.removeSubscriberCredential(withRetries: 3)
  }

  // MARK: - Region selection

  /// Returns currently chosen region. Returns nil if automatic region is selected.
  public static var selectedRegion: GRDRegion? {
    helper.selectedRegion
  }
  
  /// Switched to use an automatic region, region closest to user location.
  public static func useAutomaticRegion() {
    helper.select(nil)
  }
  
  /// Returns true whether automatic region is selected.
  public static var isAutomaticRegion: Bool {
    helper.selectedRegion == nil
  }
  
  public static func changeVPNRegion(to region: GRDRegion?, completion: @escaping ((Bool) -> Void)) {
    helper.select(region)
    helper.configureFirstTimeUser(with: region) { success, error in
      if success {
        log.debug("Changed VPN region to \(region?.regionName ?? "default selection")")
        completion(true)
      } else {
        log.debug("connection failed: \(String(describing: error))")
        completion(false)
      }
    }
  }
  
  public static func populateRegionDataIfNecessary () {
    serverManager.getRegionsWithCompletion { regions in
      self.regions = regions
    }
  }
  
  // MARK: - VPN Alerts and notifications
  private static func shouldProcessVPNAlerts() -> Bool {
    switch vpnState {
    case .purchased(let enabled):
      return enabled
    default:
      return false
    }
  }

  public static func processVPNAlerts() {
    if !shouldProcessVPNAlerts() { return }
    
    Task {
      let (data, success, error) = await GRDGatewayAPI().events()
      if !success {
        log.error("VPN getEvents call failed")
        if let error = error {
          log.warning(error)
        }
        
        return
      }
      
      guard let alertsData = data["alerts"] else {
        log.error("Failed to unwrap json for vpn alerts")
        return
      }
      
      do {
        let dataAsJSON =
        try JSONSerialization.data(withJSONObject: alertsData, options: [.fragmentsAllowed])
        let decoded = try JSONDecoder().decode([BraveVPNAlertJSONModel].self, from: dataAsJSON)
        
        BraveVPNAlert.batchInsertIfNotExists(alerts: decoded)
      } catch {
        log.error("Failed parsing vpn alerts data")
      }
    }
  }
  
  public static func sendVPNWorksInBackgroundNotification() {
    switch vpnState {
    case .expired, .notPurchased:
      break
    case .purchased(let enabled):
      if !enabled || Preferences.VPN.vpnWorksInBackgroundNotificationShowed.value {
        break
      }

      let center = UNUserNotificationCenter.current()
      let notificationId = "vpnWorksInBackgroundNotification"

      center.requestAuthorization(options: [.provisional, .alert, .sound, .badge]) { granted, error in
        if let error = error {
          log.error("Failed to request notifications permissions: \(error)")
          return
        }

        if !granted {
          log.info("Not authorized to schedule a notification")
          return
        }

        center.getPendingNotificationRequests { requests in
          if requests.contains(where: { $0.identifier == notificationId }) {
            // Already has one scheduled no need to schedule again.
            // Should not happens since we push the notification right away.
            return
          }

          let content = UNMutableNotificationContent()
          content.title = Strings.VPN.vpnBackgroundNotificationTitle
          content.body = Strings.VPN.vpnBackgroundNotificationBody

          // Empty `UNNotificationTrigger` sends the notification right away.
          let request = UNNotificationRequest(identifier: notificationId, content: content,
                                              trigger: nil)

          center.add(request) { error in
            if let error = error {
              log.error("Failed to add notification: \(error)")
              return
            }

            Preferences.VPN.vpnWorksInBackgroundNotificationShowed.value = true
          }
        }
      }
    }
  }
  
  // MARK: - Error Handling
  
  /// Stores a in-memory list of vpn errors encountered during current browsing session.
  public private(set) static var errorLog = [(date: Date, message: String)]()
  private static let errorLogQueue = DispatchQueue(label: "com.brave.errorLogQueue")

  /// Prints out the error to the logger and stores it in a in memory array.
  /// This can be further used for a customer support form.
  private static func logAndStoreError(_ message: String, printToConsole: Bool = true) {
    if printToConsole {
      log.error(message)
    }

    // Extra safety here in case the log is spammed by many messages.
    // Early logs are more valuable for debugging, we do not rotate them with new entries.
    errorLogQueue.async {
      if errorLog.count < 1000 {
        errorLog.append((Date(), message))
      }
    }
  }
    
  // MARK: - Migration
  public static func migrateV1Credentials() {
    // Moving Brave VPN v1 users to v2 type of credentials.
    GRDCredentialManager.migrateKeychainItemsToGRDCredential()
  }
}
