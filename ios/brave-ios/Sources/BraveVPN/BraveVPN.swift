// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Data
import GuardianConnect
import NetworkExtension
import Preferences
import Shared
import UIKit
import os.log

/// A static class to handle all things related to the Brave VPN service.
public class BraveVPN {

  private static let serverManager = GRDServerManager()
  private static let tunnelManager = GRDTunnelManager()

  public static let housekeepingApi = GRDHousekeepingAPI()
  public static let helper = GRDVPNHelper.sharedInstance()

  public static let iapObserver = BraveVPNInAppPurchaseObserver()

  private static let connectionName = "Brave Firewall + VPN"  // Non translatable

  // MARK: - Initialization

  /// This class is supposed to act as a namespace, disabling possibility of creating an instance of it.
  @available(*, unavailable)
  init() {}

  /// Initialize the vpn service. It should be called even if the user hasn't bought the vpn yet.
  /// This function can have side effects if the receipt has expired(removes the vpn connection then).
  public static func initialize(customCredential: BraveVPNSkusCredential?) {
    @Sendable func clearConfiguration() {
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

    helper.dummyDataForDebugging = !AppConstants.isOfficialBuild
    helper.tunnelLocalizedDescription = connectionName
    helper.grdTunnelProviderManagerLocalizedDescription = connectionName
    helper.tunnelProviderBundleIdentifier = AppInfo.baseBundleIdentifier + ".BraveWireGuard"
    helper.appGroupIdentifier = AppInfo.sharedContainerIdentifier

    if case .notPurchased = vpnState {
      // Unlikely if user has never bought the vpn, we clear vpn config here for safety.
      clearConfiguration()
      return
    }

    Task {
      do {
        let receiptResponse = try await validateReceiptData()
        // Clear configuration if only if the receipt response is expired (not retryPeriod)
        if receiptResponse?.status == .expired {
          clearConfiguration()
          logAndStoreError("Receipt expired")
        }
      } catch {
        logAndStoreError(String(describing: error))
      }
    }
  }

  // MARK: - STATE

  /// A state in which the vpn can be.
  public enum State: Equatable {
    case notPurchased
    case purchased(enabled: Bool)

    case expired

    /// What view controller to show once user taps on `Enable VPN` button at one of places in the app.
    public var enableVPNDestinationVC: BuyVPNViewController? {
      switch self {
      case .notPurchased, .expired: return BuyVPNViewController(iapObserver: iapObserver)
      // Show nothing, the `Enable` button will now be used to connect and disconnect the vpn.
      case .purchased: return nil
      }
    }
  }

  /// Lock to prevent user from spamming connect/disconnect button.
  public static var reconnectPending = false

  /// Returns true if the app store receipt is in sandbox mode.
  /// This can typically let us know whether a Testflight build is used or not.
  /// Keep in mind this function may not work correctly for future iOS builds.
  /// Apple prefers to validate the receipt by using a server.
  public static var isSandbox: Bool {
    Bundle.main.appStoreReceiptURL?.lastPathComponent == "sandboxReceipt"
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
  public static var serverLocation: (hostName: String?, isoCode: String?) {
    let mainCredential = helper.mainCredential

    return (mainCredential?.hostnameDisplayValue, activatedRegion?.countryISOCode)
  }

  /// Detailed Location of last used server for the vpn configuration.
  public static var serverLocationDetailed: (city: String?, country: String?) {
    guard let serverLocation = serverLocation.hostName else {
      return (nil, nil)
    }

    // Spliting the location format into country and city
    let locationComponents = serverLocation.split(separator: ",")

    if locationComponents.count == 2 {
      let city = locationComponents[safe: 0] ?? ""
      let country = locationComponents[safe: 1] ?? ""

      return (
        city.trimmingCharacters(in: .whitespaces), country.trimmingCharacters(in: .whitespaces)
      )
    } else {
      return (nil, nil)
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
        fetchLastUsedRegionDetail { _, _ in
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
        postCredential: nil
      ) { success, error in
        if let error = error {
          logAndStoreError("configureFirstTimeUserPostCredential \(error)")
        } else {
          helper.ikev2VPNManager.removeFromPreferences()
        }

        reconnectPending = false

        // First time user will connect automatic region - detail is pulled
        fetchLastUsedRegionDetail { _, _ in
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

    connectToVPN { status in
      completion?(status)
    }
  }

  public static func changePreferredTransportProtocol(
    with transportProtocol: TransportProtocol,
    completion: ((Bool) -> Void)? = nil
  ) {
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

  // MARK: - Credentials

  public static func setCustomVPNCredential(_ credential: BraveVPNSkusCredential) {
    GRDSubscriptionManager.setIsPayingUser(true)
    populateRegionDataIfNecessary()

    let dict: NSMutableDictionary =
      [
        "brave-vpn-premium-monthly-pass": credential.guardianCredential,
        "brave-payments-env": credential.environment,
        "validation-method": "brave-premium",
      ]
    helper.customSubscriberCredentialAuthKeys = dict
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

  // MARK: - Notifications And Alerts

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

      center.requestAuthorization(options: [.provisional, .alert, .sound, .badge]) {
        granted,
        error in
        if let error = error {
          Logger.module.error(
            "Failed to request notifications permissions: \(error.localizedDescription)"
          )
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
          let request = UNNotificationRequest(
            identifier: notificationId,
            content: content,
            trigger: nil
          )

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

  // MARK: - Error Handling

  /// Stores a in-memory list of vpn errors encountered during current browsing session.
  public private(set) static var errorLog = [(date: Date, message: String)]()
  private static let errorLogQueue = DispatchQueue(label: "com.brave.errorLogQueue")

  /// Prints out the error to the logger and stores it in a in memory array.
  /// This can be further used for a customer support form.
  public static func logAndStoreError(_ message: String, printToConsole: Bool = true) {
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
