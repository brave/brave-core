// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveShared
import NetworkExtension

private let log = Logger.browserLogger

/// A static class to handle all things related to the Brave VPN service.
class BraveVPN {
        
    private static let housekeepingApi = GRDHousekeepingAPI()
    private static let helper = GRDVPNHelper()
    private static let serverManager = GRDServerManager()
    
    // MARK: - Initialization
    
    /// This class is supposed to act as a namespace, disabling possibility of creating an instance of it.
    @available(*, unavailable)
    init() {}
    
    /// Initialize the vpn service. It should be called even if the user hasn't bought the vpn yet.
    /// This function can have side effects if the receipt has expired(removes the vpn connection then).
    static func initialize() {
        // The vpn can live outside of the app.
        // When the app loads we should load it from preferences to track its state.
        NEVPNManager.shared().loadFromPreferences { error in
            if let error = error {
                logAndStoreError("Failed to load vpn conection: \(error)")
            }
            
            if case .notPurchased = vpnState {
                // Unlikely if user has never bought the vpn, we clear vpn config here for safety.
                BraveVPN.clearConfiguration()
                return
            }
            
            // We validate the current receipt at the start to know if the subscription has expirerd.
            BraveVPN.validateReceipt() { expired in
                if expired == true {
                    BraveVPN.clearConfiguration()
                    logAndStoreError("Receipt expired")
                    return
                }
                
                if isConnected {
                    GRDGatewayAPI.shared().getServerStatus { completion in
                        if completion.responseStatus == .serverOK {
                            log.debug("VPN server status OK")
                            return
                        }
                        
                        logAndStoreError("VPN server status failure, migrating to new host")
                        disconnect()
                        reconnect()
                    }
                }
            }
        }
    }
    
    // MARK: - STATE
    
    /// How many times we should retry to configure the vpn.
    private static let configMaxRetryCount = 4
    /// Current number of retries.
    private static var configRetryCount = 0
    
    /// Sometimes restoring a purchase is triggered multiple times which leads to calling vpn.configure multiple times.
    /// This flags prevents configuring the vpn more than once.
    private static var firstTimeUserConfigPending = false
    
    /// Lock to prevent user from spamming connect/disconnect button.
    static var reconnectPending = false
    
    /// Status of creating vpn credentials on Guardian's servers.
    enum VPNUserCreationStatus {
        case success
        case error(type: VPNUserCreationError)
    }
    
    /// Errors that can happen when a vpn's user credentials are created on Guardian's servers.
    /// Each error has a number associated to it for easier debugging.
    enum VPNUserCreationError {
        case connectionProblems
        case provisioning
        case unknown
    }
    
    enum VPNConfigStatus {
        case success
        case error(type: VPNConfigErrorType)
    }
    
    /// Errors that can happen when trying to estabilish a vpn connection.
    /// Each error has a number associated to it for easier debugging.
    enum VPNConfigErrorType {
        case saveConfigError
        case loadConfigError
        /// User tapped 'Don't allow' when save-vpn-config prompt is shown.
        case permissionDenied
    }
    
    enum VPNPurchaseError {
        /// Returned when the receipt sent to the server is expired. This happens for sandbox users only.
        case receiptExpired
        /// Purchase failed on Apple's side or canceled by user.
        case purchaseFailed
    }
    
    /// A state in which the vpn can be.
    enum State {
        case notPurchased
        /// Purchased but not installed
        case purchased
        /// Purchased and installed
        case installed(enabled: Bool)
        
        case expired(enabled: Bool)
        
        /// What view controller to show once user taps on `Enable VPN` button at one of places in the app.
        var enableVPNDestinationVC: UIViewController? {
            switch self {
            case .notPurchased, .expired: return BuyVPNViewController()
            case .purchased: return InstallVPNViewController()
            // Show nothing, the `Enable` button will now be used to connect and disconnect the vpn.
            case .installed: return nil
            }
        }
    }
    
    /// Current state ot the VPN service.
    static var vpnState: State {
        // User hasn't bought or restored the vpn yet.
        // If vpn plan expired, this preference is not set to nil but the date is set to year 1970
        // to force the UI to show expired state.
        if Preferences.VPN.expirationDate.value == nil { return .notPurchased }
        
        if hasExpired == true {
            return .expired(enabled: NEVPNManager.shared().isEnabled)
        }
        
        // The app has not expired yet and nothing is in keychain.
        // This means user has reinstalled the app while their vpn plan is still active.
        if GRDKeychain.getPasswordString(forAccount: kKeychainStr_SubscriberCredential) == nil {
            return .notPurchased
        }
        
        // No VPN config set means the user could buy the vpn but hasn't gone through the second screen
        // to install the vpn and connect to a server.
        if NEVPNManager.shared().connection.status == .invalid { return .purchased }
        
        return .installed(enabled: isConnected)
    }
    
    /// Returns true if the user is connected to Brave's vpn at the moment.
    /// This will return true if the user is connected to other VPN.
    static var isConnected: Bool {
        NEVPNManager.shared().connection.status == .connected
    }
    
    /// Returns the last used hostname for the vpn configuration.
    /// Returns nil if the hostname string is empty(due to some error when configuring it for example).
    static var hostname: String? {
        UserDefaults.standard.string(forKey: kGRDHostnameOverride)
    }
    
    /// Whether the vpn subscription has expired.
    /// Returns nil if there has been no subscription yet (user never bought the vpn).
    static var hasExpired: Bool? {
        guard let expirationDate = Preferences.VPN.expirationDate.value else { return nil }
        
        return expirationDate < Date()
    }
    
    /// Location of last used server for the vpn configuration.
    static var serverLocation: String? {
        guard let serverHostname = hostname else { return nil }
        return Preferences.VPN.vpnHostDisplayName.value
            ?? GRDVPNHelper.serverLocation(forHostname: serverHostname)
    }
    
    /// Name of the purchased vpn plan.
    static var subscriptionName: String {
        guard let credentialString =
                GRDKeychain.getPasswordString(forAccount: kKeychainStr_SubscriberCredential) else {
            logAndStoreError("subscriptionName: failed to retrieve subscriber credentials")
            return ""
        }
        let credential = GRDSubscriberCredential(subscriberCredential: credentialString)
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
    
    /// Stores a in-memory list of vpn errors encountered during current browsing session.
    private(set) static var errorLog = [(date: Date, message: String)]()
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
    
    // MARK: - Actions
    
    /// Reconnects to the vpn. Checks for server health first, if it's bad it tries to connect to another host.
    /// The vpn must be configured prior to that otherwise it does nothing.
    static func reconnect() {
        if reconnectPending {
            logAndStoreError("Can't reconnect the vpn while another reconnect is pending.")
            return
        }
        
        reconnectPending = true
        
        guard let credentialString =
            GRDKeychain.getPasswordString(forAccount: kKeychainStr_SubscriberCredential) else {
            reconnectPending = false
            logAndStoreError("reconnect: failed to retrieve subscriber credentials")
            return
        }
        
        let credential = GRDSubscriberCredential(subscriberCredential: credentialString)
        
        let tokenExpirationDate = Date(timeIntervalSince1970: TimeInterval(credential.tokenExpirationDate))
        
        if Date() > tokenExpirationDate {
            guard let host = hostname else {
                reconnectPending = false
                logAndStoreError("failed to retrieve hostname")
                return
            }
            
            createNewSubscriberCredential(for: host) { status in
                switch status {
                case .success:
                    connectOrMigrateToNewNode { completion in
                        log.debug("vpn configuration status: \(completion)")
                        reconnectPending = false
                    }
                case .error:
                    logAndStoreError("Creating new credentials failed when tried to reconnect to the vpn.")
                    reconnectPending = false
                }
            }
        } else {
            connectOrMigrateToNewNode { completion in
                switch completion {
                case .error(let type):
                    logAndStoreError("Failed to reconnect, error: \(type)")
                case .success:
                    log.debug("Reconnected to the VPN")
                }
                
                reconnectPending = false
            }
        }
    }
    
    /// Disconnects the vpn.
    /// The vpn must be configured prior to that otherwise it does nothing.
    static func disconnect() {
        if reconnectPending {
            logAndStoreError("Can't disconnect the vpn while reconnect is still pending.")
            return
        }
        
        helper.disconnectVPN()
    }
    
    /// Connects to Guardian's server to validate locally stored receipt.
    /// Returns true if the receipt expired, false if not or nil if expiration status can't be determined.
    static func validateReceipt(receiptHasExpired: ((Bool?) -> Void)? = nil) {
        /// A helper struct to deal with unwrapping receipt json and extracting properites we are interested in.
        struct _Receipt: Decodable {
            let expireDate: Date
            let isTrialPeriod: Bool?

            private enum CodingKeys: String, CodingKey {
                case expiresDateMs = "expires_date_ms"
                case isTrialPeriod = "is_trial_period"
            }
            
            init(from decoder: Decoder) throws {
                let container = try decoder.container(keyedBy: CodingKeys.self)
                let expireDateString = try container.decode(String.self, forKey: .expiresDateMs)
                if let expireDateTimeIntervalMs = TimeInterval(expireDateString) {
                    // Expiration date comes in milisecond while the iOS time interval uses seconds.
                    self.expireDate = Date(timeIntervalSince1970: expireDateTimeIntervalMs / 1000.0)
                } else {
                    throw "Casting `expireDateString` failed"
                }
                
                if let isTrialString = try container.decodeIfPresent(String.self, forKey: .isTrialPeriod) {
                    self.isTrialPeriod = Bool(isTrialString)
                } else {
                    self.isTrialPeriod = nil
                }
            }
        }
        
        housekeepingApi.verifyReceipt { validSubscriptions, success, error in
            if !success {
                // Api call for receipt verification failed,
                // we do not know if the receipt has expired or not.
                receiptHasExpired?(nil)
                logAndStoreError("Api call for receipt verification failed")
                return
            }

            guard let validSubscriptions = validSubscriptions as? [[String: Any]],
                let data = try? JSONSerialization.data(withJSONObject: validSubscriptions, options: []),
                  let receipts = try? JSONDecoder().decode([_Receipt].self, from: data),
                  let newestReceipt = receipts.sorted(by: { $0.expireDate > $1.expireDate }).first else {
                // Setting super old date to force expiration logic in the UI.
                Preferences.VPN.expirationDate.value = Date(timeIntervalSince1970: 1)
                receiptHasExpired?(true)
                logAndStoreError("vpn expired", printToConsole: false)
                return
            }
            
            Preferences.VPN.expirationDate.value = newestReceipt.expireDate
            
            if let isTrial = newestReceipt.isTrialPeriod {
                Preferences.VPN.freeTrialUsed.value = !isTrial
            }
            
            receiptHasExpired?(false)
        }
    }
    
    /// Configure the vpn for first time user, or when restoring a purchase on freshly installed app.
    /// Use `resetConfiguration` if you want to reconfigure the vpn for an existing user.
    /// If IAP is restored we treat it as first user configuration as well.
    static func configureFirstTimeUser(completion: ((VPNUserCreationStatus) -> Void)?) {
        if firstTimeUserConfigPending { return }
        firstTimeUserConfigPending = true
        
        // Make sure region override is nil on new user creation.
        // This can happen if the vpn has expired and a user has to buy it again.
        Preferences.VPN.vpnRegionOverride.value = nil
        
        serverManager.selectGuardianHost { host, location, error in
            guard let host = host, error == nil else {
                firstTimeUserConfigPending = false
                logAndStoreError("configureFirstTimeUser connection problems")
                completion?(.error(type: .connectionProblems))
                return
            }
            
            saveHostname(host, displayName: location)
            
            createNewSubscriberCredential(for: host) { status in
                firstTimeUserConfigPending = false
                completion?(status)
            }
        }
    }
    
    private static func createNewSubscriberCredential(for hostname: String,
                                                      completion: @escaping ((VPNUserCreationStatus) -> Void)) {
        
        housekeepingApi.createNewSubscriberCredential(with: .ValidationMethodAppStoreReceipt) {
            jwtCredential, success, error in
            
            if !success {
                logAndStoreError("createNewSubscriberCredential connection problems")
                completion(.error(type: .connectionProblems))
                return
            }
            
            if let jwtCredential = jwtCredential {
                
                helper.createFreshUser(withSubscriberCredential: jwtCredential) { status, createError in
                    if status != .success {
                        logAndStoreError("createFreshUser provisioning problems")
                        completion(.error(type: .provisioning))
                        return
                    }
                    
                    if !saveJwtCredential(jwtCredential) {
                        logAndStoreError("createFreshUser save jwt token error")
                        completion(.error(type: .provisioning))
                        return
                    }
                    
                    completion(.success)
                }
            }
        }
    }
    
    /// Saves jwt credentials in keychain, returns true if save operation was successful.
    private static func saveJwtCredential(_ credential: String) -> Bool {
        let status = GRDKeychain.storePassword(credential, forAccount: kKeychainStr_SubscriberCredential,
                                               retry: true)
        
        return status == errSecSuccess
    }
    
    /// Creates a vpn configuration using Apple's `NEVPN*` api and connects to the vpn if successful.
    /// This method does not connect to the Guardian's servers unless there is no EAP credentials stored in keychain yet,
    /// in this case it tries to reconfigure the vpn before connecting to it.
    static func connectOrMigrateToNewNode(completion: @escaping ((VPNConfigStatus) -> Void)) {
        helper.configureAndConnectVPN { message, status in
            switch status {
            case .success:
                completion(.success)
            case .doesNeedMigration:
                reconfigureVPN() { success in
                    if success {
                        completion(.success)
                    } else {
                        completion(.error(type: .loadConfigError))
                        logAndStoreError("connectOrMigrateToNewNode doesNeedMigration error")
                    }
                }
            case .api_AuthenticationError, .api_ProvisioningError, .app_VpnPrefsLoadError,
                 .app_VpnPrefsSaveError, .coudNotReachAPIError, .fail,
                 .networkConnectionError, .migrating:
                logAndStoreError("connectOrMigrateToNewNode error: \(status.rawValue)")
                completion(.error(type: .loadConfigError))
            @unknown default:
                assertionFailure()
                logAndStoreError("connectOrMigrateToNewNode unknown error")
                completion(.error(type: .loadConfigError))
            }
        }
    }
    
    /// Attempts to reconfigure the vpn by migrating to a new server.
    /// The new hostname is chosen randomly.
    /// Depending on user preference this will connect to either manually selected server region or a region closest to the user.
    /// This method disconnects from the vpn before reconfiguration is happening
    /// and reconnects automatically after reconfiguration is done.
    static func reconfigureVPN(completion: ((Bool) -> Void)? = nil) {
        disconnect()
        GRDKeychain.removeGuardianKeychainItems()
        Preferences.VPN.vpnHostDisplayName.value = nil
        
        // Small delay to disconnect the vpn.
        // Otherwise we might end up with 'no internet connection' error.
        DispatchQueue.global().asyncAfter(deadline: .now() + 1, execute: {
            // Region selected manually by the user.
            if let regionOverride = Preferences.VPN.vpnRegionOverride.value {
                serverManager.findBestHost(inRegion: regionOverride) { host, location, error in
                    guard let host = host, error == nil else {
                        completion?(false)
                        logAndStoreError("reconfigureVPN findBestHost host error")
                        return
                    }
                    
                    reconfigure(with: host, location: location) { success in
                        completion?(success)
                    }
                }
            }
            // Default behavior, automatic mode, chooses location based on device's timezone.
            else {
                serverManager.selectGuardianHost { host, location, error in
                    guard let host = host, error == nil else {
                        completion?(false)
                        logAndStoreError("reconfigureVPN selectGuardianHost host error")
                        return
                    }
                    
                    reconfigure(with: host, location: location) { success in
                        completion?(success)
                    }
                }
                
            }
        })
    }
    
    private static func reconfigure(with host: String, location: String?, completion: ((Bool) -> Void)? = nil) {
        guard let credentialString =
            GRDKeychain.getPasswordString(forAccount: kKeychainStr_SubscriberCredential) else {
            logAndStoreError("reconfigureVPN failed to retrieve subscriber credentials")
            completion?(false)
            return
        }
        
        saveHostname(host, displayName: location)
        
        helper.createFreshUser(withSubscriberCredential: credentialString) { status, createError in
            if status != .success {
                logAndStoreError("reconfigureVPN createFreshUser failed")
                completion?(false)
                return
            }
            
            connectOrMigrateToNewNode { status in
                switch status {
                case .error:
                    logAndStoreError("reconfigureVPN connectOrMigrateToNewNode failed")
                    completion?(false)
                case .success:
                    completion?(true)
                }
            }
        }
    }

    /// Clears current vpn configuration and removes it from preferences.
    /// This method does not clear keychain items and jwt token.
    private static func clearConfiguration() {
        GRDVPNHelper.clearVpnConfiguration()
        Preferences.VPN.vpnRegionOverride.value = nil
        Preferences.VPN.vpnHostDisplayName.value = nil
        
        NEVPNManager.shared().removeFromPreferences { error in
            if let error = error {
                logAndStoreError("Remove vpn error: \(error)")
            }
        }
    }
    
    static func clearCredentials() {
        GRDKeychain.removeGuardianKeychainItems()
        GRDKeychain.removeKeychanItem(forAccount: kKeychainStr_SubscriberCredential)
    }
    
    static func sendVPNWorksInBackgroundNotification() {
        
        switch vpnState {
        case .expired, .notPurchased, .purchased:
            break
        case .installed(let enabled):
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
    
    private static func saveHostname(_ hostname: String, displayName: String?) {
        GRDVPNHelper.saveAll(inOneBoxHostname: hostname)
        GRDGatewayAPI.shared().apiHostname = hostname
        Preferences.VPN.vpnHostDisplayName.value = displayName
    }
    
    // MARK: - Server selection
    
    /// Returns a list of available vpn regions to switch to.
    static func requestAllServerRegions(_ completion: @escaping ([VPNRegion]?) -> Void) {
        housekeepingApi.requestAllServerRegions { regionList, success in
            if !success {
                logAndStoreError("requestAllServerRegions call failed")
                completion(nil)
                return
            }
            
            guard let regionList = regionList,
                  let data = try? JSONSerialization.data(withJSONObject: regionList,
                                                         options: .fragmentsAllowed) else {
                logAndStoreError("failed to deserialize server regions data")
                completion(nil)
                return
            }
            
            do {
                completion(try JSONDecoder().decode([VPNRegion].self, from: data))
            } catch {
                logAndStoreError("Failed to decode VPNRegion JSON: \(error.localizedDescription)")
                completion(nil)
            }
        }
    }
}
