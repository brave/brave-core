// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import WebKit

private let log = Logger.browserLogger

public class UserReferralProgram {
    
    /// Domains must match server HTTP header ones _exactly_
    private static let urpCookieOnlyDomains = ["coinbase.com"]
    public static let shared = UserReferralProgram()
    
    private static let apiKeyPlistKey = "API_KEY"
    private static let clipboardPrefix = "F83AB73F-9852-4F01-ABA8-7830B8825993"
    
    struct HostUrl {
        static let staging = "https://laptop-updates-staging.brave.com"
        static let prod = "https://laptop-updates.brave.com"
    }
    
    // In case of network problems when looking for referrral code
    // we retry the call few times while the app is still alive.
    private struct ReferralLookupRetry {
        var timer: Timer?
        var currentCount = 0
        let retryLimit = 10
        let retryTimeInterval = AppConstants.buildChannel.isPublic ? 3.minutes : 1.minutes
    }
    
    private var referralLookupRetry = ReferralLookupRetry()
    
    let service: UrpService
    
    public init?() {
        func getPlistString(for key: String) -> String? {
            return Bundle.main.infoDictionary?[key] as? String
        }
        
        // This should _probably_ correspond to the baseUrl for NTPDownloader
        let host = AppConstants.buildChannel == .debug ? HostUrl.staging : HostUrl.prod
        
        guard let apiKey = getPlistString(for: UserReferralProgram.apiKeyPlistKey)?.trimmingCharacters(in: .whitespacesAndNewlines) else {
                log.error("Urp init error, failed to get values from Brave.plist.")
                return nil
        }
        
        guard let urpService = UrpService(host: host, apiKey: apiKey) else { return nil }
        
        UrpLog.log("URP init, host: \(host)")
        
        self.service = urpService
    }
    
    /// Looks for referral and returns its landing page if possible.
    public func referralLookup(refCode: String?, completion: @escaping (_ refCode: String?, _ offerUrl: String?) -> Void) {
        UrpLog.log("first run referral lookup")
        
        let referralBlock: (ReferralData?, UrpError?) -> Void = { [weak self] referral, error in
            guard let self = self else { return }
            
            if error == BraveShared.UrpError.endpointError {
                UrpLog.log("URP look up had endpoint error, will retry on next launch.")
                self.referralLookupRetry.timer?.invalidate()
                self.referralLookupRetry.timer = nil
                
                // Hit max retry attempts.
                if self.referralLookupRetry.currentCount > self.referralLookupRetry.retryLimit { return }
                
                self.referralLookupRetry.currentCount += 1
                self.referralLookupRetry.timer =
                    Timer.scheduledTimer(withTimeInterval: self.referralLookupRetry.retryTimeInterval,
                                         repeats: true) { [weak self] _ in
                        self?.referralLookup(refCode: refCode) { refCode, offerUrl in
                            completion(refCode, offerUrl)
                        }
                }
                return
            }
            
            // Connection "succeeded"
            
            Preferences.URP.referralLookupOutstanding.value = false
            guard let ref = referral else {
                log.info("No referral code found")
                UrpLog.log("No referral code found")
                completion(nil, nil)
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
                
                completion(ref.referralCode, ref.offerPage)
                UrpLog.log("Extended referral code found, opening landing page: \(ref.offerPage ?? "404")")
                // We do not want to persist referral data for extended URPs
                return
            }
            
            Preferences.URP.downloadId.value = ref.downloadId
            Preferences.URP.referralCode.value = ref.referralCode
            
            self.referralLookupRetry.timer?.invalidate()
            self.referralLookupRetry.timer = nil
            
            UrpLog.log("Found referral: downloadId: \(ref.downloadId), code: \(ref.referralCode)")
            // In case of network errors or getting `isFinalized = false`, we retry the api call.
            self.initRetryPingConnection(numberOfTimes: 30)
            
            completion(ref.referralCode, nil)
        }
        
        if let refCode = refCode {
            // This is also potentially set after server network request,
            //  esp important for binaries that require server ref code retrieval.
            Preferences.URP.referralCode.value = refCode
        }
        
        // Since ref-code method may not be repeatable (e.g. clipboard was cleared), this should be retrieved from prefs,
        //  and not use the passed in referral code.
        service.referralCodeLookup(refCode: UserReferralProgram.getReferralCode(), completion: referralBlock)
    }
    
    private func initRetryPingConnection(numberOfTimes: Int32) {
        if AppConstants.buildChannel.isPublic {
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
                let timeToDelete = AppConstants.buildChannel.isPublic ? 90.days : 20.minutes
                Preferences.URP.referralCodeDeleteDate.value = Date().timeIntervalSince1970 + timeToDelete
            }
            
            return referralCode
        }
        return nil
    }
    
    /// Passing string, attempts to derive a ref code from it.
    /// Uses very strict matching.
    /// Returns the sanitized code, or nil if no code was found
    public class func sanitize(input: String?) -> String? {
        guard
            var input = input,
            input.hasPrefix(self.clipboardPrefix),
            input.count > self.clipboardPrefix.count
            else { return nil }
        
        // +1 to strip off `:` that proceeds the defined prefix
        input.removeFirst(self.clipboardPrefix.count + 1)
        let valid = input.range(of: #"\b[A-Z]{3}[0-9]{3}\b"#, options: .regularExpression) != nil

        // Both conditions must be met
        return valid ? input : nil
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
