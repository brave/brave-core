// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveStrings
import GuardianConnect
import SwiftUI

public struct BraveVPNRegionListView: View {

  @State
  private var isAutomatic: Bool

  @State
  private var isLoading = false

  @State
  private var isRegionDetailsPresented = false

  @State
  private var isShowingChangeRegionAlert = false

  @State
  private var selectedIndex = 0

  @State
  private var selectedRegion: GRDRegion?

  @State
  private var selectedRegionCities: [GRDRegion] = []

  @State
  private var regionModificationTimer: Timer?

  private var onServerRegionSet: ((_ region: GRDRegion?) -> Void)?

  public init(
    onServerRegionSet: ((_ region: GRDRegion?) -> Void)?
  ) {
    self.isAutomatic = BraveVPN.isAutomaticRegion
    self.onServerRegionSet = onServerRegionSet
  }

  public var body: some View {
    List {
      Section(
        footer: Text(Strings.VPN.serverRegionAutoSelectDescription)
          .font(.footnote)
          .foregroundStyle(Color(braveSystemName: .textSecondary))
      ) {
        automaticRegionToggle
      }

      if !isAutomatic {
        Section {
          ForEach(Array(BraveVPN.allRegions.enumerated()), id: \.offset) {
            index,
            region in
            countryRegionItem(at: index, region: region)
          }
        }
        .listRowBackground(Color(braveSystemName: .iosBrowserElevatedIos))
      }
    }
    .opacity(isLoading ? 0.5 : 1.0)
    .overlay {
      if isLoading {
        BraveVPNRegionLoadingIndicatorView()
          .transition(.opacity)
      }
    }
    .background {
      NavigationLink("", isActive: $isRegionDetailsPresented) {
        BraveRegionDetailsView(
          countryRegion: selectedRegion,
          with: selectedRegionCities
        )
      }
    }
    .onAppear {
      isAutomatic = BraveVPN.isAutomaticRegion
    }
    .onDisappear {
      cancelTimer()
    }
    .alert(isPresented: $isShowingChangeRegionAlert) {
      Alert(
        title: Text(Strings.VPN.regionPickerErrorTitle),
        message: Text(Strings.VPN.regionPickerErrorMessage),
        dismissButton: .default(Text(Strings.OKString))
      )
    }
    .onReceive(NotificationCenter.default.publisher(for: .NEVPNStatusDidChange)) { _ in
      if BraveVPN.isConnected {
        cancelTimer()
      }
    }
  }

  private func infoButtonView(index: Int) -> some View {
    Button(
      action: {
        guard !isLoading else {
          return
        }

        isRegionDetailsPresented = true
        if let designatedRegion = BraveVPN.allRegions[safe: index],
          let desiredRegion = BraveVPN.allRegions[safe: index]
        {
          selectedRegion = desiredRegion
          selectedRegionCities = designatedRegion.cities
        }
      },
      label: {
        Image(systemName: "info.circle")
          .foregroundStyle(Color(braveSystemName: .iconInteractive))
      }
    )
    .buttonStyle(.plain)
  }

  @ViewBuilder
  private func countryRegionItem(at index: Int, region: GRDRegion) -> some View {
    let isSelectedRegion = region.countryISOCode == BraveVPN.activatedRegion?.countryISOCode

    Button {
      selectDesignatedVPNRegion(at: index)
    } label: {
      HStack {
        region.countryISOCode.regionFlag ?? Image(braveSystemName: "leo.globe")
        VStack(alignment: .leading) {
          Text("\(region.displayName)")
            .font(.body)
            .foregroundStyle(
              isSelectedRegion
                ? Color(braveSystemName: .iconInteractive) : Color(braveSystemName: .textPrimary)
            )
          Text(generateServerCountDetails(for: region))
            .font(.footnote)
            .foregroundStyle(
              isSelectedRegion
                ? Color(braveSystemName: .iconInteractive) : Color(braveSystemName: .textSecondary)
            )
        }
        Spacer()
        if isSelectedRegion {
          Text(Strings.VPN.connectedRegionDescription)
            .font(.body)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
        }
        infoButtonView(index: index)
          .hidden()
      }
    }
    .overlay(alignment: .trailing) {
      infoButtonView(index: index)
    }
  }

  private var automaticRegionToggle: some View {
    Toggle(
      isOn: Binding(
        get: { isAutomatic },
        set: { enableAutomaticServer($0) }
      )
    ) {
      VStack(alignment: .leading) {
        Text(Strings.VPN.automaticServerSelectionToggleTitle)
          .font(.body)
        if isAutomatic, let regionAutomaticName = BraveVPN.lastKnownRegion?.displayName {
          Text(regionAutomaticName)
            .font(.footnote)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
        }
      }
    }
    .disabled(isLoading)
    .foregroundStyle(Color(braveSystemName: .textPrimary))
    .tint(.accentColor)
    .listRowBackground(Color(braveSystemName: .iosBrowserElevatedIos))
  }

  private func generateServerCountDetails(for region: GRDRegion) -> String {
    let cityCount = region.cities.count
    let serverCount = Int(truncating: region.serverCount)

    let cityCountTitle =
      cityCount > 1
      ? String(format: Strings.VPN.multipleCityCountTitle, cityCount)
      : String(format: Strings.VPN.cityCountTitle, cityCount)

    let serverCountTitle =
      serverCount > 1
      ? String(format: Strings.VPN.multipleServerCountTitle, serverCount)
      : String(format: Strings.VPN.serverCountTitle, serverCount)

    return "\(cityCountTitle) - \(serverCountTitle)"
  }

  private func enableAutomaticServer(_ enabled: Bool) {
    isAutomatic = enabled

    guard isAutomatic else {
      let autoRegion = BraveVPN.allRegions.first(where: {
        $0.countryISOCode == BraveVPN.lastKnownRegion?.countryISOCode
      })
      changeCountryRegion(with: autoRegion)
      return
    }

    // Implementation detail: nil region means we use an automatic way to connect to the host.
    changeCountryRegion(with: nil)
  }

  private func selectDesignatedVPNRegion(at index: Int) {
    guard !isLoading, let desiredRegion = BraveVPN.allRegions[safe: index],
      desiredRegion.regionName != BraveVPN.selectedRegion?.regionName
    else {
      return
    }

    changeCountryRegion(with: desiredRegion)
  }

  private func changeCountryRegion(with region: GRDRegion?) {
    isLoading = true

    Task { @MainActor in
      let success = await BraveVPN.changeVPNRegionForPrecision(to: region, with: .country)

      isLoading = false

      if success {
        selectedRegion = region
        onServerRegionSet?(region)
        // Changing vpn server settings takes lot of time,
        // and nothing we can do about it as it relies on Apple apis.
        // Here we observe vpn status and we show success alert if it connected,
        // otherwise an error alert is show if it did not manage to connect in 60 seconds.
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
struct ServerRegionView_Previews: PreviewProvider {
  static var previews: some View {
    BraveVPNRegionListView(onServerRegionSet: nil)
  }
}
#endif
