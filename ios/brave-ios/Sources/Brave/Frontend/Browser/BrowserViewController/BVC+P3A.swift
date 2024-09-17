// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveShields
import Data
import Foundation
import Growth
import Preferences
import Shared
import os.log

extension BrowserViewController {
  enum CreateTabActionLocation {
    case toolbar
    case tabTray
  }

  func recordCreateTabAction(location: CreateTabActionLocation?) {
    var tabCreationTotalStorage = P3ATimedStorage<Int>.tabCreationTotalStorage
    var tabCreationFromToolbarStorage = P3ATimedStorage<Int>.tabCreationFromToolbarStorage

    if let location, !privateBrowsingManager.isPrivateBrowsing {
      tabCreationTotalStorage.add(value: 1, to: Date())
      if location == .toolbar {
        tabCreationFromToolbarStorage.add(value: 1, to: Date())
      }
    }

    let toolbarCount = tabCreationFromToolbarStorage.combinedValue
    let total = tabCreationTotalStorage.combinedValue

    if total == 0 {
      return
    }

    UmaHistogramRecordValueToBucket(
      "Brave.Core.NewTabMethods",
      buckets: [
        .r(0..<25),
        .r(25..<50),
        .r(50..<75),
        .r(75...),
      ],
      value: Int((Double(toolbarCount) / Double(total)) * 100)
    )
  }

  func recordURLBarSubmitLocationP3A(from tab: Tab?) {
    var urlSubmissionTotalStorage = P3ATimedStorage<Int>.urlSubmissionTotalStorage
    var urlSubmissionNewTabStorage = P3ATimedStorage<Int>.urlSubmissionNewTabStorage

    if let tab, !tab.isPrivate {
      let isNewTab = tab.url.flatMap { InternalURL($0) }?.isAboutHomeURL == true
      if isNewTab {
        urlSubmissionNewTabStorage.add(value: 1, to: Date())
      }
      urlSubmissionTotalStorage.add(value: 1, to: Date())
    }

    let newTabCount = urlSubmissionNewTabStorage.combinedValue
    let total = urlSubmissionTotalStorage.combinedValue

    if total == 0 {
      return
    }

    UmaHistogramRecordValueToBucket(
      "Brave.Core.LocationNewEntries",
      buckets: [
        .r(0..<25),
        .r(25..<50),
        .r(50..<75),
        .r(75...),
      ],
      value: Int((Double(newTabCount) / Double(total)) * 100)
    )
  }

  func recordWeeklyUsage() {
    var weeklyUsage = P3ATimedStorage<Int>.weeklyUsage
    weeklyUsage.replaceTodaysRecordsIfLargest(value: 1)
    let calendar = Calendar(identifier: .gregorian)
    let mondayWeekday = 2
    let isTodayMonday = calendar.component(.weekday, from: .now) == mondayWeekday
    guard
      let thisMonday = isTodayMonday
        ? calendar.startOfDay(for: .now)
        : calendar.nextDate(
          after: .now,
          matching: .init(weekday: mondayWeekday),
          matchingPolicy: .nextTime,
          direction: .backward
        ),
      let lastMonday = calendar.nextDate(
        after: thisMonday,
        matching: .init(weekday: mondayWeekday),
        matchingPolicy: .nextTime,
        direction: .backward
      )
    else {
      return
    }
    let answer = weeklyUsage.combinedValue(in: lastMonday..<thisMonday)
    // buckets are 0...7 but no reason to declare them in this case
    UmaHistogramExactLinear("Brave.Core.WeeklyUsage", answer, 8)
  }

  func recordDefaultBrowserLikelyhoodP3A(openedHTTPLink: Bool = false) {
    let isInstalledInThePastWeek: Bool = {
      guard let installDate = Preferences.DAU.installationDate.value else {
        // Pref is removed after 14 days
        return false
      }
      return Date.now.timeIntervalSince(installDate) <= 7.days
    }()
    enum Answer: Int, CaseIterable {
      case notEnoughInfo = 0
      case no = 1
      case yes = 2
    }
    let isLikelyDefault = DefaultBrowserHelper.isBraveLikelyDefaultBrowser()
    let answer: Answer = {
      if !openedHTTPLink, isInstalledInThePastWeek {
        // Hasn't been at least 7 days
        return .notEnoughInfo
      }
      return isLikelyDefault ? .yes : .no
    }()
    UmaHistogramEnumeration("Brave.IOS.IsLikelyDefault", sample: answer)
  }

  @MainActor func maybeRecordInitialShieldsP3A() {
    // Increment this version if metrics in this function change
    // so that all metrics can be re-reported after update.
    let kCurrentReportRevision = 1
    guard kCurrentReportRevision > Preferences.Shields.initialP3AStateReportedRevision.value else {
      return
    }
    recordGlobalAdBlockShieldsP3A()
    recordGlobalFingerprintingShieldsP3A()
    let buckets: [Bucket] = [
      0,
      .r(1...5),
      .r(6...10),
      .r(11...20),
      .r(21...30),
      .r(31...),
    ]
    recordShieldsLevelUpdateP3A(buckets: buckets)
    recordFinterprintProtectionP3A(buckets: buckets)

    Preferences.Shields.initialP3AStateReportedRevision.value = kCurrentReportRevision
  }

  @MainActor private func recordShieldsLevelUpdateP3A(buckets: [Bucket]) {
    // Q51 On how many domains has the user set the adblock setting to be lower (block less) than the default?
    let adsBelowGlobalCount = Domain.totalDomainsWithAdblockShieldsLoweredFromGlobal()
    UmaHistogramRecordValueToBucket(
      "Brave.Shields.DomainAdsSettingsBelowGlobal",
      buckets: buckets,
      value: adsBelowGlobalCount
    )
    // Q52 On how many domains has the user set the adblock setting to be higher (block more) than the default?
    let adsAboveGlobalCount = Domain.totalDomainsWithAdblockShieldsIncreasedFromGlobal()
    UmaHistogramRecordValueToBucket(
      "Brave.Shields.DomainAdsSettingsAboveGlobal",
      buckets: buckets,
      value: adsAboveGlobalCount
    )
  }

  func recordFinterprintProtectionP3A(buckets: [Bucket]) {
    // Q53 On how many domains has the user set the FP setting to be lower (block less) than the default?
    let fingerprintingBelowGlobalCount =
      Domain.totalDomainsWithFingerprintingProtectionLoweredFromGlobal()
    UmaHistogramRecordValueToBucket(
      "Brave.Shields.DomainFingerprintSettingsBelowGlobal",
      buckets: buckets,
      value: fingerprintingBelowGlobalCount
    )
    // Q54 On how many domains has the user set the FP setting to be higher (block more) than the default?
    let fingerprintingAboveGlobalCount =
      Domain.totalDomainsWithFingerprintingProtectionIncreasedFromGlobal()
    UmaHistogramRecordValueToBucket(
      "Brave.Shields.DomainFingerprintSettingsAboveGlobal",
      buckets: buckets,
      value: fingerprintingAboveGlobalCount
    )
  }

  func recordGlobalAdBlockShieldsP3A() {
    // Q46 What is the global ad blocking shields setting?
    enum Answer: Int, CaseIterable {
      case disabled = 0
      case standard = 1
      case aggressive = 2
    }

    let answer = { () -> Answer in
      switch ShieldPreferences.blockAdsAndTrackingLevel {
      case .disabled: return .disabled
      case .standard: return .standard
      case .aggressive: return .aggressive
      }
    }()

    UmaHistogramEnumeration("Brave.Shields.AdBlockSetting", sample: answer)
  }

  func recordGlobalFingerprintingShieldsP3A() {
    // Q47 What is the global fingerprinting shields setting?
    enum Answer: Int, CaseIterable {
      case disabled = 0
      case standard = 1
      case aggressive = 2
    }
    let answer: Answer = Preferences.Shields.fingerprintingProtection.value ? .standard : .disabled
    UmaHistogramEnumeration("Brave.Shields.FingerprintBlockSetting", sample: answer)
  }

  func recordDataSavedP3A(change: Int) {
    var dataSavedStorage = P3ATimedStorage<Int>.dataSavedStorage
    dataSavedStorage.add(
      value: change * BraveGlobalShieldStats.shared.averageBytesSavedPerItem,
      to: Date()
    )

    // Values are in MB
    let buckets: [Bucket] = [
      0,
      .r(1...50),
      .r(51...100),
      .r(101...200),
      .r(201...400),
      .r(401...700),
      .r(701...1500),
      .r(1501...),
    ]
    let amountOfDataSavedInMB = dataSavedStorage.combinedValue / 1024 / 1024
    UmaHistogramRecordValueToBucket(
      "Brave.Savings.BandwidthSavingsMB",
      buckets: buckets,
      value: amountOfDataSavedInMB
    )
  }

  func recordVPNUsageP3A(vpnEnabled: Bool) {
    let usage = P3AFeatureUsage.braveVPNUsage
    var braveVPNDaysInMonthUsedStorage = P3ATimedStorage<Int>.braveVPNDaysInMonthUsedStorage

    if vpnEnabled {
      usage.recordUsage()
      braveVPNDaysInMonthUsedStorage.replaceTodaysRecordsIfLargest(value: 1)
    } else {
      usage.recordHistogram()
    }

    UmaHistogramRecordValueToBucket(
      "Brave.VPN.DaysInMonthUsed",
      buckets: [
        0,
        1,
        2,
        .r(3...5),
        .r(6...10),
        .r(11...15),
        .r(16...20),
        .r(21...),
      ],
      value: braveVPNDaysInMonthUsedStorage.combinedValue
    )
  }

  func recordAccessibilityDisplayZoomEnabledP3A() {
    // Q100 Do you have iOS display zoom enabled?
    let isDisplayZoomEnabled = UIScreen.main.scale < UIScreen.main.nativeScale
    UmaHistogramBoolean("Brave.Accessibility.DisplayZoomEnabled", isDisplayZoomEnabled)
  }

  func recordAccessibilityDocumentsDirectorySizeP3A() {
    @Sendable func fetchDocumentsAndDataSize() async -> Int? {
      let fileManager = AsyncFileManager.default

      var directorySize = 0

      do {
        let documentsDirectory = try fileManager.url(for: .documentDirectory, in: .userDomainMask)
        let documentsDirectorySize = try await fileManager.sizeOfDirectory(at: documentsDirectory)
        directorySize += Int(documentsDirectorySize / 1024 / 1024)
      } catch {
        Logger.module.error("Cant fetch document directory size")
        return nil
      }

      let temporaryDirectory = FileManager.default.temporaryDirectory

      do {
        let temporaryDirectorySize = try await fileManager.sizeOfDirectory(at: temporaryDirectory)
        directorySize += Int(temporaryDirectorySize / 1024 / 1024)
      } catch {
        Logger.module.error("Cant fetch temporary directory size")
        return nil
      }

      return directorySize
    }

    let buckets: [Bucket] = [
      .r(0...50),
      .r(50...200),
      .r(200...500),
      .r(500...1000),
      .r(1000...),
    ]

    // Q103 What is the document directory size in MB?
    Task { @MainActor in
      if let documentsSize = await fetchDocumentsAndDataSize() {
        UmaHistogramRecordValueToBucket(
          "Brave.Core.DocumentsDirectorySizeMB",
          buckets: buckets,
          value: documentsSize
        )
      }
    }
  }

  func recordGeneralBottomBarLocationP3A() {
    if UIDevice.isIpad {
      return
    }

    enum Answer: Int, CaseIterable {
      case top = 0
      case bottom = 1
    }

    // Q101 Which location Bottom bar being used?
    let answer: Answer = Preferences.General.isUsingBottomBar.value ? .bottom : .top
    UmaHistogramEnumeration("Brave.General.BottomBarLocation", sample: answer)
  }

  func recordTimeBasedNumberReaderModeUsedP3A(activated: Bool) {
    var storage = P3ATimedStorage<Int>.readerModeActivated
    if activated {
      storage.add(value: 1, to: Date())
    }

    // Q102- How many times did you use reader mode in the last 7 days?
    UmaHistogramRecordValueToBucket(
      "Brave.ReaderMode.NumberReaderModeActivated",
      buckets: [
        0,
        .r(1...5),
        .r(5...20),
        .r(20...50),
        .r(51...),
      ],
      value: storage.combinedValue
    )
  }

  func recordAdsUsageType() {
    enum Answer: Int, CaseIterable {
      case none = 0
      case ntpOnly = 1
      case pushOnly = 2
      case ntpAndPush = 3
    }
    var answer: Answer = .none
    if rewards.ads.isEnabled && Preferences.NewTabPage.backgroundMediaType.isSponsored {
      answer = .ntpAndPush
    } else if rewards.ads.isEnabled {
      answer = .pushOnly
    } else if Preferences.NewTabPage.backgroundMediaType.isSponsored {
      answer = .ntpOnly
    }
    UmaHistogramEnumeration("Brave.Rewards.AdTypesEnabled", sample: answer)
  }

  func recordNavigationActionP3A(isNavigationActionForward: Bool) {
    var navigationActionStorage = P3ATimedStorage<Int>.navigationActionPerformedStorage
    var forwardNavigationActionStorage = P3ATimedStorage<Int>.forwardNavigationActionPerformed

    navigationActionStorage.add(value: 1, to: Date())
    let newNavigationActionStorage = navigationActionStorage.combinedValue

    if isNavigationActionForward {
      forwardNavigationActionStorage.add(value: 1, to: Date())
    }

    if newNavigationActionStorage > 0 {
      let navigationForwardPercent = Int(
        (Double(forwardNavigationActionStorage.combinedValue) / Double(newNavigationActionStorage))
          * 100.0
      )
      UmaHistogramRecordValueToBucket(
        "Brave.Toolbar.ForwardNavigationAction",
        buckets: [
          .r(0..<1),
          .r(1..<3),
          .r(3..<5),
          .r(5..<10),
          .r(10..<20),
          .r(20...),
        ],
        value: navigationForwardPercent
      )
    }
  }

  func maybeRecordBraveSearchDailyUsage(url: URL) {
    let braveSearchHost = "search.brave.com"
    let braveSearchPath = "/search"
    if url.host() != braveSearchHost || url.path() != braveSearchPath {
      return
    }
    UmaHistogramBoolean("Brave.Search.BraveDaily", true)
  }
}

extension P3AFeatureUsage {
  fileprivate static let braveVPNUsage: Self = .init(
    name: "vpn-usage",
    histogram: "Brave.VPN.LastUsageTime",
    returningUserHistogram: "Brave.VPN.NewUserReturning"
  )
}

extension P3ATimedStorage where Value == Int {
  /// Holds timed storage for question 21 (`Brave.Savings.BandwidthSavingsMB`)
  fileprivate static var dataSavedStorage: Self { .init(name: "data-saved", lifetimeInDays: 7) }
  fileprivate static var braveVPNDaysInMonthUsedStorage: Self {
    .init(name: "vpn-days-in-month-used", lifetimeInDays: 30)
  }
  fileprivate static var readerModeActivated: Self {
    .init(name: "reader-mode-activated", lifetimeInDays: 7)
  }
  fileprivate static var navigationActionPerformedStorage: Self {
    .init(name: "navigation-action-performed", lifetimeInDays: 7)
  }
  fileprivate static var forwardNavigationActionPerformed: Self {
    .init(name: "forward-navigation-action-performed", lifetimeInDays: 7)
  }
  fileprivate static var weeklyUsage: Self {
    .init(name: "browser-weekly-usage", lifetimeInDays: 14)
  }
  fileprivate static var urlSubmissionTotalStorage: Self {
    .init(name: "url-bar-existing-tabs", lifetimeInDays: 7)
  }
  fileprivate static var urlSubmissionNewTabStorage: Self {
    .init(name: "url-bar-new-tabs", lifetimeInDays: 7)
  }
  fileprivate static var tabCreationTotalStorage: Self {
    .init(name: "tabs-created-total", lifetimeInDays: 7)
  }
  fileprivate static var tabCreationFromToolbarStorage: Self {
    .init(name: "tabs-created-from-toolbar", lifetimeInDays: 7)
  }
}
