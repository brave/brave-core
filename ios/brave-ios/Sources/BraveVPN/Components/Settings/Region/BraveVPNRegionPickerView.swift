// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import GuardianConnect
import SwiftUI

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

          if !isAutomatic {
            Section {
              ForEach(Array(serverRegionDetail.serverRegions.enumerated()), id: \.offset) {
                index,
                region in
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
      BraveVPNRegionChangedContentView(
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

}

struct ServerRegionView_Previews: PreviewProvider {
  static var previews: some View {
    BraveVPNRegionPickerView(isAutomatic: false)
  }
}
