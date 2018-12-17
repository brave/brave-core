// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Shared

private let log = Logger.browserLogger

public class UserReferralProgram {
    public static let shared = UserReferralProgram()
    
    private static let apiKeyPlistKey = "API_KEY"
    
    struct HostUrl {
        static let staging = "https://laptop-updates-staging.herokuapp.com"
        static let prod = "https://laptop-updates.brave.com"
    }
    
    let service: UrpService
    
    public init?() {
        func getPlistString(for key: String) -> String? {
            return Bundle.main.infoDictionary?[key] as? String
        }
        
        let host = AppConstants.BuildChannel == .release ? HostUrl.prod : HostUrl.staging
        
        guard let apiKey = getPlistString(for: UserReferralProgram.apiKeyPlistKey) else {
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
                self.getCustomHeaders()
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
        let _10minutes: TimeInterval = 10 * 60
        if AppConstants.BuildChannel == .developer {
            Preferences.URP.nextCheckDate.value = Date().timeIntervalSince1970 + _10minutes
        } else {
            let _30daysInSeconds = Double(30 * 24 * 60 * 60)
            // Adding some time offset to be extra safe.
            let offset = Double(1 * 60 * 60)
            let _30daysFromToday = Date().timeIntervalSince1970 + _30daysInSeconds + offset
            
            Preferences.URP.nextCheckDate.value = _30daysFromToday
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
                let _1dayInSeconds = Double(1 * 24 * 60 * 60)
                Preferences.URP.nextCheckDate.value = checkDate + _1dayInSeconds
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
                let timeToDelete = AppConstants.BuildChannel == .developer ? TimeInterval(20 * 60) : TimeInterval(90 * 24 * 60 * 60)
                
                Preferences.URP.referralCodeDeleteDate.value = Date().timeIntervalSince1970 + timeToDelete
            }
            
            return referralCode
        }
        return nil
    }
    
    public func getCustomHeaders() {
        service.fetchCustomHeaders() { headers, error in
            if headers.isEmpty { return }
            
            do {
                try Preferences.URP.customHeaderData.value = NSKeyedArchiver.archivedData(withRootObject: headers, requiringSecureCoding: false)
            } catch {
                log.error("Failed to save URP custom header data \(headers) with error: \(error.localizedDescription)")
            }
        }
    }
    
    /// Checks if a custom header should be added to the request and returns its value and field.
    public class func shouldAddCustomHeader(for request: URLRequest) -> (value: String, field: String)? {
        guard let customHeadersAsData = Preferences.URP.customHeaderData.value,
            let customHeaders = (try? NSKeyedUnarchiver.unarchiveTopLevelObjectWithData(customHeadersAsData)) as? [CustomHeaderData],
            let hostUrl = request.url?.host else { return nil }
        
        for customHeader in customHeaders {
            // There could be an egde case when we would have two domains withing different domain groups, that would
            // cause to return only the first domain-header it approaches.
            for domain in customHeader.domainList {
                if hostUrl.contains(domain) {
                    if let allFields = request.allHTTPHeaderFields, !allFields.keys.contains(customHeader.headerField) {
                        return (customHeader.headerValue, customHeader.headerField)
                    }
                }
            }
        }
        
        return nil
    }
}
