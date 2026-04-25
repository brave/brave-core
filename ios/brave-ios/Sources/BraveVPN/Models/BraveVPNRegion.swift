// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import Foundation
import GuardianConnect
import Shared
import SwiftUI

struct BraveVPNCityRegion: Identifiable, Equatable {

  static let optimalCityRegionName = "auto"

  let id = UUID()
  let displayName: String
  let regionName: String
  var serverCount: Int = 0
  var isAutomatic = false
}

class VPNCityRegionDetail: ObservableObject {
  var cityRegions = [
    BraveVPNCityRegion(
      displayName: Strings.VPN.vpnCityRegionOptimalTitle,
      regionName: BraveVPNCityRegion.optimalCityRegionName,
      isAutomatic: true
    )
  ]

  var countryName: String = ""

  @Published var selectedRegion: BraveVPNCityRegion? = nil

  func assignSelectedRegion(
    countryName: String,
    cityRegions: [BraveVPNCityRegion]
  ) {
    self.countryName = countryName
    self.cityRegions.append(contentsOf: cityRegions)

    let selectedCity = cityRegions.first(where: {
      $0.regionName == BraveVPN.selectedRegion?.regionName
    })

    guard let selectedCity = selectedCity else {
      selectedRegion = self.cityRegions.first
      return
    }

    selectedRegion = selectedCity
  }
}

extension GRDRegion {
  /// The title used in menu while chaging region
  public var settingTitle: String {
    var settingSelection = displayName

    if BraveVPN.isAutomaticRegion {
      settingSelection = Strings.VPN.regionPickerAutomaticModeCellText
    }

    return String(format: Strings.VPN.vpnRegionSelectorButtonSubTitle, settingSelection)
  }

  /// Flag of the region as an image
  public var regionFlag: Image? {
    // Root Unicode flags index
    let rootIndex: UInt32 = 127397
    var unicodeScalarView = ""

    for scalar in countryISOCode.unicodeScalars {
      // Shift the letter index to the flags index
      if let appendedScalar = UnicodeScalar(rootIndex + scalar.value) {
        // Append symbol to the Unicode string
        unicodeScalarView.unicodeScalars.append(appendedScalar)
      }
    }

    if unicodeScalarView.isEmpty {
      return nil
    }

    return Image(uiImage: unicodeScalarView.image())
  }
}
