// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared
import Growth
import Data

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
}
