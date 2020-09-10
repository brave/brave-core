/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

public enum AppBuildChannel: String {
    case release
    case beta
    case dev
    case enterprise
    case debug
    
    /// Whether this release channel is used/seen by external users (app store or testers)
    public var isPublic: Bool {
        // Using switch to force a return definition for each enum value
        // Simply using `return [.release, .beta].includes(self)` could lead to easily missing a definition
        //  if enum is ever expanded
        switch self {
        case .release, .beta:
            return true
        case .dev, .debug, .enterprise:
            return false
        }
    }
    
    public var serverChannelParam: String {
        switch self {
        case .release:
            return "release"
        case .beta:
            return "beta"
         case .dev:
             // This is designed to follow desktop platform
            return "developer"
        case .debug, .enterprise:
            return "invalid"
        }
    }
}

public enum KVOConstants: String {
    case loading = "loading"
    case estimatedProgress = "estimatedProgress"
    case URL = "URL"
    case title = "title"
    case canGoBack = "canGoBack"
    case canGoForward = "canGoForward"
    case contentSize = "contentSize"
    case hasOnlySecureContent = "hasOnlySecureContent"
    case serverTrust = "serverTrust"
}

public struct AppConstants {
    public static let isRunningTest = NSClassFromString("XCTestCase") != nil || ProcessInfo.processInfo.arguments.contains(LaunchArguments.test)

    /// Build Channel.
    public static let buildChannel: AppBuildChannel = {
        #if MOZ_CHANNEL_RELEASE
            return AppBuildChannel.release
        #elseif MOZ_CHANNEL_BETA
            return AppBuildChannel.beta
        #elseif MOZ_CHANNEL_DEV
            return AppBuildChannel.dev
        #elseif MOZ_CHANNEL_ENTERPRISE
            return AppBuildChannel.enterprise
        #elseif MOZ_CHANNEL_DEBUG
            return AppBuildChannel.debug
        #endif
    }()
    
    public static func iOSVersionGreaterThanOrEqual(to version: Int) -> Bool {
        ProcessInfo().operatingSystemVersion.majorVersion >= version
    }

    public static let scheme: String = {
        guard let identifier = Bundle.main.bundleIdentifier else {
            return "unknown"
        }

        let scheme = identifier.replacingOccurrences(of: "com.brave.ios.", with: "")
        if scheme == "FirefoxNightly.enterprise" {
            return "FirefoxNightly"
        }
        return scheme
    }()

    public static let prefSendUsageData = "settings.sendUsageData"

    /// Whether we just mirror (false) or actively do a full bookmark merge and upload (true).
    public static var shouldMergeBookmarks = false

    /// Should we send a repair request to other clients when the bookmarks buffer validation fails.
    public static let MOZ_BOOKMARKS_REPAIR_REQUEST: Bool = {
        #if MOZ_CHANNEL_RELEASE
            return false
        #elseif MOZ_CHANNEL_BETA
            return true
        #elseif MOZ_CHANNEL_DEBUG
            return true
        #else
            return true
        #endif
    }()

    /// The maximum length of a URL stored by Firefox. Shared with Places on desktop.
    public static let DB_URL_LENGTH_MAX = 65536

    /// The maximum length of a page title stored by Firefox. Shared with Places on desktop.
    public static let DB_TITLE_LENGTH_MAX = 4096

    /// The maximum length of a bookmark description stored by Firefox. Shared with Places on desktop.
    public static let DB_DESCRIPTION_LENGTH_MAX = 1024
}
