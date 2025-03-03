// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import AIChat
import Brave
import BraveCore
import BraveNews
import Data
import Foundation
import Growth
import Preferences
import RuntimeWarnings
@_spi(AppLaunch) import Shared
import Storage
import UIKit
import UserAgent
import os.log

private let adsRewardsLog = Logger(
  subsystem: Bundle.main.bundleIdentifier!,
  category: "ads-rewards"
)

/// Class that does startup initialization
/// Everything in this class can only be execute ONCE
/// IE: BraveCore initialization, BuildChannel, Migrations, etc.
public class AppState {
  private let log = Logger(subsystem: Bundle.main.bundleIdentifier!, category: "app-state")

  public static let shared = AppState()

  public let braveCore: BraveCoreMain

  private(set) public var dau: DAU!
  private(set) public var migration: Migration!
  private(set) public var profile: Profile!
  private(set) public var rewards: Brave.BraveRewards!
  private(set) public var newsFeedDataSource: FeedDataSource!
  private(set) public var uptimeMonitor = UptimeMonitor()

  public var state: State = .willLaunch(options: [:]) {
    didSet {
      switch state {
      case .willLaunch(_):
        // Setup Profile
        profile = BrowserProfile(localName: "profile")

        // Setup Migrations
        migration = Migration(braveCore: braveCore)

        // Perform Migrations
        migration.launchMigrations(keyPrefix: profile.prefs.getBranchPrefix(), profile: profile)

        // Setup DAU
//        dau = DAU(braveCoreStats: braveCore.braveStats)

        // Setup Rewards & Ads
//        let configuration = BraveRewards.Configuration.current()
//        rewards = BraveRewards(configuration: configuration)
//        newsFeedDataSource = FeedDataSource()

        // Setup Custom URL scheme handlers
        setupCustomSchemeHandlers(profile: profile)

        if !AppConstants.isOfficialBuild || Preferences.Debug.developerOptionsEnabled.value {
          NetworkMonitor.shared.start()
        }
      case .didLaunch(_):
        // We have to wait until pre 1.12 migration is done until we proceed with database
        // initialization. This is because Database container may change. See bugs #3416, #3377.
        DataController.shared.initializeOnce()
        DataController.sharedInMemory.initializeOnce()

        // Run migrations that need access to Data
        MainActor.assumeIsolated {
          Migration.postDataLoadMigration()
        }
      case .active:
        break
      case .backgrounded:
        break
      case .terminating:
        profile.shutdown()
        braveCore.syncAPI.removeAllObservers()
      }
    }
  }

  private init() {
    // Setup Constants
    AppState.setupConstants()

    // Setup BraveCore
    braveCore = AppState.setupBraveCore()
  }

  public enum State {
    case willLaunch(options: [UIApplication.LaunchOptionsKey: Any])
    case didLaunch(options: [UIApplication.LaunchOptionsKey: Any])
    case active
    case backgrounded
    case terminating
  }

  private static func setupConstants() {
    // Application Constants must be initialized first
    #if BRAVE_CHANNEL_RELEASE
    AppConstants.setBuildChannel(.release)
    #elseif BRAVE_CHANNEL_BETA
    AppConstants.setBuildChannel(.beta)
    #elseif BRAVE_CHANNEL_NIGHTLY
    AppConstants.setBuildChannel(.nightly)
    #elseif BRAVE_CHANNEL_DEBUG
    AppConstants.setBuildChannel(.debug)
    #endif

    #if OFFICIAL_BUILD
    AppConstants.setOfficialBuild(true)
    #endif
  }

  private static func setupBraveCore() -> BraveCoreMain {
    // BraveCore Log Handler
    BraveCoreMain.setLogHandler { severity, file, line, messageStartIndex, message in
      let message = String(message.dropFirst(messageStartIndex).dropLast())
        .trimmingCharacters(in: .whitespacesAndNewlines)
      if message.isEmpty {
        // Nothing to print
        return true
      }

      if severity == .fatal {
        let filename = URL(fileURLWithPath: file).lastPathComponent
        #if DEBUG
        // Prints a special runtime warning instead of crashing.
        os_log(
          .fault,
          dso: os_rw.dso,
          log: os_rw.log(category: "BraveCore"),
          "[%@:%ld] > %@",
          filename,
          line,
          message
        )
        return true
        #else
        fatalError("Fatal BraveCore Error at \(filename):\(line).\n\(message)")
        #endif
      }

      let isLoggingAccessible =
        !AppConstants.isOfficialBuild || Preferences.Debug.developerOptionsEnabled.value

      let level: OSLogType = {
        switch severity {
        case .fatal: return .fault
        case .error: return .error
        // No `.warning` level exists for OSLogType. os_Log.warning is an alias for `.error`.
        case .warning: return .error
        case .info: return .info
        // `debug` log level doesn't show up in Console.app when not building a Debug configuration
        default: return isLoggingAccessible ? .info : .debug
        }
      }()

      let braveCoreLogger = Logger(subsystem: Bundle.main.bundleIdentifier!, category: "brave-core")
      if isLoggingAccessible {
        braveCoreLogger.log(level: level, "\(message, privacy: .public)")
      } else {
        braveCoreLogger.log(level: level, "\(message, privacy: .private)")
      }

      return true
    }

    // Initialize BraveCore Switches
    var switches: [BraveCoreSwitch] = []
    // Check prefs for additional switches
    let activeSwitches = Set(Preferences.BraveCore.activeSwitches.value)
    let customSwitches = Set(Preferences.BraveCore.customSwitches.value)
    let switchValues = Preferences.BraveCore.switchValues.value

    // Add regular known switches
    for activeSwitch in activeSwitches {
      let key = BraveCoreSwitchKey(rawValue: activeSwitch)
      if key.isValueless {
        switches.append(.init(key: key))
      } else if let value = switchValues[activeSwitch], !value.isEmpty {
        switches.append(.init(key: key, value: value))
      }
    }

    // Add custom user defined switches
    for customSwitch in customSwitches {
      let key = BraveCoreSwitchKey(rawValue: customSwitch)
      if let value = switchValues[customSwitch] {
        if value.isEmpty {
          switches.append(.init(key: key))
        } else {
          switches.append(.init(key: key, value: value))
        }
      }
    }

    switches.append(.init(key: .rewardsFlags, value: BraveRewards.Configuration.current().flags))

    // Initialize BraveCore
    return BraveCoreMain(userAgent: UserAgent.mobile, additionalSwitches: switches)
  }

  private func setupCustomSchemeHandlers(profile: Profile) {
    let responders: [(String, InternalSchemeResponse)] = [
      (AboutHomeHandler.path, AboutHomeHandler()),
      (AboutLicenseHandler.path, AboutLicenseHandler()),
      (SessionRestoreHandler.path, SessionRestoreHandler()),
      (ErrorPageHandler.path, ErrorPageHandler()),
      (ReaderModeHandler.path, ReaderModeHandler(profile: profile)),
      (Web3DomainHandler.path, Web3DomainHandler()),
      (BlockedDomainHandler.path, BlockedDomainHandler()),
      (HTTPBlockedHandler.path, HTTPBlockedHandler()),
    ]

    responders.forEach { (path, responder) in
      InternalSchemeHandler.responders[path] = responder
    }
  }
}
