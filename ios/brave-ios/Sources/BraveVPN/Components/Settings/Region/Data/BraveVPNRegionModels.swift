// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import GuardianConnect

struct VPNCityRegion: Identifiable, Equatable {

  static let optimalCityRegionName = "auto"

  let id = UUID()
  let displayName: String
  let regionName: String
  var isAutomatic = false
}

class VPNCityRegionDetail: ObservableObject {
  var cityRegions = [
    VPNCityRegion(
      displayName: "Optimal",
      regionName: VPNCityRegion.optimalCityRegionName,
      isAutomatic: true
    )
  ]

  var countryName: String
  var countryISOCode: String

  @Published var selectedRegion: VPNCityRegion? = nil

  init(
    isAutoSelectEnabled: Bool = true,
    countryName: String,
    countryISOCode: String,
    cityRegions: [VPNCityRegion]
  ) {
    self.countryName = countryName
    self.countryISOCode = countryISOCode
    self.cityRegions.append(contentsOf: cityRegions)

    if isAutoSelectEnabled {
      selectedRegion = self.cityRegions.first
    }
  }
}
