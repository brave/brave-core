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
import Shared
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
  public let dau: DAU
  public let migration: Migration
  public let profile: Profile
  public let rewards: Brave.BraveRewards
  public let newsFeedDataSource: FeedDataSource
  public let uptimeMonitor = UptimeMonitor()
  private var didBecomeActive = false

  public var state: State = .launching(options: [:], active: false) {
    didSet {
      switch state {
      case .launching(_, let isActive):
        if didBecomeActive {
          assertionFailure("Cannot set launching state twice!")
        }

        if isActive && !didBecomeActive {
          // We have to wait until pre 1.12 migration is done until we proceed with database
          // initialization. This is because Database container may change. See bugs #3416, #3377.
          didBecomeActive = true
          DataController.shared.initializeOnce()
          DataController.sharedInMemory.initializeOnce()
          Migration.migrateLostTabsActiveWindow()
        }

        if !AppConstants.isOfficialBuild || Preferences.Debug.developerOptionsEnabled.value {
          NetworkMonitor.shared.start()
        }
        break
      case .active:
        break
      case .backgrounded:
        break
      case .terminating:
        break
      }
    }
  }

  private init() {
    // Setup Constants
    AppState.setupConstants()

    // Setup BraveCore
    braveCore = AppState.setupBraveCore().then {
      $0.scheduleLowPriorityStartupTasks()
    }

    // Setup DAU
    dau = DAU(braveCoreStats: braveCore.braveStats)

    // Setup Profile
    profile = BrowserProfile(localName: "profile")

    // Setup Migrations
    migration = Migration(braveCore: braveCore)

    // Perform Migrations
    migration.launchMigrations(keyPrefix: profile.prefs.getBranchPrefix(), profile: profile)

    // Setup Rewards & Ads
    let configuration = BraveRewards.Configuration.current()
    Self.migrateAdsConfirmations(for: configuration)
    rewards = BraveRewards(configuration: configuration)
    newsFeedDataSource = FeedDataSource()

    // Setup Custom URL scheme handlers
    setupCustomSchemeHandlers(profile: profile)
  }

  public enum State {
    case launching(options: [UIApplication.LaunchOptionsKey: Any], active: Bool)
    case active
    case backgrounded
    case terminating
  }

  private static func setupConstants() {
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
      (ReaderModeHandler.path, ReaderModeHandler(profile: profile, braveCore: braveCore)),
      (Web3DomainHandler.path, Web3DomainHandler()),
      (BlockedDomainHandler.path, BlockedDomainHandler()),
    ]

    responders.forEach { (path, responder) in
      InternalSchemeHandler.responders[path] = responder
    }
  }

  private static func migrateAdsConfirmations(for configuruation: Brave.BraveRewards.Configuration)
  {
    // To ensure after a user launches 1.21 that their ads confirmations, viewed count and
    // estimated payout remain correct.
    //
    // This hack is unfortunately neccessary due to a missed migration path when moving
    // confirmations from ledger to ads, we must extract `confirmations.json` out of ledger's
    // state file and save it as a new file under the ads directory.
    let base = configuruation.storageURL
    let ledgerStateContainer = base.appendingPathComponent("ledger/random_state.plist")
    let adsConfirmations = base.appendingPathComponent("ads/confirmations.json")
    let fm = FileManager.default

    if !fm.fileExists(atPath: ledgerStateContainer.path)
      || fm.fileExists(atPath: adsConfirmations.path)
    {
      // Nothing to migrate or already migrated
      return
    }

    do {
      let contents = NSDictionary(contentsOfFile: ledgerStateContainer.path)
      guard let confirmations = contents?["confirmations.json"] as? String else {
        adsRewardsLog.debug("No confirmations found to migrate in ledger's state container")
        return
      }
      try confirmations.write(toFile: adsConfirmations.path, atomically: true, encoding: .utf8)
    } catch {
      adsRewardsLog.error(
        "Failed to migrate confirmations.json to ads folder: \(error.localizedDescription)"
      )
    }
  }
}
