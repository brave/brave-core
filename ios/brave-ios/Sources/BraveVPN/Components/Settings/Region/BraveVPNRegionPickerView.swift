// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import GuardianConnect
import SwiftUI

public struct ServerRegion: Identifiable, Equatable {
  public let id = UUID()
  let name: String
  let servers: Int
}

public class ServerRegionDetail: ObservableObject {
  public var serverRegions: [ServerRegion] = [
    ServerRegion(name: "Australia", servers: 5),
    ServerRegion(name: "Brazil", servers: 4),
    //     ServerRegion(name: "Brazil", servers: 4, isConnected: true),
    ServerRegion(name: "Canada", servers: 2),
    ServerRegion(name: "France", servers: 3),
    ServerRegion(name: "Germany", servers: 5),
    ServerRegion(name: "Italy", servers: 3),
    ServerRegion(name: "Japan", servers: 7),
    ServerRegion(name: "Mexico", servers: 9),
    ServerRegion(name: "Netherlands", servers: 1),
  ]

  @Published var selectedRegion: ServerRegion? = nil

  public init() {
    selectedRegion = serverRegions.first
  }
}

public struct BraveVPNRegionPickerView: View {

  @State private var isAutomatic: Bool
  @State private var isLoading = false
  @State private var isRegionDetailsPresented = false
  @State private var selectedIndex = 0

  @ObservedObject private var serverRegionDetail = ServerRegionDetail()

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
              "A server region most proximate to you will be automatically selected, based on your system timezone. This is recommended in order to ensure fast internet speeds."
            )
            .font(.footnote)
            .foregroundColor(Color(braveSystemName: .textSecondary))
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
                if isAutomatic {
                  Text("Brazil")
                    .font(.footnote)
                    .foregroundColor(Color(braveSystemName: .textSecondary))
                }
              }
            }
            .disabled(isLoading)
            .foregroundColor(Color(braveSystemName: .textPrimary))
            .tint(.accentColor)
            .listRowBackground(Color(.white))
          }

          if !isAutomatic {
            Section {
              ForEach(Array(serverRegionDetail.serverRegions.enumerated()), id: \.offset) {
                index,
                region in
                Button {
                  print("Index Outside \(index)")
                  selectDesignatedVPNRegion(at: index)
                } label: {
                  HStack {
                    regionFlag ?? Image(braveSystemName: "leo.globe")
                    VStack(alignment: .leading) {
                      Text("\(region.name)")
                      Text("\(region.servers) servers")
                        .foregroundColor(.gray)
                    }
                    Spacer()
                    if region == serverRegionDetail.selectedRegion {
                      Text("Connected")
                        .foregroundColor(.green)
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
            .listRowBackground(Color(.white))
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
      }
    )
    .buttonStyle(.plain)
  }

  private func enableAutomaticServer(_ enabled: Bool) {
    isAutomatic = enabled
  }

  private func selectDesignatedVPNRegion(at index: Int) {
    guard !isLoading else {
      return
    }

    isLoading = true

    // TODO: Select Region
    DispatchQueue.main.asyncAfter(deadline: .now() + 3) {
      if let selectedRegion = serverRegionDetail.serverRegions[safe: index] {
        serverRegionDetail.selectedRegion = selectedRegion
      }
      
      isLoading = false
    }
  }

  private var regionFlag: Image? {
    // Root Unicode flags index
    let rootIndex: UInt32 = 127397
    var unicodeScalarView = ""

    for scalar in "CA".unicodeScalars {
      // Shift the letter index to the flags index
      if let appendedScalar = UnicodeScalar(rootIndex + scalar.value) {
        // Append symbol to the Unicode string
        unicodeScalarView.unicodeScalars.append(appendedScalar)
      }
    }

    if unicodeScalarView.isEmpty {
      return nil
    }

    return Image(uiImage: unicodeScalarView.image())
  }
}

struct ServerRegionView_Previews: PreviewProvider {
  static var previews: some View {
    BraveVPNRegionPickerView(isAutomatic: false)
  }
}
