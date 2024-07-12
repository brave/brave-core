// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import GuardianConnect
import SwiftUI

struct BraveRegionDetailsView: View {

  @State
  private var isAutoSelectEnabled: Bool

  @State
  private var isLoading = false

  @State
  private var isConfirmationPresented = false

  @State
  private var isShowingChangeRegionAlert = false

  @ObservedObject
  private var cityRegionDetail: VPNCityRegionDetail

  @State
  private var regionModificationTimer: Timer?

  @State
  private var cityRegions: [GRDRegion]

  public init(
    countryRegion: GRDRegion?,
    with cityRegions: [GRDRegion] = [],
    isAutoSelectEnabled: Bool = true
  ) {
    self.isAutoSelectEnabled = isAutoSelectEnabled
    self.cityRegions = cityRegions

    var regions: [BraveVPNCityRegion] = []

    for cityRegion in cityRegions {
      regions.append(
        BraveVPNCityRegion(displayName: cityRegion.displayName, regionName: cityRegion.regionName)
      )
    }

    cityRegionDetail = VPNCityRegionDetail(
      countryName: countryRegion?.country ?? "",
      countryISOCode: countryRegion?.countryISOCode ?? "",
      cityRegions: regions
    )
  }

  var body: some View {
    ZStack {
      List {
        Section(header: Text(Strings.VPN.availableServerTitle)) {
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
    .navigationBarTitle(
      String(format: Strings.VPN.serverNameTitle, cityRegionDetail.countryName),
      displayMode: .inline
    )
    .background {
      BraveVPNRegionConfirmationContentView(
        isPresented: $isConfirmationPresented,
        regionCountry: BraveVPN.serverLocationDetailed.country,
        regionCity: BraveVPN.serverLocationDetailed.city,
        regionCountryISOCode: BraveVPN.serverLocation.isoCode
      )
    }
    .alert(isPresented: $isShowingChangeRegionAlert) {
      Alert(
        title: Text(Strings.VPN.regionPickerErrorTitle),
        message: Text(Strings.VPN.regionPickerErrorMessage),
        dismissButton: .default(Text(Strings.OKString))
      )
    }
    .onReceive(NotificationCenter.default.publisher(for: .NEVPNStatusDidChange)) { _ in
      let isVPNEnabled = BraveVPN.isConnected

      if isVPNEnabled {
        cancelTimer()
        isConfirmationPresented = true

        // Dismiss confirmation dialog automatically
        Task.delayed(bySeconds: 2) { @MainActor in
          isConfirmationPresented = false
        }
      }
    }
  }

  @ViewBuilder
  private func cityRegionItem(at index: Int, region: BraveVPNCityRegion) -> some View {
    HStack {
      VStack(alignment: .leading) {
        Text(region.displayName.capitalizeFirstLetter)
          .foregroundStyle(
            cityRegionDetail.selectedRegion == region
              ? Color(braveSystemName: .textInteractive)
              : Color(braveSystemName: .textPrimary)
          )

        if region.isAutomatic == true {
          Text(Strings.VPN.vpnCityRegionOptimalDescription)
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
      selectDesignatedVPNCity(region)
    }
  }

  private func selectDesignatedVPNCity(_ region: BraveVPNCityRegion) {
    guard !isLoading, cityRegionDetail.selectedRegion?.regionName != region.regionName else {
      return
    }

    isLoading = true

    Task { @MainActor in
      var regionToChange: GRDRegion?

      // If the optimal server is chosen, we activate the country
      // This is done in order to handle auto selection
      // cities among the list in selected region/country
      if region.regionName == BraveVPNCityRegion.optimalCityRegionName {
        regionToChange = nil
      } else {
        regionToChange = cityRegions.filter { $0.regionName == region.regionName }.first
      }

      let success = await BraveVPN.changeVPNRegionForPrecision(to: regionToChange, with: .city)

      isLoading = false

      if success {
        cityRegionDetail.selectedRegion = region

        cancelTimer()
        regionModificationTimer = Timer.scheduledTimer(withTimeInterval: 60.0, repeats: false) {
          _ in
          isShowingChangeRegionAlert = true
        }
      } else {
        isShowingChangeRegionAlert = true
      }
    }
  }

  private func cancelTimer() {
    // Invalidate the modification timer if it is still running
    regionModificationTimer?.invalidate()
    regionModificationTimer = nil
  }
}

#if DEBUG
struct BraveRegionDetailsView_Previews: PreviewProvider {
  static var previews: some View {
    BraveRegionDetailsView(countryRegion: GRDRegion(dictionary: [:]))
  }
}
#endif
