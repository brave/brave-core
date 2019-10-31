// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import Deferred
import WebKit

private let log = Logger.browserLogger

public class UserReferralProgram {
    
    /// Domains must match server HTTP header ones _exactly_
    private static let urpCookieOnlyDomains = ["coinbase.com"]
    public static let shared = UserReferralProgram()
    
    private static let apiKeyPlistKey = "API_KEY"
    
    struct HostUrl {
        static let staging = "https://brave-laptop-updates-staging.herokuapp.com"
        static let prod = "https://laptop-updates.brave.com"
    }
    
    let service: UrpService
    
    public init?() {
        func getPlistString(for key: String) -> String? {
            return Bundle.main.infoDictionary?[key] as? String
        }
        
        let host = AppConstants.BuildChannel == .developer ? HostUrl.staging : HostUrl.prod
        
        guard let apiKey = getPlistString(for: UserReferralProgram.apiKeyPlistKey)?.trimmingCharacters(in: .whitespacesAndNewlines) else {
                log.error("Urp init error, failed to get values from Brave.plist.")
                return nil
        }
        
        guard let urpService = UrpService(host: host, apiKey: apiKey) else { return nil }
        
        UrpLog.log("URP init, host: \(host)")
        
        self.service = urpService
    }
    
    /// Looks for referral and returns its landing page if possible.
    public func referralLookup(completion: @escaping (String?) -> Void) {
        UrpLog.log("first run referral lookup")
        
        service.referralCodeLookup { referral, _ in
            guard let ref = referral else {
                log.info("No referral code found")
                UrpLog.log("No referral code found")
                return
            }
            
            if ref.isExtendedUrp() {
                if let headers = ref.customHeaders {
                    do {
                        try Preferences.URP.customHeaderData.value = NSKeyedArchiver.archivedData(withRootObject: headers, requiringSecureCoding: false)
                    } catch {
                        log.error("Failed to save URP custom header data \(headers) with error: \(error.localizedDescription)")
                    }
                }
                
                completion(ref.offerPage)
                UrpLog.log("Extended referral code found, opening landing page: \(ref.offerPage ?? "404")")
                // We do not want to persist referral data for extended URPs
                return
            }
            
            Preferences.URP.downloadId.value = ref.downloadId
            Preferences.URP.referralCode.value = ref.referralCode
            
            UrpLog.log("Found referral: downloadId: \(ref.downloadId), code: \(ref.referralCode)")
            // In case of network errors or getting `isFinalized = false`, we retry the api call.
            self.initRetryPingConnection(numberOfTimes: 30)
            
            completion(nil)
        }
    }
    
    private func initRetryPingConnection(numberOfTimes: Int32) {
        if AppConstants.BuildChannel.isPublic {
            // Adding some time offset to be extra safe.
            let offset = 1.hours
            let _30daysFromToday = Date().timeIntervalSince1970 + 30.days + offset
            Preferences.URP.nextCheckDate.value = _30daysFromToday
        } else {
            // For local and beta builds use a short timer
            Preferences.URP.nextCheckDate.value = Date().timeIntervalSince1970 + 10.minutes
        }
        
        Preferences.URP.retryCountdown.value = Int(numberOfTimes)
    }
    
    public func pingIfEnoughTimePassed() {
        if !DeviceInfo.hasConnectivity() {
            UrpLog.log("No internet connection, not sending update ping.")
            return
        }
        
        guard let downloadId = Preferences.URP.downloadId.value else {
            log.info("Could not retrieve download id model from preferences.")
            UrpLog.log("Update ping, no download id found.")
            return
        }
        
        guard let checkDate = Preferences.URP.nextCheckDate.value else {
            log.error("Could not retrieve check date from preferences.")
            return
        }
        
        let todayInSeconds = Date().timeIntervalSince1970
        
        if todayInSeconds <= checkDate {
            log.debug("Not enough time has passed for referral ping.")
            UrpLog.log("Not enough time has passed for referral ping.")
            return
        }
        
        UrpLog.log("Update ping")
        service.checkIfAuthorizedForGrant(with: downloadId) { initialized, error in
            guard let counter = Preferences.URP.retryCountdown.value else {
                log.error("Could not retrieve retry countdown from preferences.")
                return
            }
            
            var shouldRemoveData = false
            
            if error == .downloadIdNotFound {
                UrpLog.log("Download id not found on server.")
                shouldRemoveData = true
            }
            
            if initialized == true {
                UrpLog.log("Got initialized = true from server.")
                shouldRemoveData = true
            }
            
            // Last retry attempt
            if counter <= 1 {
                UrpLog.log("Last retry and failed to get data from server.")
                shouldRemoveData = true
            }
            
            if shouldRemoveData {
                UrpLog.log("Removing all referral data from device")
                
                Preferences.URP.downloadId.value = nil
                Preferences.URP.nextCheckDate.value = nil
                Preferences.URP.retryCountdown.value = nil
            } else {
                UrpLog.log("Network error or isFinalized returned false, decrementing retry counter and trying again next time.")
                // Decrement counter, next retry happens on next day
                Preferences.URP.retryCountdown.value = counter - 1
                Preferences.URP.nextCheckDate.value = checkDate + 1.days
            }
        }
    }
    
    /// Returns referral code and sets expiration day for its deletion from DAU pings(if needed).
    public class func getReferralCode() -> String? {
        if let referralCodeDeleteDate = Preferences.URP.referralCodeDeleteDate.value,
            Date().timeIntervalSince1970 >= referralCodeDeleteDate {
            Preferences.URP.referralCode.value = nil
            Preferences.URP.referralCodeDeleteDate.value = nil
            UrpLog.log("Enough time has passed, removing referral code data")
            return nil
        } else if let referralCode = Preferences.URP.referralCode.value {
            // Appending ref code to dau ping if user used installed the app via user referral program.
            if Preferences.URP.referralCodeDeleteDate.value == nil {
                UrpLog.log("Setting new date for deleting referral code.")
                let timeToDelete = AppConstants.BuildChannel.isPublic ? 90.days : 20.minutes
                Preferences.URP.referralCodeDeleteDate.value = Date().timeIntervalSince1970 + timeToDelete
            }
            
            return referralCode
        }
        return nil
    }
    
    /// Same as `customHeaders` only blocking on result, to gaurantee data is available
    private func fetchNewCustomHeaders() -> Deferred<[CustomHeaderData]> {
        let result = Deferred<[CustomHeaderData]>()
        
        // No early return, even if data exists, still want to flush the storage
        service.fetchCustomHeaders() { headers, error in
            if headers.isEmpty { return }
            
            do {
                try Preferences.URP.customHeaderData.value = NSKeyedArchiver.archivedData(withRootObject: headers, requiringSecureCoding: false)
            } catch {
                log.error("Failed to save URP custom header data \(headers) with error: \(error.localizedDescription)")
            }
            result.fill(headers)
        }
        
        return result
    }
    
    /// Returns custom headers synchronously
    private var customHeaders: [CustomHeaderData]? {
        guard let customHeadersAsData = Preferences.URP.customHeaderData.value else { return nil }
        
        do {
            return try NSKeyedUnarchiver.unarchiveTopLevelObjectWithData(customHeadersAsData) as? [CustomHeaderData]
        } catch {
            log.error("Failed to unwrap custom headers with error: \(error.localizedDescription)")
        }
        return nil
    }
    
    /// Checks if a custom header should be added to the request and returns its value and field.
    public class func shouldAddCustomHeader(for request: URLRequest) -> (value: String, field: String)? {
        guard let customHeaders = UserReferralProgram.shared?.customHeaders,
            let hostUrl = request.url?.host else { return nil }
        
        for customHeader in customHeaders {
            // There could be an egde case when we would have two domains withing different domain groups, that would
            // cause to return only the first domain-header it approaches.
            for domain in customHeader.domainList where hostUrl.contains(domain) {
                // If `domain` is "cookie only", we exclude it from HTTP Headers, and just use cookie approach
                let cookieOnly = urpCookieOnlyDomains.contains(domain)
                if !cookieOnly, let allFields = request.allHTTPHeaderFields, !allFields.keys.contains(customHeader.headerField) {
                    return (customHeader.headerValue, customHeader.headerField)
                }
            }
        }
        
        return nil
    }
    
    public func insertCookies(intoStore store: WKHTTPCookieStore) {
        assertIsMainThread("Setting up cookies for URP, must happen on main thread")
        
        func attachCookies(from headers: [CustomHeaderData]?) {
            headers?.flatMap { $0.cookies() }.forEach { store.setCookie($0) }
        }
        
        // Attach all existing cookies
        attachCookies(from: customHeaders)
        
        // Pull new ones and attach them async
        fetchNewCustomHeaders().uponQueue(.main, block: attachCookies)
    }
}
