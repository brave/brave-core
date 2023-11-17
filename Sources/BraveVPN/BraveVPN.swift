// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import Preferences
import NetworkExtension
import Data
import GuardianConnect
import os.log

/// A static class to handle all things related to the Brave VPN service.
public class BraveVPN {
  private static let housekeepingApi = GRDHousekeepingAPI()
  private static let helper = GRDVPNHelper.sharedInstance()
  private static let serverManager = GRDServerManager()
  private static let tunnelManager = GRDTunnelManager()
  
  public static let iapObserver = IAPObserver()

  /// List of regions the VPN can connect to.
  /// This list is not static and should be refetched every now and then.
  static var regions: [GRDRegion] = []
  
  /// Record last used region
  /// It is used to hold details of the region when automatic selection is used
  static var lastKnownRegion: GRDRegion?
  
  // Non translatable
  private static let connectionName = "Brave Firewall + VPN"

  // MARK: - Initialization

  /// This class is supposed to act as a namespace, disabling possibility of creating an instance of it.
  @available(*, unavailable)
  init() {}

  /// Initialize the vpn service. It should be called even if the user hasn't bought the vpn yet.
  /// This function can have side effects if the receipt has expired(removes the vpn connection then).
  public static func initialize(customCredential: SkusVPNCredential?) {
    func clearConfiguration() {
      GRDVPNHelper.clearVpnConfiguration()
      clearCredentials()

      NEVPNManager.shared().removeFromPreferences { error in
        if let error = error {
          logAndStoreError("Remove vpn error: \(error.localizedDescription)")
        }
      }
    }

    if let customCredential = customCredential {
      if hasExpired == true {
        clearConfiguration()
        logAndStoreError("Skus credential expired, resetting configuration")
        return
      }
      
      setCustomVPNCredential(customCredential)
    }
    
    helper.verifyMainCredentials { valid, error in
      logAndStoreError("Initialize credentials valid: \(valid)")
      if let error = error {
        logAndStoreError("Initialize credentials error: \(error)")
      }
    }

    helper.dummyDataForDebugging = !AppConstants.buildChannel.isPublic
    helper.tunnelLocalizedDescription = connectionName
    helper.grdTunnelProviderManagerLocalizedDescription = connectionName
    helper.tunnelProviderBundleIdentifier = AppInfo.baseBundleIdentifier + ".BraveWireGuard"
    helper.appGroupIdentifier = AppInfo.sharedContainerIdentifier

    if case .notPurchased = vpnState {
      // Unlikely if user has never bought the vpn, we clear vpn config here for safety.
      clearConfiguration()
      return
    }
    
    BraveVPN.validateReceiptData() { receiptResponse in
      // Clear configuration if only if the receipt response is expired (not retryPeriod)
      if receiptResponse?.status == .expired {
        clearConfiguration()
        logAndStoreError("Receipt expired")
        return
      }
    }
  }
  
  public static var receipt: String? {
    guard let receiptUrl = Bundle.main.appStoreReceiptURL,
          let receipt = try? Data(contentsOf: receiptUrl).base64EncodedString else { return nil }
    
    return receipt
  }
  
  /// Returns true if the app store receipt is in sandbox mode.
  /// This can typically let us know whether a Testflight build is used or not.
  /// Keep in mind this function may not work correctly for future iOS builds.
  /// Apple prefers to validate the receipt by using a server.
  public static var isSandbox: Bool {
    Bundle.main.appStoreReceiptURL?.lastPathComponent == "sandboxReceipt"
  }
  
  public static func setCustomVPNCredential(_ credential: SkusVPNCredential) {
    GRDSubscriptionManager.setIsPayingUser(true)
    populateRegionDataIfNecessary()
    
    let dict: NSMutableDictionary =
    ["brave-vpn-premium-monthly-pass": credential.guardianCredential,
     "brave-payments-env": credential.environment,
     "validation-method": "brave-premium"]
    helper.customSubscriberCredentialAuthKeys = dict
  }
  
  /// Connects to Guardian's server to validate locally stored receipt.
  /// Returns ReceiptResponse whoich hold information about status of receipt expiration etc
  public static func validateReceiptData(receiptResponse: ((ReceiptResponse?) -> Void)? = nil) {
    guard let receipt = receipt,
          let bundleId = Bundle.main.bundleIdentifier else {
      receiptResponse?(nil)
      return
    }
    
    if Preferences.VPN.skusCredential.value != nil {
      // Receipt verification applies to Apple's IAP only,
      // if we detect Brave's SKU token we should not look at Apple's receipt.
      return
    }
    
    housekeepingApi.verifyReceiptData(receipt, bundleId: bundleId) { response, error in
      if let error = error {
        // Error while fetching receipt response, the variations of error can be listed
        // No App Store receipt data present
        // Failed to retrieve receipt data from server
        // Failed to decode JSON response data
        receiptResponse?(nil)
        logAndStoreError("Call for receipt verification failed: \(error.localizedDescription)")
        return
      }
      
      guard let response = response else {
        receiptResponse?(nil)
        logAndStoreError("Receipt verification response is empty")
        return
      }
      
      let receiptResponseItem = GRDIAPReceiptResponse(withReceiptResponse: response)
      let processedReceiptDetail = BraveVPN.processReceiptResponse(receiptResponseItem: receiptResponseItem)

      switch processedReceiptDetail.status {
      case .expired:
        Preferences.VPN.expirationDate.value = Date(timeIntervalSince1970: 1)
        Preferences.VPN.originalTransactionId.value = nil
        logAndStoreError("VPN Subscription LineItems are empty subscription expired", printToConsole: false)
      case .active, .retryPeriod:
        if let expirationDate = processedReceiptDetail.expiryDate {
          Preferences.VPN.expirationDate.value = expirationDate
        }
        
        if let gracePeriodExpirationDate = processedReceiptDetail.graceExpiryDate {
          Preferences.VPN.gracePeriodExpirationDate.value = gracePeriodExpirationDate
        }
        
        Preferences.VPN.freeTrialUsed.value = !processedReceiptDetail.isInTrialPeriod

        populateRegionDataIfNecessary()
        GRDSubscriptionManager.setIsPayingUser(true)
      }
      
      Preferences.VPN.vpnReceiptStatus.value = processedReceiptDetail.status.rawValue
      
      receiptResponse?(processedReceiptDetail)
    }
  }
  
  public static func processReceiptResponse(receiptResponseItem: GRDIAPReceiptResponse) -> ReceiptResponse {
    guard let newestReceiptLineItem = receiptResponseItem.lineItems.sorted(by: { $0.expiresDate > $1.expiresDate }).first else {
      if let originalTransactionId = Preferences.VPN.originalTransactionId.value {
        let lineItemMetaDataForOriginalId =  receiptResponseItem.lineItemsMetadata.first(
          where: { Int($0.originalTransactionId) ?? 00 == originalTransactionId })
        
        if let metaData = lineItemMetaDataForOriginalId, metaData.gracePeriodExpiresDate > Date() {
          let response = ReceiptResponse(
            status: .retryPeriod,
            expiryReason: ReceiptResponse.ExpirationIntent(rawValue: Int(metaData.expirationIntent)) ?? .none,
            graceExpiryDate: metaData.gracePeriodExpiresDate,
            isInTrialPeriod: false,
            autoRenewEnabled: false)
          
          return response
        }
      }

      return ReceiptResponse(status: .expired)
    }

    let lineItemMetaData =  receiptResponseItem.lineItemsMetadata.first(
      where: { Int($0.originalTransactionId) ?? 00 == newestReceiptLineItem.originalTransactionId })

    // Original transaction id of last active subscription in order to detect grace period
    Preferences.VPN.originalTransactionId.value = newestReceiptLineItem.originalTransactionId
    
    guard let metadata = lineItemMetaData else {
      logAndStoreError("No line item meta data - can not happen")
      return ReceiptResponse(status: .active)
    }

    let receiptStatus: ReceiptResponse.Status = lineItemMetaData?.isInBillingRetryPeriod == true ? .retryPeriod : .active
    // Expiration Intent is unsigned
    let expirationIntent = ReceiptResponse.ExpirationIntent(rawValue: Int(metadata.expirationIntent)) ?? .none
    // 0 is for turned off renewal, 1 is subscription renewal
    let autoRenewEnabled = metadata.autoRenewStatus == 1

    let response = ReceiptResponse(
      status: receiptStatus,
      expiryReason: expirationIntent,
      expiryDate: newestReceiptLineItem.expiresDate,
      graceExpiryDate: metadata.gracePeriodExpiresDate,
      isInTrialPeriod: newestReceiptLineItem.isTrialPeriod,
      autoRenewEnabled: autoRenewEnabled)
    
    return response
  }

  // MARK: - STATE

  /// Lock to prevent user from spamming connect/disconnect button.
  public static var reconnectPending = false

  /// A state in which the vpn can be.
  public enum State: Equatable {
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
  private static var hasExpired: Bool? {
    guard let expirationDate = Preferences.VPN.expirationDate.value else { return nil }

    if expirationDate < Date() {
      if let gracePeriodExpirationDate = Preferences.VPN.gracePeriodExpirationDate.value {
        return gracePeriodExpirationDate < Date()
      }
      
      return true
    }
    
    return false
  }

  /// Location of last used server for the vpn configuration.
  public static var serverLocation: String? {
    helper.mainCredential?.hostnameDisplayValue
  }

  /// Type of the vpn subscription
  public enum SubscriptionType: Equatable {
    case monthly, yearly, other
  }
  
  /// Type of the active purchased vpn plan
  public static var activeSubscriptionType: SubscriptionType {
    guard let credential = GRDSubscriberCredential.current() else {
      logAndStoreError("subscriptionName: failed to retrieve subscriber credentials")
      return .other
    }
    let productId = credential.subscriptionType
    
    switch productId {
    case VPNProductInfo.ProductIdentifiers.monthlySub:
      return .monthly
    case VPNProductInfo.ProductIdentifiers.yearlySub:
      return .yearly
    default:
      return .other
    }
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
    case VPNProductInfo.ProductIdentifiers.monthlySubSKU:
      return Strings.VPN.vpnSettingsMonthlySubName
    default:
      assertionFailure("Can't get product id")
      return ""
    }
  }
  
  // MARK: - ReceiptResponse
  
  public struct ReceiptResponse {
    public enum Status: Int {
      case active, expired, retryPeriod
    }
    
    enum ExpirationIntent: Int {
      case none, cancelled, billingError, priceIncreaseConsent, notAvailable, unknown
    }
    
    var status: Status
    var expiryReason: ExpirationIntent = .none
    var expiryDate: Date?
    var graceExpiryDate: Date?

    var isInTrialPeriod: Bool = false
    var autoRenewEnabled: Bool = false
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
  public static func disconnect(skipChecks: Bool = false) {
    if skipChecks {
      helper.disconnectVPN()
      return
    }
    
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
        
        // Re-connected user should update last used region - detail is pulled
        fetchLastUsedRegionDetail() { _, _ in
          DispatchQueue.main.async {
            completion?(status == .success)
          }
        }
      }
    } else {
      // Setting User preferred Transport Protocol to WireGuard
      // In order to easily fetch and change in settings later
      GRDTransportProtocol.setUserPreferred(.wireGuard)
      
      // New user or no credentials and have to remake them.
      helper.configureFirstTimeUser(
        for: GRDTransportProtocol.getUserPreferredTransportProtocol(),
        postCredential: nil) { success, error in
        if let error = error {
          logAndStoreError("configureFirstTimeUserPostCredential \(error)")
        } else {
          helper.ikev2VPNManager.removeFromPreferences()
        }
         
        reconnectPending = false
          
        // First time user will connect automatic region - detail is pulled
        fetchLastUsedRegionDetail() { _, _ in
          DispatchQueue.main.async {  
            completion?(success)
          }
        }
      
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
  
  public static func changePreferredTransportProtocol(with transportProtocol: TransportProtocol, completion: ((Bool) -> Void)? = nil) {
    helper.forceDisconnectVPNIfNecessary()
    GRDVPNHelper.clearVpnConfiguration()
    
    GRDTransportProtocol.setUserPreferred(transportProtocol)

    // New user or no credentials and have to remake them.
    helper.configureFirstTimeUser(for: transportProtocol, postCredential: nil) { success, error in
      if let error = error {
        logAndStoreError("Change Preferred transport FirstTimeUserPostCredential \(error)")
      } else {
        removeConfigurationFromPreferences(for: transportProtocol)
      }
      
      reconnectPending = false
      completion?(success)
    }
    
    func removeConfigurationFromPreferences(for transportProtocol: TransportProtocol) {
      switch transportProtocol {
      case .wireGuard:
        helper.ikev2VPNManager.removeFromPreferences()
      case .ikEv2:
        tunnelManager.removeTunnel()
      default:
        return
      }
    }
  }

  public static func clearCredentials() {
    GRDKeychain.removeGuardianKeychainItems()
    GRDKeychain.removeSubscriberCredential(withRetries: 3)
    
    clearSkusCredentials(includeExpirationDate: true)
  }
  
  public static func clearSkusCredentials(includeExpirationDate: Bool) {
    Preferences.VPN.skusCredential.reset()
    Preferences.VPN.skusCredentialDomain.reset()
    
    if includeExpirationDate {
      Preferences.VPN.expirationDate.reset()
      Preferences.VPN.gracePeriodExpirationDate.reset()
    }
  }

  // MARK: - Region selection

  /// Returns currently chosen region. Returns nil if automatic region is selected.
  public static var selectedRegion: GRDRegion? {
    helper.selectedRegion
  }
  
  /// Return the region last activated with the details
  /// It will give region details for automatic
  public static var activatedRegion: GRDRegion? {
    helper.selectedRegion ?? lastKnownRegion
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
    if isConnected {
      helper.disconnectVPN()
    }
    
    helper.select(region)
    
    // The preferred tunnel has to be used for configuration
    // Otherwise faulty configuration will be added while connecting
    let activeTunnelProtocol = GRDTransportProtocol.getUserPreferredTransportProtocol()
    
    helper.configureFirstTimeUser(for: activeTunnelProtocol, with: region) { success, error in
      let subcredentials = "Credentials \(GRDKeychain.getPasswordString(forAccount: kKeychainStr_SubscriberCredential) ?? "Empty")"
      
      if success {
        Logger.module.debug("Changed VPN region to \(region?.regionName ?? "default selection")")
        completion(true)
      } else {
        Logger.module.debug("Connection failed: \(error ?? "nil")")
        Logger.module.debug("Region change connection failed for subcredentials \(subcredentials)")
        completion(false)
      }
    }
  }
  
  public static func populateRegionDataIfNecessary () {
    serverManager.regions { regions, _ in
      self.regions = regions ?? []
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
        Logger.module.error("VPN getEvents call failed")
        if let error = error {
          Logger.module.warning("\(error)")
        }
        
        return
      }
      
      guard let alertsData = data["alerts"] else {
        Logger.module.error("Failed to unwrap json for vpn alerts")
        return
      }
      
      do {
        let dataAsJSON =
        try JSONSerialization.data(withJSONObject: alertsData, options: [.fragmentsAllowed])
        let decoded = try JSONDecoder().decode([BraveVPNAlertJSONModel].self, from: dataAsJSON)
        
        BraveVPNAlert.batchInsertIfNotExists(alerts: decoded)
      } catch {
        Logger.module.error("Failed parsing vpn alerts data")
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
          Logger.module.error("Failed to request notifications permissions: \(error.localizedDescription)")
          return
        }

        if !granted {
          Logger.module.info("Not authorized to schedule a notification")
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
              Logger.module.error("Failed to add notification: \(error.localizedDescription)")
              return
            }

            Preferences.VPN.vpnWorksInBackgroundNotificationShowed.value = true
          }
        }
      }
    }
  }
  
  /// The function that fetched the last used region details from timezones
  /// It used to get details of Region when Automatic Region is used
  /// Otherwise the region detail items will be empty
  /// - Parameter completion: completion block that returns region with details or error
  public static func fetchLastUsedRegionDetail(_ completion: ((GRDRegion?, Bool) -> Void)? = nil) {
    switch vpnState {
    case .expired, .notPurchased:
      break
    case .purchased(_):
      housekeepingApi.requestTimeZonesForRegions { timeZones, success, responseStatusCode in
        guard success, let timeZones = timeZones else {
          logAndStoreError(
            "Failed to get timezones while fetching region: \(responseStatusCode)",
            printToConsole: true)
          completion?(nil, false)
          
          return
        }
        
        let region = GRDServerManager.localRegion(fromTimezones: timeZones)
        completion?(region, true)
        lastKnownRegion = region
      }
    }
  }
  
  // MARK: - Promotion
  
  /// Editing product promotion order first yearly and monthly after
  @MainActor public static func updateStorePromotionOrder() async {
    let storePromotionController = SKProductStorePromotionController.default()
    // Fetch Products
    guard let yearlyProduct = VPNProductInfo.yearlySubProduct,
          let monthlyProduct = VPNProductInfo.monthlySubProduct else {
      Logger.module.debug("Found empty while fetching SKProducts for promotion order")
      return
    }
    
    // Update the order
    do {
      try await storePromotionController.update(promotionOrder: [yearlyProduct, monthlyProduct])
    } catch {
      Logger.module.debug("Error while opdating product promotion order ")
    }
  }
  
  /// Hiding Store pormotion if the active subscription for the type
  @MainActor public static func hideActiveStorePromotion() async {
    let storePromotionController = SKProductStorePromotionController.default()
    
    // Fetch Products
    guard let yearlyProduct = VPNProductInfo.yearlySubProduct,
          let monthlyProduct = VPNProductInfo.monthlySubProduct else {
      Logger.module.debug("Found empty while fetching SKProducts for promotion order")
      return
    }
    
    // No promotion for VPN is purchased through website side
    if Preferences.VPN.skusCredential.value != nil {
      await hideSubscriptionType(yearlyProduct)
      await hideSubscriptionType(monthlyProduct)
      
      return
    }
    
    // Hide the promotion
    let activeSubscriptionType = BraveVPN.activeSubscriptionType
    
    switch activeSubscriptionType {
    case .monthly:
      await hideSubscriptionType(monthlyProduct)
    case .yearly:
      await hideSubscriptionType(yearlyProduct)
    default:
      break
    }
    
    func hideSubscriptionType(_ product: SKProduct) async {
      do {
        try await storePromotionController.update(promotionVisibility: .hide, for: product)
      } catch {
        Logger.module.debug("Error while opdating product promotion order ")
      }
    }
  }
  
  public static func activatePaymentTypeForStoredPromotion(savedPayment: SKPayment?) {
    if let payment = savedPayment {
      SKPaymentQueue.default().add(payment)
    }
      
    iapObserver.savedPayment = nil
  }
  
  // MARK: - Error Handling
  
  /// Stores a in-memory list of vpn errors encountered during current browsing session.
  public private(set) static var errorLog = [(date: Date, message: String)]()
  private static let errorLogQueue = DispatchQueue(label: "com.brave.errorLogQueue")

  /// Prints out the error to the logger and stores it in a in memory array.
  /// This can be further used for a customer support form.
  private static func logAndStoreError(_ message: String, printToConsole: Bool = true) {
    if printToConsole {
      Logger.module.error("\(message)")
    }

    // Extra safety here in case the log is spammed by many messages.
    // Early logs are more valuable for debugging, we do not rotate them with new entries.
    errorLogQueue.async {
      if errorLog.count < 1000 {
        errorLog.append((Date(), message))
      }
    }
  }
}
