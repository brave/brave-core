/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import SwiftUI
import BraveCore
import BraveShared
import Strings

extension BraveWallet.NetworkInfo {
  var shortChainName: String {
    chainName.split(separator: " ").first?.capitalized ?? chainName
  }
}

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
  var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  @State private var isPresentingAddNetwork: Bool = false
  /// If we are prompting the user to add an account for the `nextNetwork.coin` type
  @State private var isShowingNextNetworkAlert = false
  /// The network the user wishes to switch to, but does not (yet) have an account for `nextNetwork.coin` type
  @State private var nextNetwork: BraveWallet.NetworkInfo?
  /// If we are prompting the user to create a new account for the `nextNetwork.coin` type
  @State private var isPresentingAddAccount: Bool = false
  @Environment(\.presentationMode) @Binding private var presentationMode
  @Environment(\.buySendSwapDestination) @Binding private var buySendSwapDestination
  
  init(
    style: Style = .`default`,
    keyringStore: KeyringStore,
    networkStore: NetworkStore
  ) {
    self.style = style
    self.keyringStore = keyringStore
    self.networkStore = networkStore
  }
  
  private var availableChains: [BraveWallet.NetworkInfo] {
    networkStore.allChains.filter { chain in
      if !Preferences.Wallet.showTestNetworks.value {
        var testNetworkChainIdsToRemove = WalletConstants.supportedTestNetworkChainIds
        // Don't remove selected network (possible if selected then disabled showing test networks)
        testNetworkChainIdsToRemove.removeAll(where: { $0 == networkStore.selectedChain.chainId })
        if testNetworkChainIdsToRemove.contains(chain.chainId) {
          return false
        }
      }
      if let destination = buySendSwapDestination {
        if destination.kind != .send {
          return !chain.isCustom
        }
      }
      return true
    }
  }
  
  var body: some View {
    Menu {
      Picker(
        Strings.Wallet.selectedNetworkAccessibilityLabel,
        selection: Binding(
          get: { networkStore.selectedChain },
          set: { network in
            Task { @MainActor in
              let error = await networkStore.setSelectedChain(network)
              switch error {
              case .selectedChainHasNoAccounts:
                self.isShowingNextNetworkAlert = true
                self.nextNetwork = network
              default:
                break
              }
            }
          }
        )
      ) {
        ForEach(availableChains) {
          Text($0.chainName).tag($0)
        }
      }
      Divider()
      Button(action: { isPresentingAddNetwork = true }) {
        Label(Strings.Wallet.addCustomNetworkDropdownButtonTitle, systemImage: "plus")
      }
    } label: {
      HStack {
        Text(networkStore.selectedChain.shortChainName)
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
      .animation(nil, value: networkStore.selectedChain)
    }
    .accessibilityLabel(Strings.Wallet.selectedNetworkAccessibilityLabel)
    .accessibilityValue(networkStore.selectedChain.shortChainName)
    .sheet(isPresented: $isPresentingAddNetwork) {
      NavigationView {
        CustomNetworkDetailsView(networkStore: networkStore, model: .init())
      }
    }
    .alert(
      isPresented: $isShowingNextNetworkAlert
    ) {
      Alert(
        title: Text(String.localizedStringWithFormat(Strings.Wallet.createAccountAlertTitle, nextNetwork?.shortChainName ?? "")),
        message: Text(Strings.Wallet.createAccountAlertMessage),
        primaryButton: .default(Text(Strings.yes), action: {
          // show create account for `nextNetwork.coin`
          isPresentingAddAccount = true
        }),
        secondaryButton: .cancel(Text(Strings.no), action: {
          // not creating account, don't switch to nextNetwork
          self.nextNetwork = nil
        })
      )
    }
    .sheet(
      isPresented: $isPresentingAddAccount
    ) {
      NavigationView {
        AddAccountView(keyringStore: keyringStore, preSelectedCoin: nextNetwork?.coin)
      }
      .onDisappear {
        // User either created an account, or cancelled. If account created,
        // `NetworkStore`s `keyringCreated` observation will switch to the new account
        self.nextNetwork = nil
      }
    }
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
