// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import GuardianConnect

public struct ServerRegion: Identifiable, Equatable {
  public let id = UUID()
  let name: String
  let servers: Int
  let countryISOCode: String
}

public class ServerRegionDetail: ObservableObject {
  public var serverRegions: [ServerRegion] = [
    ServerRegion(name: "Australia", servers: 5, countryISOCode: "AU"),
    ServerRegion(name: "Brazil", servers: 4, countryISOCode: "BR"),
    ServerRegion(name: "Canada", servers: 2, countryISOCode: "CA"),
    ServerRegion(name: "France", servers: 3, countryISOCode: "FR"),
    ServerRegion(name: "Germany", servers: 5, countryISOCode: "DE"),
    ServerRegion(name: "Italy", servers: 3, countryISOCode: "IT"),
    ServerRegion(name: "Japan", servers: 7, countryISOCode: "JP"),
    ServerRegion(name: "Mexico", servers: 9, countryISOCode: "MX"),
    ServerRegion(name: "Netherlands", servers: 1, countryISOCode: "NL"),
  ]

  @Published var selectedRegion: ServerRegion? = nil

  public init() {
    selectedRegion = serverRegions.first
  }
}
