// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveStrings
import GuardianConnect
import SwiftUI

struct BraveRegionDetailsView: View {

  @State
  private var isLoading = false

  @State
  private var isConfirmationPresented = false

  @State
  private var isShowingChangeRegionAlert = false

  @StateObject
  private var cityRegionDetail = VPNCityRegionDetail()

  @State
  private var regionModificationTimer: Timer?

  private var countryRegion: GRDRegion?

  private var cityRegions: [GRDRegion]

  public init(
    countryRegion: GRDRegion?,
    with cityRegions: [GRDRegion] = []
  ) {
    self.countryRegion = countryRegion
    self.cityRegions = cityRegions
  }

  var body: some View {
    List {
      Section(header: Text(Strings.VPN.availableServerTitle)) {
        ForEach(cityRegionDetail.cityRegions) { server in
          cityRegionItem(region: server)
        }
      }
      .listRowBackground(Color(braveSystemName: .iosBrowserElevatedIos))
    }
    .opacity(isLoading ? 0.5 : 1.0)
    .overlay {
      if isLoading {
        BraveVPNRegionLoadingIndicatorView()
          .transition(.opacity)
      }
    }
    .navigationBarTitle(
      String(format: Strings.VPN.serverNameTitle, cityRegionDetail.countryName),
      displayMode: .inline
    )
    .background {
      BraveVPNRegionConfirmationContentView(
        isPresented: $isConfirmationPresented,
        country: BraveVPN.serverLocationDetailed.country,
        city: BraveVPN.serverLocationDetailed.city,
        countryISOCode: BraveVPN.serverLocation.isoCode
      )
    }
    .onAppear {
      let regions = cityRegions.map {
        BraveVPNCityRegion(
          displayName: $0.displayName,
          regionName: $0.regionName,
          serverCount: Int(truncating: $0.serverCount)
        )
      }

      cityRegionDetail.assignSelectedRegion(
        countryName: countryRegion?.country ?? "",
        cityRegions: regions
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
    .alert(isPresented: $isShowingChangeRegionAlert) {
      Alert(
        title: Text(Strings.VPN.regionPickerErrorTitle),
        message: Text(Strings.VPN.regionPickerErrorMessage),
        dismissButton: .default(Text(Strings.OKString))
      )
    }
  }

  @ViewBuilder
  private func cityRegionItem(region: BraveVPNCityRegion) -> some View {
    HStack {
      VStack(alignment: .leading) {
        Text(region.displayName.capitalizeFirstLetter)
          .foregroundStyle(
            cityRegionDetail.selectedRegion == region
              ? Color(braveSystemName: .textInteractive)
              : Color(braveSystemName: .textPrimary)
          )

        Text(generateServerCountDetails(for: region))
          .foregroundStyle(
            cityRegionDetail.selectedRegion == region
              ? Color(braveSystemName: .textInteractive)
              : Color(braveSystemName: .textPrimary)
          )
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

  private func generateServerCountDetails(for region: BraveVPNCityRegion) -> String {
    let serverCount = region.serverCount

    let serverCountTitle =
      serverCount > 1
      ? String(format: Strings.VPN.multipleServerCountTitle, serverCount)
      : String(format: Strings.VPN.serverCountTitle, serverCount)

    return region.isAutomatic == true
      ? Strings.VPN.vpnCityRegionOptimalDescription : serverCountTitle
  }

  private func selectDesignatedVPNCity(_ region: BraveVPNCityRegion) {
    guard !isLoading, cityRegionDetail.selectedRegion?.regionName != region.regionName else {
      return
    }

    isLoading = true

    Task { @MainActor in
      var regionToChange: GRDRegion?
      var regionPrecision: BraveVPN.RegionPrecision

      // If the optimal server is chosen, we activate the country
      // This is done in order to handle auto selection
      // cities among the list in selected region/country
      if region.regionName == BraveVPNCityRegion.optimalCityRegionName {
        regionToChange = countryRegion
        regionPrecision = .country
      } else {
        regionToChange = cityRegions.filter { $0.regionName == region.regionName }.first
        regionPrecision = .city
      }

      let success = await BraveVPN.changeVPNRegionForPrecision(
        to: regionToChange,
        with: regionPrecision
      )

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
