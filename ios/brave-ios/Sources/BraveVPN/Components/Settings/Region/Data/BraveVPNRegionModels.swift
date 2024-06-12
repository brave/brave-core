// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import GuardianConnect

struct CityRegion: Identifiable, Equatable {
  let id = UUID()
  let displayName: String
  let regionName: String
  var isAutomatic = false
}

class CityRegionDetail: ObservableObject {
  var cityRegions = [
    CityRegion(displayName: "Optimal", regionName: "auto", isAutomatic: true)
  ]

  var countryName: String
  var countryISOCode: String

  @Published var selectedRegion: CityRegion? = nil

  init(
    isAutoSelectEnabled: Bool = true,
    countryName: String,
    countryISOCode: String,
    cityRegions: [CityRegion]
  ) {
    self.countryName = countryName
    self.countryISOCode = countryISOCode
    self.cityRegions.append(contentsOf: cityRegions)

    if isAutoSelectEnabled {
      selectedRegion = self.cityRegions.first
    }
  }
}
