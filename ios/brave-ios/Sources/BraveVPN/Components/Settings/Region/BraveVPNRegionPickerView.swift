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

  @ObservedObject
  private var serverRegionDetail = ServerRegionDetail()

  public init(isAutomatic: Bool, serverRegionDetail: ServerRegionDetail? = nil) {
    self.isAutomatic = isAutomatic

    if let serverRegionDetail = serverRegionDetail {
      self.serverRegionDetail = serverRegionDetail
    }

    BraveVPN.populateRegionDataIfNecessary()
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
              ForEach(Array(serverRegionDetail.serverRegions.enumerated()), id: \.offset) {
                index,
                region in
                regionItem(at: index, region: region)
              }
            }
            .listRowBackground(Color(braveSystemName: .containerBackgroundMobile))
          }
        }
      }
      .background {
        NavigationLink("", isActive: $isRegionDetailsPresented) {
          BraveRegionDetailsView()
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
      BraveVPNRegionConfirmationContentView(
        isPresented: $isConfirmationPresented,
        regionTitle: "Brazil",
        regionSubtitle: "Rio de Janeiro"
      )
    }
  }

  private func infoButtonView(index: Int) -> some View {
    Button(
      action: {
        guard !isLoading else {
          return
        }

        isRegionDetailsPresented = true
      },
      label: {
        Image(systemName: "info.circle")
          .foregroundStyle(Color(braveSystemName: .iconInteractive))
      }
    )
    .buttonStyle(.plain)
  }

  private func enableAutomaticServer(_ enabled: Bool) {
    isAutomatic = enabled
  }

  private func selectDesignatedVPNRegion(at index: Int) {
    guard !isLoading, let desiredRegion = serverRegionDetail.serverRegions[safe: index],
      desiredRegion.id != serverRegionDetail.selectedRegion?.id
    else {
      return
    }

    isLoading = true

    // TODO: Select Region
    Task.delayed(bySeconds: 3) { @MainActor in
      serverRegionDetail.selectedRegion = desiredRegion

      isLoading = false
      isConfirmationPresented = true

      Task.delayed(bySeconds: 2) { @MainActor in
        isConfirmationPresented = false
      }
    }
  }

  @ViewBuilder
  private func regionItem(at index: Int, region: ServerRegion) -> some View {
    Button {
      selectDesignatedVPNRegion(at: index)
    } label: {
      HStack {
        region.countryISOCode.regionFlag ?? Image(braveSystemName: "leo.globe")
        VStack(alignment: .leading) {
          Text("\(region.name)")
            .font(.body)
            .foregroundStyle(
              region == serverRegionDetail.selectedRegion
                ? Color(braveSystemName: .iconInteractive)
                : Color(braveSystemName: .textPrimary)
            )
          Text("\(region.servers) servers")
            .font(.footnote)
            .foregroundStyle(
              region == serverRegionDetail.selectedRegion
                ? Color(braveSystemName: .iconInteractive)
                : Color(braveSystemName: .textSecondary)
            )
        }
        Spacer()
        if region == serverRegionDetail.selectedRegion {
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
        if isAutomatic, let regionAutomaticName = serverRegionDetail.selectedRegion?.name {
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
}

struct ServerRegionView_Previews: PreviewProvider {
  static var previews: some View {
    BraveVPNRegionPickerView(isAutomatic: false)
  }
}
