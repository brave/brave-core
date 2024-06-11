// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Foundation
import GuardianConnect

public class BraveVPNRegionManager {

  public static let shared = BraveVPNRegionManager()

  private let vpnRegionListEndPoint =
    "https://connect-api.guardianapp.com/api/v1.3/servers/all-server-regions/city-by-country"

  private var sessionManager: URLSession

  private init() {
    sessionManager = URLSession(configuration: .default)
  }

  public func getAllRegionsWithDetails() async -> [GRDRegion] {
    guard let endPoint = URL(string: vpnRegionListEndPoint) else {
      return []
    }

    do {
      let (result, _) = try await sessionManager.request(endPoint)

      if let resultData = result as? Data {
        if let jsonRegionList = try JSONSerialization.jsonObject(with: resultData, options: [])
          as? [[String: Any]]
        {
          var regionList: [GRDRegion] = []

          for region in jsonRegionList {
            let regionItem = GRDRegion(dictionary: region)
            regionList.append(regionItem)
          }

          let sortedRegions = regionList.sorted { $0.displayName < $1.displayName }

          print("Regions Sorted \(sortedRegions)")

          return sortedRegions
        }
      }
    } catch {
      print("API All Regions Error \(error)")
      return []
    }

    return []
  }

}
