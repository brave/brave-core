// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import Growth
import Data
import BraveShields

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
    case .AllOff, .NoScript, .SafeBrowsing:
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
    
    usage.recordReturningUsageMetric()
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
}
