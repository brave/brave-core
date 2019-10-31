/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import SwiftKeychainWrapper

public let KeychainKeyAuthenticationInfo = "authenticationInfo"
public let AllowedPasscodeFailedAttempts = 3

// MARK: - Helper methods for accessing Authentication information from the Keychain
public extension KeychainWrapper {
    func authenticationInfo() -> AuthenticationKeychainInfo? {
        NSKeyedUnarchiver.setClass(AuthenticationKeychainInfo.self, forClassName: "AuthenticationKeychainInfo")
        return object(forKey: KeychainKeyAuthenticationInfo) as? AuthenticationKeychainInfo
    }

    func setAuthenticationInfo(_ info: AuthenticationKeychainInfo?) {
        NSKeyedArchiver.setClassName("AuthenticationKeychainInfo", for: AuthenticationKeychainInfo.self)
        if let info = info {
            set(info, forKey: KeychainKeyAuthenticationInfo)
        } else {
            removeObject(forKey: KeychainKeyAuthenticationInfo)
        }
    }
}

open class AuthenticationKeychainInfo: NSObject, NSSecureCoding {
    fileprivate(set) open var passcode: String?
    open var isPasscodeRequiredImmediately: Bool
    fileprivate(set) open var lockOutInterval: TimeInterval?
    fileprivate(set) open var failedAttempts: Int
    open var useTouchID: Bool

    // Timeout period before user can retry entering passcodes
    private let lockTimeInterval: TimeInterval = AppConstants.BuildChannel.isPublic ? 15 * 60 : 60

    public init(passcode: String) {
        self.passcode = passcode
        self.isPasscodeRequiredImmediately = true
        self.failedAttempts = 0
        self.useTouchID = false
    }

    open func encode(with aCoder: NSCoder) {
        if let lockOutInterval = lockOutInterval, isLocked() {
            let interval = NSNumber(value: lockOutInterval as Double)
            aCoder.encode(interval, forKey: "lockOutInterval")
        }

        aCoder.encode(passcode, forKey: "passcode")
        aCoder.encode(isPasscodeRequiredImmediately, forKey: "isPasscodeRequiredImmediately")
        aCoder.encode(failedAttempts, forKey: "failedAttempts")
        aCoder.encode(useTouchID, forKey: "useTouchID")
    }

    public required init?(coder aDecoder: NSCoder) {
        if let lockOutInterval = aDecoder.decodeObject(of: NSNumber.self, forKey: "lockOutInterval") as NSNumber? {
            self.lockOutInterval = lockOutInterval.doubleValue
        }
        self.passcode = aDecoder.decodeObject(of: NSString.self, forKey: "passcode") as String?
        self.failedAttempts = aDecoder.decodeInteger(forKey: "failedAttempts")
        self.useTouchID = aDecoder.decodeBool(forKey: "useTouchID")
        if aDecoder.containsValue(forKey: "isPasscodeRequiredImmediately") {
            self.isPasscodeRequiredImmediately = aDecoder.decodeBool(forKey: "isPasscodeRequiredImmediately")
        } else if let interval = aDecoder.decodeObject(of: NSNumber.self, forKey: "requiredPasscodeInterval") as NSNumber? {
            // This is solely used for 1.6.6 -> 1.7 migration
            //  `requiredPasscodeInterval` is not re-encoded on this object
            self.isPasscodeRequiredImmediately = (interval == 2)
        } else {
            self.isPasscodeRequiredImmediately = true
        }
    }
    
    public static var supportsSecureCoding: Bool {
        return true
    }
}

// MARK: - API
public extension AuthenticationKeychainInfo {
    private func resetLockoutState() {
        self.failedAttempts = 0
        self.lockOutInterval = nil
    }

    func updatePasscode(_ passcode: String) {
        self.passcode = passcode
    }

    func recordValidation() {
        resetLockoutState()
    }

    func lockOutUser() {
        self.lockOutInterval = SystemUtils.systemUptime()
    }

    func recordFailedAttempt() {
        if self.failedAttempts >= AllowedPasscodeFailedAttempts {
            //This is a failed attempt after a lockout period. Reset the lockout state
            //This prevents failedAttemps from being higher than 3
            self.resetLockoutState()
        }
        self.failedAttempts += 1
    }

    func isLocked() -> Bool {
        guard let lockOutInterval = self.lockOutInterval else {
            return false
        }
        
        if SystemUtils.systemUptime() < lockOutInterval {
            // Unlock and require passcode input
            resetLockoutState()
            return false
        }
        return (SystemUtils.systemUptime() - (self.lockOutInterval ?? 0)) < lockTimeInterval
    }
    
    var lockoutTimeLeft: TimeInterval? {
        guard let lockOutInterval = lockOutInterval else { return nil }
        
        let timeLeft = (lockOutInterval + lockTimeInterval) - SystemUtils.systemUptime()
        return timeLeft > 0 ? timeLeft : nil
    }
}
