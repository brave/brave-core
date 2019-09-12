// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

public struct UserAgentBuilder {
    struct UAVersions {
        let safariVersion: String
        let webkitVersion: String
    }
    
    struct OSVersionMap {
        let majorVersion: Int
        let minorVersion: Int
        let uaVersions: UAVersions
    }
    
    public init() {}
    
    public func build(appendSafariVersion: Bool = false) -> String {
        let uaVersions = webkitVersion
        var ua = "Mozilla/5.0 (\(cpuInfo)) AppleWebKit/\(uaVersions.webkitVersion) (KHTML, like Gecko) Mobile/\(kernelVersion)"
        
        if appendSafariVersion {
            ua += " Safari/\(uaVersions.safariVersion)"
        }
        
        return ua
    }
    
    var cpuInfo: String {
        let os = ProcessInfo().operatingSystemVersion
        var osVersion = "\(os.majorVersion)_\(os.minorVersion)"
        
        if os.patchVersion > 0 {
            osVersion.append("_\(os.patchVersion)")
        }
        
        var currentDevice = UIDevice.current.model
        // Only use first part of device name(so "iPod Touch" becomes "iPod")
        if let deviceNameFirstPart = currentDevice.split(separator: " ").first {
            currentDevice = String(deviceNameFirstPart)
        }
        
        let platform = UIDevice.current.userInterfaceIdiom == .pad ? "OS" : "iPhone OS"
        
        return "\(currentDevice); CPU \(platform) \(osVersion) like Mac OS X"
    }
    
    var webkitVersion: UAVersions {
        let _12_0 = OSVersionMap(majorVersion: 12, minorVersion: 0,
                              uaVersions: UAVersions(safariVersion: "605.1",
                                                     webkitVersion: "605.1.15"))
        
        var uaVersions: UAVersions?
        
        // Versions should be added from highest to lowest
        let versionsMap = [_12_0]
        
        if versionsMap.isEmpty { assertionFailure() }
        
        let os = ProcessInfo().operatingSystemVersion
        let majorVersion = os.majorVersion
        let minorVersion = os.minorVersion
        
        // Return the versions corresponding to the first (and thus highest) OS
        // version less than or equal to the given OS version.
        versionsMap.forEach {
            if majorVersion > $0.majorVersion ||
                majorVersion == $0.majorVersion && minorVersion >= $0.minorVersion {
                uaVersions = $0.uaVersions
                return
            }
        }
        
        return uaVersions ?? _12_0.uaVersions
    }
    
    var kernelVersion: String {
        // Kernel version is frozen for iOS 11.3 and later.
        return "15E148"
    }
}
