// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import Growth
import Data
import BraveShields
import BraveCore
import Shared
import os.log

extension BrowserViewController {
  
  func maybeRecordInitialShieldsP3A() {
    if Preferences.Shields.initialP3AStateReported.value { return }
    defer { Preferences.Shields.initialP3AStateReported.value = true }
    recordShieldsUpdateP3A(shield: .AdblockAndTp)
    recordShieldsUpdateP3A(shield: .FpProtection)
  }
  
  func recordShieldsUpdateP3A(shield: BraveShield) {
    let buckets: [Bucket] = [
      0,
      .r(1...5),
      .r(6...10),
      .r(11...20),
      .r(21...30),
      .r(31...),
    ]
    switch shield {
    case .AdblockAndTp:
      // Q51 On how many domains has the user set the adblock setting to be lower (block less) than the default?
      let adsBelowGlobalCount = Domain.totalDomainsWithAdblockShieldsLoweredFromGlobal()
      UmaHistogramRecordValueToBucket("Brave.Shields.DomainAdsSettingsBelowGlobal", buckets: buckets, value: adsBelowGlobalCount)
      // Q52 On how many domains has the user set the adblock setting to be higher (block more) than the default?
      let adsAboveGlobalCount = Domain.totalDomainsWithAdblockShieldsIncreasedFromGlobal()
      UmaHistogramRecordValueToBucket("Brave.Shields.DomainAdsSettingsAboveGlobal", buckets: buckets, value: adsAboveGlobalCount)
    case .FpProtection:
      // Q53 On how many domains has the user set the FP setting to be lower (block less) than the default?
      let fingerprintingBelowGlobalCount = Domain.totalDomainsWithFingerprintingProtectionLoweredFromGlobal()
      UmaHistogramRecordValueToBucket("Brave.Shields.DomainFingerprintSettingsBelowGlobal", buckets: buckets, value: fingerprintingBelowGlobalCount)
      // Q54 On how many domains has the user set the FP setting to be higher (block more) than the default?
      let fingerprintingAboveGlobalCount = Domain.totalDomainsWithFingerprintingProtectionIncreasedFromGlobal()
      UmaHistogramRecordValueToBucket("Brave.Shields.DomainFingerprintSettingsAboveGlobal", buckets: buckets, value: fingerprintingAboveGlobalCount)
    case .AllOff, .NoScript:
      break
    }
  }
  
  func recordDataSavedP3A(change: Int) {
    var dataSavedStorage = P3ATimedStorage<Int>.dataSavedStorage
    dataSavedStorage.add(value: change * BraveGlobalShieldStats.shared.averageBytesSavedPerItem, to: Date())
    
    // Values are in MB
    let buckets: [Bucket] = [
      0,
      .r(1...50),
      .r(51...100),
      .r(101...200),
      .r(201...400),
      .r(401...700),
      .r(701...1500),
      .r(1501...)
    ]
    let amountOfDataSavedInMB = dataSavedStorage.combinedValue / 1024 / 1024
    UmaHistogramRecordValueToBucket("Brave.Savings.BandwidthSavingsMB", buckets: buckets, value: amountOfDataSavedInMB)
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
    func fetchDocumentsAndDataSize() -> Int? {
      let fileManager = FileManager.default
      
      var directorySize = 0
      
      if let documentsDirectory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first {
        do {
          if let documentsDirectorySize = try fileManager.directorySize(at: documentsDirectory) {
            directorySize += Int(documentsDirectorySize / 1024 / 1024)
          }
        } catch {
          Logger.module.error("Cant fetch document directory size")
          return nil
        }
      }
      
      let temporaryDirectory = FileManager.default.temporaryDirectory
      
      do {
        if let temporaryDirectorySize = try fileManager.directorySize(at: temporaryDirectory) {
          directorySize += Int(temporaryDirectorySize / 1024 / 1024)
        }
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
    if let documentsSize = fetchDocumentsAndDataSize() {
      UmaHistogramRecordValueToBucket("Brave.Core.DocumentsDirectorySizeMB", buckets: buckets, value: documentsSize)
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
        .r(51...)
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
    if rewards.ads.isEnabled && Preferences.NewTabPage.backgroundSponsoredImages.value {
      answer = .ntpAndPush
    } else if rewards.ads.isEnabled {
      answer = .pushOnly
    } else if Preferences.NewTabPage.backgroundSponsoredImages.value {
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
      let navigationForwardPercent = Int((Double(forwardNavigationActionStorage.combinedValue) / Double(newNavigationActionStorage)) * 100.0)
      UmaHistogramRecordValueToBucket(
        "Brave.Toolbar.ForwardNavigationAction",
        buckets: [
          .r(0..<1),
          .r(1..<3),
          .r(3..<5),
          .r(5..<10),
          .r(10..<20),
          .r(20...)
        ],
        value: navigationForwardPercent
      )
    }
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
  fileprivate static var braveVPNDaysInMonthUsedStorage: Self { .init(name: "vpn-days-in-month-used", lifetimeInDays: 30) }
  fileprivate static var readerModeActivated: Self { .init(name: "reader-mode-activated", lifetimeInDays: 7) }
  fileprivate static var navigationActionPerformedStorage: Self { .init(name: "navigation-action-performed", lifetimeInDays: 7) }
  fileprivate static var forwardNavigationActionPerformed: Self { .init(name: "forward-navigation-action-performed", lifetimeInDays: 7) }
}
