// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import GuardianConnect
import SwiftUI

public struct BraveVPNRegionPickerView: View {

  @State
  private var isAutomatic: Bool

  @State
  private var isLoading = false

  @State
  private var isRegionDetailsPresented = false

  @State
  private var isConfirmationPresented = false

  @State
  private var selectedIndex = 0

  @State
  private var selectedRegion: GRDRegion?

  @State
  private var selectedRegionCities: [GRDRegion] = []

  @State
  private var regionModificationTimer: Timer?

  public init() {
    self.isAutomatic = BraveVPN.isAutomaticRegion
  }

  public var body: some View {
    ZStack {
      VStack {
        List {
          Section(
            footer: Text(
              "Auto-select the VPN server region closest to you based on your timezone. This option is recommended to maximize Internet speeds."
            )
            .font(.footnote)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
          ) {
            automaticRegionToggle
          }

          if !isAutomatic {
            Section {
              ForEach(Array(BraveVPN.allCountryRegions.enumerated()), id: \.offset) {
                index,
                region in
                countryRegionItem(at: index, region: region)
              }
            }
            .listRowBackground(Color(braveSystemName: .containerBackgroundMobile))
          }
        }
      }
      .opacity(isLoading ? 0.5 : 1.0)

      if isLoading {
        BraveVPNRegionLoadingIndicatorView()
          .transition(.opacity)
          .zIndex(1)
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
    .background {
      BraveVPNRegionConfirmationContentView(
        isPresented: $isConfirmationPresented,
        regionCountry: selectedRegion?.displayName,
        regionCountryISOCode: selectedRegion?.countryISOCode
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

  private func infoButtonView(index: Int) -> some View {
    Button(
      action: {
        guard !isLoading else {
          return
        }

        isRegionDetailsPresented = true
        if let designatedRegion = BraveVPN.allCountryRegions[safe: index],
          let desiredRegion = BraveVPN.allCountryRegions[safe: index]
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
    let serverCount = region.cities.count

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
          Text(serverCount > 1 ? "\(serverCount) servers" : "1 server")
            .font(.footnote)
            .foregroundStyle(
              isSelectedRegion
                ? Color(braveSystemName: .iconInteractive) : Color(braveSystemName: .textSecondary)
            )
        }
        Spacer()
        if isSelectedRegion {
          Text("Connected")
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
        Text("Automatic")
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
    .listRowBackground(Color(braveSystemName: .containerBackgroundMobile))
  }

  private func enableAutomaticServer(_ enabled: Bool) {
    isAutomatic = enabled

    // Implementation detail: nil region means we use an automatic way to connect to the host.
    changeCountryRegion(with: nil) {
      // TODO: Show Alert if it fails

    }
  }

  private func selectDesignatedVPNRegion(at index: Int, isAutomatic: Bool = false) {
    guard !isLoading, let desiredRegion = BraveVPN.allCountryRegions[safe: index],
      desiredRegion.regionName != BraveVPN.selectedRegion?.regionName,
      desiredRegion.countryISOCode != BraveVPN.selectedRegion?.countryISOCode
    else {
      return
    }

    changeCountryRegion(with: desiredRegion) {
      // TODO: Show Alert if it fails
    }
  }

  private func changeCountryRegion(with region: GRDRegion?, failure: @escaping () -> Void) {
    isLoading = true

    Task { @MainActor in
      let success = await BraveVPNRegionManager.shared.changeVPNRegionAsync(to: region)

      isLoading = false

      if success {
        selectedRegion = region

        // Changing vpn server settings takes lot of time,
        // and nothing we can do about it as it relies on Apple apis.
        // Here we observe vpn status and we show success alert if it connected,
        // otherwise an error alert is show if it did not manage to connect in 60 seconds.
        cancelTimer()
        regionModificationTimer = Timer.scheduledTimer(withTimeInterval: 60.0, repeats: false) {
          _ in
          failure()
        }
      } else {
        failure()
      }
    }
  }

  private func cancelTimer() {
    // Invalidate the modification timer if it is still running
    regionModificationTimer?.invalidate()
    regionModificationTimer = nil
  }
}

struct ServerRegionView_Previews: PreviewProvider {
  static var previews: some View {
    BraveVPNRegionPickerView()
  }
}
