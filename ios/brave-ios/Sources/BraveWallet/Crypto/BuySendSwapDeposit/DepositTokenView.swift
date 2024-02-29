// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import DesignSystem
import Strings
import SwiftUI

struct DepositTokenView: View {
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var depositTokenStore: DepositTokenStore

  var onDismiss: () -> Void

  @State private var isPresentingNetworkFilter = false
  @State private var selectedAccount: BraveWallet.AccountInfo?

  private var availableAccounts: [BraveWallet.AccountInfo] {
    guard let token = depositTokenStore.prefilledToken else { return [] }
    return depositTokenStore.allAccounts.filter({ $0.coin == token.coin })
  }

  var body: some View {
    NavigationView {
      Group {
        if let prefilledAccount = depositTokenStore.prefilledAccount {
          DepositDetailsView(
            type: .prefilledAccount(account: prefilledAccount),
            allNetworks: depositTokenStore.allNetworks
          )
        } else if let prefilledToken = depositTokenStore.prefilledToken, !availableAccounts.isEmpty
        {
          DepositDetailsView(
            type: .prefilledToken(token: prefilledToken, availableAccounts: availableAccounts),
            allNetworks: depositTokenStore.allNetworks
          )
        } else {
          List {
            ForEach(depositTokenStore.assetViewModels) { assetViewModel in
              NavigationLink(
                destination:
                  DepositDetailsView(
                    type: .prefilledToken(
                      token: assetViewModel.token,
                      availableAccounts: depositTokenStore.allAccounts.filter {
                        $0.coin == assetViewModel.token.coin
                      }
                    ),
                    allNetworks: depositTokenStore.allNetworks
                  )
              ) {
                HStack {
                  AssetIconView(
                    token: assetViewModel.token,
                    network: assetViewModel.network,
                    shouldShowNetworkIcon: true
                  )
                  VStack(alignment: .leading) {
                    Text(assetViewModel.token.name)
                      .font(.footnote)
                      .fontWeight(.semibold)
                      .foregroundColor(Color(.bravePrimary))
                    Text(
                      String.localizedStringWithFormat(
                        Strings.Wallet.userAssetSymbolNetworkDesc,
                        assetViewModel.token.symbol,
                        assetViewModel.network.chainName
                      )
                    )
                    .font(.caption)
                    .foregroundColor(Color(.braveLabel))
                  }
                }
              }
            }
          }
          .listBackgroundColor(Color(UIColor.braveGroupedBackground))
          .searchable(
            text: $depositTokenStore.query,
            placement: .navigationBarDrawer(displayMode: .always)
          )
          .toolbar {
            ToolbarItemGroup(placement: .bottomBar) {
              Button(action: {
                self.isPresentingNetworkFilter = true
              }) {
                Image(braveSystemName: "leo.tune")
                  .font(.footnote.weight(.medium))
                  .foregroundColor(Color(.braveBlurpleTint))
                  .clipShape(Rectangle())
              }
              Spacer()
            }
          }
        }
      }
      .navigationViewStyle(StackNavigationViewStyle())
      .navigationTitle(Strings.Wallet.deposit)
      .navigationBarTitleDisplayMode(.inline)
      .toolbar {
        ToolbarItemGroup(placement: .cancellationAction) {
          Button(action: {
            onDismiss()
          }) {
            Text(Strings.cancelButtonTitle)
              .foregroundColor(Color(.braveBlurpleTint))
          }
        }
      }
      .sheet(isPresented: $isPresentingNetworkFilter) {
        NavigationView {
          NetworkFilterView(
            networks: depositTokenStore.networkFilters,
            networkStore: networkStore,
            saveAction: { selectedNetworks in
              depositTokenStore.networkFilters = selectedNetworks
            }
          )
        }
        .navigationViewStyle(.stack)
        .onDisappear {
          networkStore.closeNetworkSelectionStore()
        }
      }
    }
    .task {
      depositTokenStore.setup()
    }
  }
}

struct DepositDetailsView: View {
  var type: DepositType
  var allNetworks: [BraveWallet.NetworkInfo]

  @State private var selectedAccount: BraveWallet.AccountInfo?
  @State private var isPresentingAccountPicker: Bool = false
  @State private var addressToShare: String?
  @ScaledMetric private var avatarSize = 24.0

  enum DepositType {
    case prefilledAccount(account: BraveWallet.AccountInfo)
    case prefilledToken(
      token: BraveWallet.BlockchainToken,
      availableAccounts: [BraveWallet.AccountInfo]
    )
  }

  init(
    type: DepositType,
    allNetworks: [BraveWallet.NetworkInfo]
  ) {
    self.type = type
    self.allNetworks = allNetworks
    if case let .prefilledToken(_, availableAccounts) = type {
      self._selectedAccount = State(initialValue: availableAccounts.first)
    }
  }

  private var accountPicker: some View {
    HStack(spacing: 16) {
      Blockie(address: selectedAccount?.address ?? "")
        .frame(width: avatarSize, height: avatarSize)
      Text(selectedAccount?.name ?? "")
        .fontWeight(.semibold)
        .foregroundColor(Color(.bravePrimary))
        .multilineTextAlignment(.leading)
        .font(.body)
      Image(braveSystemName: "leo.arrow.small-down")
        .font(.callout.weight(.medium))
        .foregroundColor(Color(.primaryButtonTint))
        .padding(.leading, 48)
    }
    .padding(.horizontal, 12)
    .padding(.vertical, 8)
    .background {
      RoundedRectangle(cornerRadius: 8)
        .fill(Color(.secondaryBraveBackground))
    }
  }

  @ViewBuilder func depositHeader(
    _ coin: BraveWallet.CoinType,
    networks: [BraveWallet.NetworkInfo]
  ) -> some View {
    VStack(spacing: 8) {
      switch coin {
      case .eth:
        Text(Strings.Wallet.ethAccountDescription)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        MultipleNetworkIconsView(networks: networks.filter({ $0.coin == .eth }), maxIcons: 8)
      case .sol:
        Text(Strings.Wallet.solAccountDescription)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        MultipleNetworkIconsView(networks: networks.filter({ $0.coin == .sol }))
      case .fil:
        Text(Strings.Wallet.filAccountDescription)
          .fontWeight(.semibold)
          .foregroundColor(Color(.bravePrimary))
        MultipleNetworkIconsView(networks: networks.filter({ $0.coin == .fil }))
      case .zec, .btc:
        EmptyView()
      @unknown default:
        EmptyView()
      }
    }
  }

  @ViewBuilder func qrCodeView(_ account: BraveWallet.AccountInfo) -> some View {
    VStack(spacing: 12) {
      RoundedRectangle(cornerRadius: 20, style: .continuous)
        .stroke(Color(.secondaryButtonTint).opacity(0.5), lineWidth: 1)
        .frame(width: 184, height: 184)
        .overlay(
          Group {
            if let image = account.qrCodeImage?.cgImage {
              Image(uiImage: UIImage(cgImage: image))
                .resizable()
                .interpolation(.none)
                .scaledToFit()
                .padding()
                .accessibilityHidden(true)
            }
          }
        )
        .padding(.bottom, 12)
      Text(account.name)
        .font(.callout.weight(.semibold))
        .multilineTextAlignment(.center)
      Text(account.address)
        .font(.subheadline)
        .foregroundColor(Color(.secondaryBraveLabel))
        .multilineTextAlignment(.center)
      HStack {
        Button(action: { UIPasteboard.general.string = account.address }) {
          HStack {
            Image(braveSystemName: "leo.copy")
            Text(Strings.Wallet.depositAddressCopy)
              .font(.footnote.weight(.semibold))
          }
          .foregroundColor(Color(.braveBlurpleTint))
        }
        .buttonStyle(BraveOutlineButtonStyle(size: .normal))
        Button(action: {
          addressToShare = account.address
        }) {
          HStack {
            Image(braveSystemName: "leo.share.macos")
            Text(Strings.share)
              .font(.footnote.weight(.semibold))
          }
          .foregroundColor(Color(.braveBlurpleTint))
        }
        .buttonStyle(BraveOutlineButtonStyle(size: .normal))
      }
      .padding(.top, 12)
    }
  }

  var ethNetworksCombined: String {
    let networks =
      allNetworks
      .filter {
        $0.coin == .eth && $0.chainId.lowercased() != BraveWallet.MainnetChainId.lowercased()
      }
      .map { $0.chainName }
    return networks.joined(separator: ",")
  }

  @ViewBuilder var ethDisclosureView: some View {
    Text(String.localizedStringWithFormat(Strings.Wallet.depositEthDisclosure, ethNetworksCombined))
      .font(.caption2)
      .foregroundColor(Color(.secondaryBraveLabel))
      .multilineTextAlignment(.center)
  }

  var body: some View {
    ScrollView {
      VStack(spacing: 24) {
        switch type {
        case let .prefilledAccount(account):
          depositHeader(account.coin, networks: allNetworks)
          qrCodeView(account)
          if account.coin == .eth {
            ethDisclosureView
          }
        case let .prefilledToken(token, _):
          Button(action: {
            isPresentingAccountPicker = true
          }) {
            accountPicker
          }
          depositHeader(token.coin, networks: allNetworks)
          if let selectedAccount {
            qrCodeView(selectedAccount)
          }
          if token.coin == .eth {
            ethDisclosureView
          }
        }
      }
      .padding(.vertical, 32)
      .padding(.horizontal, 16)
      .sheet(isPresented: $isPresentingAccountPicker) {
        if let selectedAccount, case let .prefilledToken(_, availableAccounts) = type {
          NavigationView {
            AccountSelectionRootView(
              navigationTitle: Strings.Wallet.selectAccountTitle,
              allAccounts: availableAccounts,
              selectedAccounts: [selectedAccount],
              showsSelectAllButton: false
            ) { selectedAccount in
              self.selectedAccount = selectedAccount
              self.isPresentingAccountPicker = false
            }
          }
          .navigationViewStyle(.stack)
        }
      }
      .sheet(
        isPresented: Binding(
          get: { addressToShare != nil },
          set: { if !$0 { addressToShare = nil } }
        )
      ) {
        if let addressToShare {
          ShareSheetView(activityItems: [addressToShare])
        }
      }
    }
  }
}

struct ShareSheetView: UIViewControllerRepresentable {
  typealias Callback = (
    _ activityType: UIActivity.ActivityType?, _ completed: Bool, _ returnedItems: [Any]?,
    _ error: Error?
  ) -> Void

  let activityItems: [Any]
  let applicationActivities: [UIActivity]? = nil
  let excludedActivityTypes: [UIActivity.ActivityType]? = nil
  let callback: Callback? = nil

  func makeUIViewController(context: Context) -> UIActivityViewController {
    let controller = UIActivityViewController(
      activityItems: activityItems,
      applicationActivities: applicationActivities
    )
    controller.excludedActivityTypes = excludedActivityTypes
    controller.completionWithItemsHandler = callback
    return controller
  }

  func updateUIViewController(_ uiViewController: UIActivityViewController, context: Context) {
    // nothing to do here
  }
}

#if DEBUG
#Preview {
  DepositTokenView(
    networkStore: .previewStore,
    depositTokenStore: .previewStore,
    onDismiss: {}
  )
}
#endif
