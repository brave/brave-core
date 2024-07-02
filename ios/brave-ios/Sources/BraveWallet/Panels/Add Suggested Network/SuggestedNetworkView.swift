// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
import DesignSystem
import Strings
import SwiftUI

struct SuggestedNetworkView: View {
  enum Mode: Equatable {
    case switchNetworks(BraveWallet.SwitchChainRequest)
    case addNetwork(BraveWallet.AddChainRequest)
  }

  var mode: Mode
  var originInfo: BraveWallet.OriginInfo {
    switch mode {
    case .addNetwork(let request):
      return request.originInfo
    case .switchNetworks(let request):
      return request.originInfo
    }
  }
  var cryptoStore: CryptoStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var networkStore: NetworkStore

  @State private var isPresentingNetworkDetails: NetworkModel?
  @State private var customNetworkError: CustomNetworkError?
  @State private var isLoading: Bool = false

  @ScaledMetric private var blockieSize = 24
  private let maxBlockieSize: CGFloat = 72
  @ScaledMetric private var faviconSize = 48
  private let maxFaviconSize: CGFloat = 96

  @Environment(\.sizeCategory) private var sizeCategory
  @Environment(\.openURL) private var openWalletURL

  var onDismiss: () -> Void

  init(
    mode: Mode,
    cryptoStore: CryptoStore,
    keyringStore: KeyringStore,
    networkStore: NetworkStore,
    onDismiss: @escaping () -> Void
  ) {
    self.mode = mode
    self.cryptoStore = cryptoStore
    self.keyringStore = keyringStore
    self.networkStore = networkStore
    self.onDismiss = onDismiss
  }

  private var chain: BraveWallet.NetworkInfo? {
    switch mode {
    case .addNetwork(let request):
      return request.networkInfo
    case .switchNetworks(let request):
      return networkStore.allChains.first(where: { $0.chainId == request.chainId })
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
        if let url = URL(string: originInfo.originSpec) {
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
      Text(originInfo: originInfo)
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
        Button {
          UIPasteboard.general.string = keyringStore.selectedAccount.address
        } label: {
          Label(Strings.Wallet.copyAddressButtonTitle, braveSystemImage: "leo.copy.plain-text")
            .font(.body)
        }
      } label: {
        HStack(spacing: 8) {
          Spacer()
          if !keyringStore.selectedAccount.address.isEmpty {
            Text(keyringStore.selectedAccount.address.truncatedAddress)
              .fontWeight(.semibold)
          }
          Blockie(address: keyringStore.selectedAccount.blockieSeed)
            .frame(
              width: min(blockieSize, maxBlockieSize),
              height: min(blockieSize, maxBlockieSize)
            )
            .aspectRatio(1, contentMode: .fit)
        }
      }
      .accessibilityLabel(Strings.Wallet.selectedAccountAccessibilityLabel)
      .accessibilityValue(
        "\(keyringStore.selectedAccount.name), \(keyringStore.selectedAccount.address.truncatedAddress)"
      )
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
            openWalletURL(.brave.support)
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
        Group {
          if let chain = chain {
            VStack(alignment: .leading) {
              Text(Strings.Wallet.networkNameTitle)
                .fontWeight(.semibold)
              Text(chain.chainName)
            }
            .padding(.vertical, 6)
            .accessibilityElement(children: .combine)
            if let networkURL = chain.rpcEndpoints.first {
              VStack(alignment: .leading) {
                Text(Strings.Wallet.networkURLTitle)
                  .fontWeight(.semibold)
                Text(networkURL.absoluteDisplayString)
              }
              .padding(.vertical, 6)
            }
            Button {
              isPresentingNetworkDetails = .init(
                mode: .view(chain)
              )
            } label: {
              Text(Strings.Wallet.viewDetails)
                .foregroundColor(Color(.braveBlurpleTint))
            }
          }
        }
        .listRowBackground(Color(.secondaryBraveGroupedBackground))
      } header: {
        headerView
      }
      .font(.footnote)

      Section {
        actionButtonContainer
          .frame(maxWidth: .infinity)
          .listRowBackground(Color(.braveGroupedBackground))
      }
      .listRowInsets(.zero)
      .opacity(sizeCategory.isAccessibilityCategory ? 0 : 1)
      .accessibility(hidden: sizeCategory.isAccessibilityCategory)
    }
    .listStyle(InsetGroupedListStyle())
    .listBackgroundColor(Color(UIColor.braveGroupedBackground))
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
            NetworkDetailsView(
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
              dismissButton: .default(Text(Strings.OKString), action: onDismiss)
            )
          }
        )
    )
    .onAppear {
      // this can occur when Add Network is dismissed while still loading...
      // we need to show loading state again, and handle success/failure response
      if case .addNetwork(let request) = mode,
        cryptoStore.addNetworkDappRequestCompletion[request.networkInfo.chainId] != nil
      {
        self.isLoading = true
        // overwrite the completion closure with a new one for this new view instance
        cryptoStore.addNetworkDappRequestCompletion[request.networkInfo.chainId] =
          handleAddNetworkCompletion
      }
    }
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
    Button {  // cancel
      handleAction(approved: false)
    } label: {
      HStack {
        Image(systemName: "xmark")
        Text(Strings.cancelButtonTitle)
      }
    }
    .buttonStyle(BraveOutlineButtonStyle(size: .large))
    .disabled(isLoading)
    WalletLoadingButton(
      isLoading: isLoading,
      action: {  // approve
        handleAction(approved: true)
      },
      label: {
        HStack {
          Image(braveSystemName: "leo.check.circle-filled")
          Text(actionButtonTitle)
            .multilineTextAlignment(.center)
        }
      }
    )
    .buttonStyle(BraveFilledButtonStyle(size: .large))
    .disabled(isLoading)
  }

  private func handleAction(approved: Bool) {
    isLoading = true
    switch mode {
    case .addNetwork(let request):
      cryptoStore.handleWebpageRequestResponse(
        .addNetwork(approved: approved, chainId: request.networkInfo.chainId),
        completion: handleAddNetworkCompletion
      )
    case .switchNetworks(let request):
      cryptoStore.handleWebpageRequestResponse(
        .switchChain(approved: approved, requestId: request.requestId),
        completion: { _ in
          isLoading = false
          onDismiss()
        }
      )
    }
  }

  private func handleAddNetworkCompletion(_ error: String?) {
    isLoading = false
    if let error, !error.isEmpty {
      customNetworkError = .failed(errorMessage: error)
      return
    }
    onDismiss()
  }
}

#if DEBUG
struct SuggestedNetworkView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      SuggestedNetworkView(
        mode: .addNetwork(
          .init(
            originInfo: .init(originSpec: "https://app.uniswap.org", eTldPlusOne: "uniswap.org"),
            networkInfo: .mockSepolia
          )
        ),
        cryptoStore: .previewStore,
        keyringStore: .previewStoreWithWalletCreated,
        networkStore: .previewStore,
        onDismiss: {}
      )
      SuggestedNetworkView(
        mode: .switchNetworks(
          .init(
            requestId: UUID().uuidString,
            originInfo: .init(
              originSpec: "https://polygon.technology/",
              eTldPlusOne: "polygon.technology"
            ),
            chainId: BraveWallet.NetworkInfo.mockPolygon.chainId
          )
        ),
        cryptoStore: .previewStore,
        keyringStore: .previewStoreWithWalletCreated,
        networkStore: .previewStore,
        onDismiss: {}
      )
    }
  }
}
#endif
