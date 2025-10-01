// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import BraveStrings
import BraveUI
import GuardianConnect
import SwiftUI

public struct BraveVPNRegionListView: View {

  @State
  private var isAutomaticRegion: Bool = false

  @State
  private var isLoading = false

  @State
  private var isRegionDetailsPresented = false

  @State
  private var isShowingChangeRegionAlert = false

  @State
  private var allRegions: [GRDRegion] = []

  @State
  private var selectedIndex = 0

  @State
  private var selectedRegion: GRDRegion?

  @State
  private var selectedRegionCities: [GRDRegion] = []

  @State
  private var isVPNEnabled = BraveVPN.isConnected

  @State
  private var regionModificationTimer: Timer?

  private var onServerRegionSet: ((_ region: GRDRegion?) -> Void)?

  public init(
    onServerRegionSet: ((_ region: GRDRegion?) -> Void)?
  ) {
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

      if !isAutomaticRegion {
        Section {
          ForEach(Array(allRegions.enumerated()), id: \.offset) {
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
      Task { @MainActor in
        isLoading = true
        isAutomaticRegion = BraveVPN.isAutomaticRegion
        allRegions = await BraveVPN.fetchRegionData() ?? []
        isLoading = false
      }
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
      self.isVPNEnabled = BraveVPN.isConnected
      if self.isVPNEnabled {
        cancelTimer()
      }
    }
  }

  @ViewBuilder
  private func countryRegionItem(at index: Int, region: GRDRegion) -> some View {
    BraveVPNCountryRegionView(region: region) {
      selectDesignatedVPNRegion(at: index)
    } onInfoButtonPressed: {
      guard !isLoading else {
        return
      }

      isRegionDetailsPresented = true
      if let designatedRegion = allRegions[safe: index],
        let desiredRegion = allRegions[safe: index]
      {
        selectedRegion = desiredRegion
        selectedRegionCities = designatedRegion.cities
      }
    }
  }

  private var automaticRegionToggle: some View {
    Toggle(
      isOn: Binding(
        get: { isAutomaticRegion },
        set: { enableAutomaticServer($0) }
      )
    ) {
      VStack(alignment: .leading) {
        Text(Strings.VPN.automaticServerSelectionToggleTitle)
          .font(.body)
        if isAutomaticRegion, let regionAutomaticName = BraveVPN.lastKnownRegion?.displayName {
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

  private func enableAutomaticServer(_ enabled: Bool) {
    isAutomaticRegion = enabled

    guard isAutomaticRegion else {
      let autoRegion = allRegions.first(where: {
        $0.countryISOCode == BraveVPN.lastKnownRegion?.countryISOCode
      })
      changeCountryRegion(with: autoRegion)
      return
    }

    // Implementation detail: nil region means we use an automatic way to connect to the host.
    changeCountryRegion(with: nil)
  }

  private func selectDesignatedVPNRegion(at index: Int) {
    guard !isLoading, let desiredRegion = allRegions[safe: index] else {
      return
    }

    // If we're already connected to this region, do nothing.
    if desiredRegion.regionName == BraveVPN.selectedRegion?.regionName && isVPNEnabled {
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

private struct BraveVPNCountryRegionView: View {
  @State
  private var showPopover = false

  let region: GRDRegion
  let onRegionSelected: () -> Void
  let onInfoButtonPressed: () -> Void

  var body: some View {
    let isSelectedRegion = region.countryISOCode == BraveVPN.activatedRegion?.countryISOCode

    Button {
      onRegionSelected()
    } label: {
      HStack {
        region.countryISOCode.regionFlag ?? Image(braveSystemName: "leo.globe")
        VStack(alignment: .leading) {
          HStack {
            Text("\(region.displayName)")
              .font(.body)
              .foregroundStyle(
                isSelectedRegion
                  ? Color(braveSystemName: .iconInteractive) : Color(braveSystemName: .textPrimary)
              )

            Button {
              showPopover.toggle()
            } label: {
              Label {
                Text(Strings.VPN.smartProxyPopoverTitle)
              } icon: {
                Image(braveSystemName: "leo.smart.proxy-routing")
                  .foregroundStyle(Color(braveSystemName: .iconDefault))
                  .padding(4.0)
                  .background(Color(braveSystemName: .containerHighlight))
                  .clipShape(RoundedRectangle(cornerRadius: 8.0, style: .continuous))
              }
              .labelStyle(.iconOnly)
            }
            .bravePopover(isPresented: $showPopover, arrowDirection: [.down]) {
              Text(Strings.VPN.smartProxyPopoverTitle)
                .font(.subheadline)
                .foregroundStyle(Color(braveSystemName: .textTertiary))
                .fixedSize(horizontal: false, vertical: true)
                .padding()
            }
          }
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
        infoButtonView()
          .hidden()
      }
    }
    .overlay(alignment: .trailing) {
      infoButtonView()
    }
  }

  private func infoButtonView() -> some View {
    Button(
      action: {
        onInfoButtonPressed()
      },
      label: {
        Image(systemName: "info.circle")
          .foregroundStyle(Color(braveSystemName: .iconInteractive))
      }
    )
    .buttonStyle(.plain)
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
}

#if DEBUG
struct ServerRegionView_Previews: PreviewProvider {
  static var previews: some View {
    BraveVPNRegionListView(onServerRegionSet: nil)
  }
}
#endif
