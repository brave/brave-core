// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import Strings
import SwiftUI

struct NetworkPicker: View {

  struct Style: Equatable {
    let textColor: UIColor
    let borderColor: UIColor

    static let `default` = Style(
      textColor: .bravePrimary,
      borderColor: .secondaryButtonTint
    )
  }

  let style: Style
  let isForOrigin: Bool
  var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @State private var isPresentingAddNetwork: Bool = false
  @State private var networkSelectionStore: NetworkSelectionStore?
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.walletActionDestination) @Binding private var walletActionDestination

  init(
    style: Style = .`default`,
    isForOrigin: Bool = false,
    keyringStore: KeyringStore,
    networkStore: NetworkStore
  ) {
    self.style = style
    self.isForOrigin = isForOrigin
    self.keyringStore = keyringStore
    self.networkStore = networkStore
  }

  private var chainName: String {
    if isForOrigin {
      return networkStore.selectedChainForOrigin.chainName
    }
    return networkStore.defaultSelectedChain.chainName
  }

  var body: some View {
    Button {
      networkSelectionStore = .init(
        mode: .select(isForOrigin: isForOrigin),
        networkStore: networkStore
      )
    } label: {
      HStack {
        Text(chainName)
          .fontWeight(.bold)
        Image(systemName: "chevron.down.circle")
      }
      .foregroundColor(Color(style.textColor))
      .font(.caption.weight(.semibold))
      .padding(.init(top: 6, leading: 12, bottom: 6, trailing: 12))
      .background(
        Color(style.borderColor)
          .clipShape(Capsule().inset(by: 0.5).stroke())
      )
      .clipShape(Capsule())
      .contentShape(Capsule())
    }
    .animation(nil, value: networkStore.defaultSelectedChain)
    .animation(nil, value: networkStore.selectedChainForOrigin)
    .background(
      Color.clear
        .sheet(isPresented: $isPresentingAddNetwork) {
          NavigationView {
            NetworkDetailsView(networkStore: networkStore, model: .init())
          }
        }
    )
    .background(
      Color.clear
        .sheet(
          isPresented: Binding(
            get: { self.networkSelectionStore != nil },
            set: { if !$0 { self.networkSelectionStore = nil } }
          )
        ) {
          if let networkSelectionStore = networkSelectionStore {
            NavigationView {
              NetworkSelectionView(
                keyringStore: keyringStore,
                networkStore: networkStore,
                networkSelectionStore: networkSelectionStore
              )
            }
            .accentColor(Color(.braveBlurpleTint))
            .navigationViewStyle(.stack)
          }
        }
    )
  }
}

#if DEBUG
struct NetworkPicker_Previews: PreviewProvider {
  static var previews: some View {
    NetworkPicker(
      keyringStore: .previewStoreWithWalletCreated,
      networkStore: .previewStore
    )
    .padding()
    .previewLayout(.sizeThatFits)
    .previewColorSchemes()
  }
}
#endif
