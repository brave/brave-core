// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import GuardianConnect
import SwiftUI

struct BraveRegionDetailsView: View {

  @State
  private var isAutoSelectEnabled: Bool

  @State
  private var isLoading = false

  @State
  private var isConfirmationPresented = false

  @ObservedObject
  private var cityRegionDetail: CityRegionDetail

  public init(
    countryRegion: GRDRegion?,
    with cityRegions: [GRDRegion] = [],
    isAutoSelectEnabled: Bool = true
  ) {
    self.isAutoSelectEnabled = isAutoSelectEnabled

    var regions: [CityRegion] = []

    for cityRegion in cityRegions {
      regions.append(
        CityRegion(displayName: cityRegion.displayName, regionName: cityRegion.regionName)
      )
    }

    cityRegionDetail = CityRegionDetail(
      countryName: countryRegion?.country ?? "",
      countryISOCode: countryRegion?.countryISOCode ?? "",
      cityRegions: regions
    )
  }

  var body: some View {
    ZStack {
      List {
        Section(header: Text("AVAILABLE SERVERS")) {
          ForEach(cityRegionDetail.cityRegions) { server in
            cityRegionItem(at: 0, region: server)
          }
        }
        .listRowBackground(Color(braveSystemName: .containerBackgroundMobile))
      }
      .opacity(isLoading ? 0.5 : 1.0)

      if isLoading {
        BraveVPNRegionLoadingIndicatorView()
          .transition(.opacity)
          .zIndex(1)
      }
    }
    .navigationBarTitle("\(cityRegionDetail.countryName) Server", displayMode: .inline)
    .background {
      if let region = cityRegionDetail.selectedRegion {
        BraveVPNRegionConfirmationContentView(
          isPresented: $isConfirmationPresented,
          regionCountry: cityRegionDetail.countryName,
          regionCity: region.displayName,
          regionCountryISOCode: cityRegionDetail.countryISOCode
        )
      }
    }
  }

  @ViewBuilder
  private func cityRegionItem(at index: Int, region: CityRegion) -> some View {
    HStack {
      VStack(alignment: .leading) {
        Text(region.displayName.capitalizeFirstLetter)
          .foregroundStyle(
            cityRegionDetail.selectedRegion == region
              ? Color(braveSystemName: .textInteractive)
              : Color(braveSystemName: .textPrimary)
          )

        if region.isAutomatic == true {
          Text("Use the best server available")
            .foregroundStyle(
              cityRegionDetail.selectedRegion == region
                ? Color(braveSystemName: .textInteractive)
                : Color(braveSystemName: .textPrimary)
            )
        }
      }
      Spacer()

      if cityRegionDetail.selectedRegion == region {
        Image(systemName: "checkmark")
          .foregroundStyle(Color(braveSystemName: .iconInteractive))
      }
    }
    .contentShape(Rectangle())
    .onTapGesture {
      selectDesignatedVPNServer(region)
    }
  }

  private func selectDesignatedVPNServer(_ region: CityRegion) {
    guard !isLoading, cityRegionDetail.selectedRegion?.regionName != region.regionName else {
      return
    }

    isLoading = true

    // TODO: Select Region
    Task.delayed(bySeconds: 3) { @MainActor in
      cityRegionDetail.selectedRegion = region

      isLoading = false
      isConfirmationPresented = true

      Task.delayed(bySeconds: 2) { @MainActor in
        isConfirmationPresented = false
      }
    }
  }
}

struct BraveRegionDetailsView_Previews: PreviewProvider {
  static var previews: some View {
    BraveRegionDetailsView(countryRegion: GRDRegion(dictionary: [:]))
  }
}
