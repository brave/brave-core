// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import SwiftUI

public struct BraveVPNRegionPickerView: View {
  @State public var isAutomatic: Bool

  public var serverRegions: [ServerRegion] = [
    ServerRegion(name: "Australia", servers: 5),
    ServerRegion(name: "Brazil", servers: 4, isConnected: true),
    ServerRegion(name: "Canada", servers: 2),
    ServerRegion(name: "France", servers: 3),
    ServerRegion(name: "Germany", servers: 5),
    ServerRegion(name: "Italy", servers: 3),
    ServerRegion(name: "Japan", servers: 7),
    ServerRegion(name: "Mexico", servers: 9),
    ServerRegion(name: "Netherlands", servers: 1),
  ]

  var selectedCountry: String {
    serverRegions.first(where: { $0.isConnected })?.name ?? "Unknown"
  }

  public init(isAutomatic: Bool, serverRegions: [ServerRegion]? = nil) {
    self.isAutomatic = isAutomatic

    if let serverRegions = serverRegions {
      self.serverRegions = serverRegions
    }
  }

  public var body: some View {
    List {
      Section(
        footer: Text(
          "A server region most proximate to you will be automatically selected, based on your system timezone. This is recommended in order to ensure fast internet speeds."
        )
        .foregroundColor(Color(.black))
      ) {
        Toggle(
          "Automatic",
          isOn: Binding(
            get: { isAutomatic },
            set: { toggledBiometricsUnlock($0) }
          )
        )
        .foregroundColor(Color(.black))
        .toggleStyle(SwitchToggleStyle(tint: Color(.blue)))
        .listRowBackground(Color(.white))
      }

      if !isAutomatic {
        Section {
          ForEach(serverRegions) { region in
            HStack {
              Image(systemName: "flag.2.crossed")

              VStack(alignment: .leading) {
                Text("\(region.name)")

                Text("\(region.servers) servers")
                  .foregroundColor(.gray)
              }
              Spacer()

              if region.isConnected {
                Text("Connected")
                  .foregroundColor(.green)
              }

              Button(
                action: {
                },
                label: {
                  Image(systemName: "info.circle")
                }
              )
            }
          }
        }
        .listRowBackground(Color(.white))
      }
    }
  }

  private func toggledBiometricsUnlock(_ enabled: Bool) {
    isAutomatic = enabled
  }
}

public struct ServerRegion: Identifiable {
  public let id = UUID()
  public let name: String
  public let servers: Int
  public var isConnected: Bool = false
}

struct ServerRegionView_Previews: PreviewProvider {
  static var previews: some View {
    BraveVPNRegionPickerView(isAutomatic: false)
  }
}
