// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

public struct UserAgentBuilder {
    
    // These parts of UA are frozen in WebKit.
    private let kernelVersion = "15E148"
    private let safariBuildNumber = "604.1"
    private let webkitVersion = "605.1.15"
    
    /// If we fail getting proper safari version, this version will be used instead.
    /// It is for the 'Safari/13.0.5' part of UA.
    private let fallbackSafariVersion = "13.0.5"
    
    private let device: UIDevice
    private let os: OperatingSystemVersion
    
    /// - parameter iOSVersion: iOS version of the created UA. Both desktop and mobile UA differ between iOS versions.
    public init(device: UIDevice = .current,
                iOSVersion: OperatingSystemVersion = ProcessInfo().operatingSystemVersion) {
        self.device = device
        self.os = iOSVersion
    }
    
    /// Creates Safari-like user agent.
    /// - parameter desktopMode: Wheter to use Mac's Safari UA or regular mobile iOS UA.
    /// The desktop UA is taken from iOS Safari `Request desktop website` feature.
    ///
    /// - returns: A proper user agent to use in WKWebView and url requests.
    public func build(desktopMode: Bool) -> String {
        
        if desktopMode { return desktopUA }
        
        return """
        Mozilla/5.0 (\(cpuInfo)) \
        AppleWebKit/\(webkitVersion) (KHTML, like Gecko) \
        Version/\(versionNumber) \
        Mobile/\(kernelVersion) \
        Safari/\(safariBuildNumber)
        """
    }
    
    // These user agents are taken from iOS Safari in desktop mode and hardcoded.
    // These are not super precise because each iOS version can have slighly different desktop UA.
    // The only differences are with exact `Version/XX` and `MAC OS X 10_XX` numbers.
    private var desktopUA: String {
        // Taken from Safari 13.4
        let iOS13DesktopUA =
        """
        Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15) \
        AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/13.1 \
        Safari/605.1.15
        """
        
        // Taken from Safari 12.4
        let iOS12DesktopUA =
        """
        Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14_6) \
        AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/12.1.2 \
        Safari/605.1.15
        """
        
        switch os.majorVersion {
        case 12: return iOS12DesktopUA
        case 13: return iOS13DesktopUA
        // Fallback to iOS13 UA, next desktop UA will be added once iOS 14 is ready.
        default: return iOS13DesktopUA
        }
    }
    
    private var cpuInfo: String {
        var osVersionString = "\(os.majorVersion)_\(os.minorVersion)"
        
        if os.patchVersion > 0 {
            osVersionString.append("_\(os.patchVersion)")
        }
        
        var currentDevice = device.model
        // Only use first part of device name(so "iPod Touch" becomes "iPod")
        if let deviceNameFirstPart = currentDevice.split(separator: " ").first {
            currentDevice = String(deviceNameFirstPart)
        }
        
        let platform = device.userInterfaceIdiom == .pad ? "OS" : "iPhone OS"
        
        return "\(currentDevice); CPU \(platform) \(osVersionString) like Mac OS X"
    }
    
    // 'Version/13.0' part of UA. It seems to be based on Safaris build number.
    private var versionNumber: String {
        var versionWithDots = "\(os.majorVersion).\(os.minorVersion)"
        if os.patchVersion > 0 {
            versionWithDots += ".\(os.patchVersion)"
        }
        
        return getSafariVersion(for: versionWithDots)
    }
    
    private func getSafariVersion(for iOSVersion: String) -> String {
        // Safari's versions don't follow iOS versions directly.
        // For better coverage of versions, each iOS version has their Safari number set individually.
        // Correct Safari numbers were taken from https://user-agents.net/browsers/safari
        switch iOSVersion {
        case "12.0", "12.0.1", "12.1", "12.1.1", "12.1.2", "12.1.3", "12.1.4":
            return "12.0"
        case "12.2":
            return "12.1"
        case "12.3", "12.3.1", "12.3.2":
            return "12.1.1"
        case "12.4", "12.4.1", "12.4.2", "12.4.3", "12.4.4", "12.4.5":
            return "12.1.2"
        case "13.0":
            return "13.0"
        case "13.1", "13.1.1", "13.1.2", "13.1.3":
            return "13.0.1"
        case "13.2", "13.2.1", "13.2.2", "13.2.3":
            return "13.0.3"
        case "13.3":
            return "13.0.4"
        case "13.3.1":
            return "13.0.5"
        case "13.4":
            return "13.1"
        default:
            // We can't predict what exact safari versions for future iOS releases.
            // As a fallback we aim to use as highest Safari version as possible for give iOS major version.
            //
            // For example if new iOS 13.5 release shows up `highestSafariVersion` will be used.
            // This also works for iOS 12, as Apple still releases hotfixes there.
            // iOS versions were taken from https://en.wikipedia.org/wiki/IOS_version_history
            guard let majorVersion = iOSVersion.components(separatedBy: ".").first else {
                assertionFailure("Could not extract major version from \(iOSVersion)")
                return fallbackSafariVersion
            }
            
            return highestSafariVersion(majorOSVersion: majorVersion)
        }
    }
    
    private func highestSafariVersion(majorOSVersion: String) -> String {
        switch majorOSVersion {
        case "12": return "12.1.2"
        case "13": return "13.1"
        default:
            return "\(majorOSVersion).0"
        }
    }
}
