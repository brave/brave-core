// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import BraveUI
import Strings
import BraveShared
import BraveCore
import DesignSystem

struct SuggestedNetworkView: View {
  enum Mode: Equatable {
    case switchNetworks(chainId: String)
    case addNetwork(BraveWallet.NetworkInfo)
  }
  
  var mode: Mode
  let originInfo: BraveWallet.OriginInfo
  var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore
  
  @State private var isPresentingNetworkDetails: CustomNetworkModel?
  @State private var customNetworkError: CustomNetworkError?
  
  @ScaledMetric private var blockieSize = 24
  private let maxBlockieSize: CGFloat = 72
  @ScaledMetric private var faviconSize = 48
  private let maxFaviconSize: CGFloat = 96

  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.openWalletURLAction) private var openWalletURL
  
  var onDismiss: () -> Void
  
  init(
    mode: Mode,
    originInfo: BraveWallet.OriginInfo,
    cryptoStore: CryptoStore,
    keyringStore: KeyringStore,
    networkStore: NetworkStore,
    onDismiss: @escaping () -> Void
  ) {
    self.mode = mode
    self.originInfo = originInfo
    self.cryptoStore = cryptoStore
    self.keyringStore = keyringStore
    self.networkStore = networkStore
    self.onDismiss = onDismiss
  }
  
  private var chain: BraveWallet.NetworkInfo? {
    switch mode {
    case let .addNetwork(chain):
      return chain
    case let .switchNetworks(chainId):
      return networkStore.allChains.first(where: { $0.chainId == chainId })
    }
  }
  
  private var navigationTitle: String {
    switch mode {
    case .switchNetworks: return Strings.Wallet.switchNetworkTitle
    case .addNetwork: return Strings.Wallet.addNetworkTitle
    }
  }
  
  private var headerTitle: String {
    switch mode {
    case .switchNetworks: return Strings.Wallet.switchNetworkSubtitle
    case .addNetwork: return Strings.Wallet.addNetworkSubtitle
    }
  }
  
  private var headerDescription: String {
    switch mode {
    case .switchNetworks: return Strings.Wallet.switchNetworkDescription
    case .addNetwork: return Strings.Wallet.addNetworkDescription
    }
  }
  
  private var globeFavicon: some View {
    Image(systemName: "globe")
      .resizable()
      .aspectRatio(contentMode: .fit)
      .padding(8)
      .background(Color(.braveDisabled))
  }
  
  @ViewBuilder private var faviconAndOrigin: some View {
    VStack(spacing: 8) {
      Group {
        if let url = originInfo.origin.url {
          FaviconReader(url: url) { image in
            if let image = image {
              Image(uiImage: image)
                .resizable()
            } else {
              globeFavicon
            }
          }
        } else {
          globeFavicon
        }
      }
      .frame(width: min(faviconSize, maxFaviconSize), height: min(faviconSize, maxFaviconSize))
      .clipShape(RoundedRectangle(cornerRadius: 4, style: .continuous))
      Text(urlOrigin: originInfo.origin)
        .font(.subheadline)
        .foregroundColor(Color(.braveLabel))
        .multilineTextAlignment(.center)
    }
    .accessibilityElement(children: .combine)
  }
  
  private var headerView: some View {
    VStack {
      Menu {
        Text(keyringStore.selectedAccount.address.zwspOutput)
        Button(action: {
          UIPasteboard.general.string = keyringStore.selectedAccount.address
        }) {
          Label(Strings.Wallet.copyAddressButtonTitle, braveSystemImage: "brave.clipboard")
            .font(.body)
        }
      } label: {
        HStack(spacing: 8) {
          Spacer()
          Text(keyringStore.selectedAccount.address.truncatedAddress)
            .fontWeight(.semibold)
          Blockie(address: keyringStore.selectedAccount.address)
            .frame(width: min(blockieSize, maxBlockieSize), height: min(blockieSize, maxBlockieSize))
            .aspectRatio(1, contentMode: .fit)
        }
      }
      .accessibilityLabel(Strings.Wallet.selectedAccountAccessibilityLabel)
      .accessibilityValue("\(keyringStore.selectedAccount.name), \(keyringStore.selectedAccount.address.truncatedAddress)")
      VStack(spacing: 8) {
        faviconAndOrigin
        Text(headerTitle)
          .font(.headline)
          .foregroundColor(Color(.bravePrimary))
          .multilineTextAlignment(.center)
        Text(headerDescription)
          .font(.subheadline)
          .foregroundColor(Color(.braveLabel))
          .multilineTextAlignment(.center)
        if case .addNetwork = mode {
          Button {
            openWalletURL?(BraveUX.braveWalletNetworkLearnMoreURL)
          } label: {
            Text(Strings.Wallet.learnMoreButton)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
      }
      .frame(maxWidth: .infinity)
      .padding(.vertical)
    }
    .resetListHeaderStyle()
    .padding(.vertical)
  }
  
  var body: some View {
    List {
      Section {
        if let chain = chain {
          VStack(alignment: .leading) {
            Text(Strings.Wallet.networkNameTitle)
              .fontWeight(.semibold)
            Text(chain.chainName)
          }
          .padding(.vertical, 6)
          .accessibilityElement(children: .combine)
          if let networkURL = chain.rpcUrls.first {
            VStack(alignment: .leading) {
              Text(Strings.Wallet.networkURLTitle)
                .fontWeight(.semibold)
              Text(URL(string: networkURL)?.absoluteDisplayString ?? networkURL)
            }
            .padding(.vertical, 6)
          }
          Button {
            isPresentingNetworkDetails = .init(from: chain, mode: .view)
          } label: {
            Text(Strings.Wallet.viewDetails)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
      } header: {
        headerView
      }
      .listRowBackground(Color(.secondaryBraveGroupedBackground))
      .font(.footnote)
      
      Section {
        actionButtonContainer
          .frame(maxWidth: .infinity)
      }
      .listRowBackground(Color(.braveGroupedBackground))
      .listRowInsets(.zero)
      .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
      .accessibility(hidden: sizeCategory.isAccessibilityCategory)
    }
    .listStyle(InsetGroupedListStyle())
    .navigationTitle(navigationTitle)
    .navigationBarTitleDisplayMode(.inline)
    .overlay(
      Group {
        if sizeCategory.isAccessibilityCategory {
          actionButtonContainer
            .frame(maxWidth: .infinity)
            .padding(.top)
            .background(
              LinearGradient(
                stops: [
                  .init(color: Color(.braveGroupedBackground).opacity(0), location: 0),
                  .init(color: Color(.braveGroupedBackground).opacity(1), location: 0.05),
                  .init(color: Color(.braveGroupedBackground).opacity(1), location: 1),
                ],
                startPoint: .top,
                endPoint: .bottom
              )
                .ignoresSafeArea()
                .allowsHitTesting(false)
            )
        }
      },
      alignment: .bottom
    )
    .background(
      Color.clear
        .sheet(item: $isPresentingNetworkDetails) { detailsModel in
          NavigationView {
            CustomNetworkDetailsView(
              networkStore: networkStore,
              model: detailsModel
            )
          }
          .navigationViewStyle(StackNavigationViewStyle())
        }
    )
    .background(
      Color.clear
        .alert(
          item: $customNetworkError,
          content: { error in
            Alert(
              title: Text(error.errorTitle),
              message: Text(error.errorDescription),
              dismissButton: .default(Text(Strings.OKString))
            )
          })
    )
  }
  
  private var actionButtonTitle: String {
    switch mode {
    case .switchNetworks: return Strings.Wallet.switchNetworkButtonTitle
    case .addNetwork: return Strings.Wallet.approveNetworkButtonTitle
    }
  }
  
  @ViewBuilder private var actionButtonContainer: some View {
    if sizeCategory.isAccessibilityCategory {
      VStack {
        actionButtons
      }
    } else {
      HStack {
        actionButtons
      }
    }
  }

  @ViewBuilder private var actionButtons: some View {
    Button(action: { // cancel
      handleAction(approved: false)
    }) {
      HStack {
        Image(systemName: "xmark")
        Text(Strings.cancelButtonTitle)
      }
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    Button(action: { // approve
      handleAction(approved: true)
    }) {
      HStack {
        Image(braveSystemName: "brave.checkmark.circle.fill")
        Text(actionButtonTitle)
          .multilineTextAlignment(.center)
      }
    }
    .buttonStyle(BraveFilledButtonStyle(size: .large))
  }
  
  private func handleAction(approved: Bool) {
    switch mode {
    case let .addNetwork(networkInfo):
      cryptoStore.handleWebpageRequestResponse(
        .addNetwork(approved: approved, chainId: networkInfo.chainId)
      )
    case .switchNetworks:
      cryptoStore.handleWebpageRequestResponse(
        .switchChain(approved: approved, originInfo: originInfo)
      )
    }
    onDismiss()
  }
}

#if DEBUG
struct SuggestedNetworkView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      SuggestedNetworkView(
        mode: .addNetwork(.mockRopsten),
        originInfo: .init(origin: .init(url: URL(string: "https://app.uniswap.org")!), originSpec: "", eTldPlusOne: "uniswap.org"),
        cryptoStore: .previewStore,
        keyringStore: .previewStoreWithWalletCreated,
        networkStore: .previewStore,
        onDismiss: { }
      )
      SuggestedNetworkView(
        mode: .switchNetworks(chainId: BraveWallet.RopstenChainId),
        originInfo: .init(origin: .init(url: URL(string: "https://app.uniswap.org")!), originSpec: "", eTldPlusOne: "uniswap.org"),
        cryptoStore: .previewStore,
        keyringStore: .previewStoreWithWalletCreated,
        networkStore: .previewStore,
        onDismiss: { }
      )
    }
  }
}
#endif
