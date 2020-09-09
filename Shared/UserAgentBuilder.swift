// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit

public struct UserAgentBuilder {
    
    // These parts of UA are frozen in WebKit.
    private let kernelVersion = "15E148"
    private let safariBuildNumber = "604.1"
    private let webkitVersion = "605.1.15"
    
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
        Version/\(safariVersion) \
        Mobile/\(kernelVersion) \
        Safari/\(safariBuildNumber)
        """
    }
    
    // These user agents are taken from iOS Safari in desktop mode and hardcoded.
    // These are not super precise because each iOS version can have slighly different desktop UA.
    // The only differences are with exact `Version/XX` and `MAC OS X 10_XX` numbers.
    private var desktopUA: String {
        // Taken from Safari 14.0(ios 14 beta 7)
        let iOS14DesktopUA =
        """
        Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_6) \
        AppleWebKit/605.1.15 (KHTML, like Gecko) \
        Version/14.0 \
        Safari/605.1.15
        """
        
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
        case 14: return iOS14DesktopUA
        // Fallback to iOS13 UA, next desktop UA will be added once iOS 14 is ready.
        default: return iOS13DesktopUA
        }
    }
    
    private var cpuInfo: String {
        var currentDevice = device.model
        // Only use first part of device name(so "iPod Touch" becomes "iPod")
        if let deviceNameFirstPart = currentDevice.split(separator: " ").first {
            currentDevice = String(deviceNameFirstPart)
        }
        
        let platform = device.userInterfaceIdiom == .pad ? "OS" : "iPhone OS"
        
        return "\(currentDevice); CPU \(platform) \(osVersion) like Mac OS X"
    }
    
    /// 'Version/13.0' part of UA. It seems to be based on Safaris build number.
    private var osVersion: String {
        switch os.majorVersion {
            case 12: return "12_4_1"
            case 13: return "13_6_1"
            case 14: return "14_0"
            default: return "\(os.majorVersion)_0"
                
        }
    }
    
    /// 'Version/13.0' part of UA. It seems to be based on Safaris build number.
    private var safariVersion: String {
        switch os.majorVersion {
            case 12: return "12.1.2"
            case 13: return "13.1.2"
            case 14: return "14.0"
            default: return "\(os.majorVersion).0"
                
        }
    }
}
