// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import DesignSystem
import Foundation
import Preferences
import Strings
import SwiftUI

public protocol WalletSiteConnectionDelegate {
  /// A list of accounts connected to this webpage (addresses)
  var connectedAccounts: [String] { get }
  /// Update the connection status for a given account
  func updateConnectionStatusForAccountAddress(_ address: String)
}

public struct WalletPanelContainerView: View {
  var walletStore: WalletStore
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var tabDappStore: TabDappStore
  var origin: URLOrigin
  var presentWalletWithContext: ((PresentingContext) -> Void)?
  var presentBuySendSwap: (() -> Void)?
  var openWalletURLAction: ((URL) -> Void)?
  /// An invisible `UIView` background lives in SwiftUI for UIKit API to reference later
  var buySendSwapBackground: InvisibleUIView = .init()

  private enum VisibleScreen: Equatable {
    case loading
    case panel
    case onboarding
    case unlock
  }

  private var visibleScreen: VisibleScreen {
    // check if we are still fetching async info from core
    if !keyringStore.isLoaded {
      return .loading
    }
    // keyring fetched, check if user has setup a wallet
    if !keyringStore.isWalletCreated || keyringStore.isOnboardingVisible {
      return .onboarding
    }
    // keyring fetched & wallet setup, but selected account not fetched
    if keyringStore.selectedAccount.accountId.uniqueKey.isEmpty {
      return .loading
    }
    // keyring fetched & wallet setup, wallet is locked
    if keyringStore.isWalletLocked || keyringStore.isRestoreFromUnlockBiometricsPromptVisible {
      // wallet is locked
      return .unlock
    }
    return .panel
  }

  private var lockedView: some View {
    VStack(spacing: 36) {
      Image("graphic-lock", bundle: .module)
        .resizable()
        .aspectRatio(contentMode: .fit)
        .frame(maxWidth: 150)
      Button {
        presentWalletWithContext?(.panelUnlockOrSetup)
      } label: {
        HStack(spacing: 4) {
          Image(braveSystemName: "leo.lock.open")
          Text(Strings.Wallet.unlockWallet)
        }
      }
      .buttonStyle(BraveFilledButtonStyle(size: .normal))
    }
    .padding()
    .padding()
    .frame(maxWidth: .infinity)
    .background(Color(.braveBackground).ignoresSafeArea())
  }

  private var setupView: some View {
    ScrollView(.vertical) {
      VStack(spacing: 36) {
        VStack(spacing: 4) {
          Text(Strings.Wallet.braveWallet)
            .foregroundColor(Color(.bravePrimary))
            .font(.headline)
          Text(Strings.Wallet.walletPanelSetupWalletDescription)
            .foregroundColor(Color(.secondaryBraveLabel))
            .font(.subheadline)
        }
        .multilineTextAlignment(.center)
        Button {
          presentWalletWithContext?(.panelUnlockOrSetup)
        } label: {
          Text(Strings.Wallet.learnMoreButton)
        }
        .buttonStyle(BraveFilledButtonStyle(size: .normal))
      }
      .padding()
      .padding()
    }
    .frame(maxWidth: .infinity)
    .background(Color(.braveBackground).ignoresSafeArea())
  }

  public var body: some View {
    ZStack {
      switch visibleScreen {
      case .loading:
        lockedView
          .hidden()  // used for sizing to prevent #5378
          .accessibilityHidden(true)
        Color.white
          .overlay(ProgressView())
      case .panel:
        if let cryptoStore = walletStore.cryptoStore {
          WalletPanelView(
            keyringStore: keyringStore,
            cryptoStore: cryptoStore,
            networkStore: cryptoStore.networkStore,
            accountActivityStore: cryptoStore.accountActivityStore(
              for: keyringStore.selectedAccount,
              isWalletPanel: true
            ),
            tabDappStore: tabDappStore,
            origin: origin,
            presentWalletWithContext: { context in
              self.presentWalletWithContext?(context)
            },
            presentBuySendSwap: {
              self.presentBuySendSwap?()
            },
            buySendSwapBackground: buySendSwapBackground
          )
          .transition(.asymmetric(insertion: .identity, removal: .opacity))
        }
      case .unlock:
        lockedView
          .transition(.move(edge: .bottom).combined(with: .opacity))
          .zIndex(1)
      case .onboarding:
        setupView
          .transition(.move(edge: .bottom).combined(with: .opacity))
          .zIndex(2)  // Needed or the dismiss animation messes up
      }
    }
    .frame(idealWidth: 320, maxWidth: .infinity)
    .onChange(of: keyringStore.isWalletLocked) { newValue in
      guard keyringStore.isLoaded, newValue, !keyringStore.lockedManually else { return }
      // Wallet was auto-locked with panel open
      presentWalletWithContext?(.panelUnlockOrSetup)
    }
    .onChange(of: keyringStore.isLoaded) { newValue in
      guard newValue else { return }  // KeyringStore loaded
      handleKeyringStoreLoaded()
    }
    .onAppear {
      if keyringStore.isLoaded {
        // If KeyringStore is loaded prior to view appearing on
        // screen onChange won't be executed
        handleKeyringStoreLoaded()
      }
    }
    .environment(
      \.openURL,
      .init(handler: { [openWalletURLAction] url in
        openWalletURLAction?(url)
        return .handled
      })
    )
  }

  /// Flag to help prevent race condition between panel appearing on screen and KeyringStore `isLoaded`.
  @State private var didHandleKeyringLoaded: Bool = false
  /// Present unlock if displayed locked state (unless manually locked), or onboarding if displaying
  /// onboarding state
  private func handleKeyringStoreLoaded() {
    guard !didHandleKeyringLoaded else { return }
    didHandleKeyringLoaded = true
    if visibleScreen == .onboarding {
      // automatically open full wallet when displaying onboarding
      presentWalletWithContext?(.panelUnlockOrSetup)
    } else if visibleScreen == .unlock, !keyringStore.lockedManually {
      // automatically open full unlock wallet view when displaying
      // locked panel unless user locked manually
      presentWalletWithContext?(.panelUnlockOrSetup)
    }
  }
}

struct WalletPanelView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var cryptoStore: CryptoStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var accountActivityStore: AccountActivityStore
  @ObservedObject var allowSolProviderAccess: Preferences.Option<Bool> = Preferences.Wallet
    .allowSolProviderAccess
  @ObservedObject var tabDappStore: TabDappStore
  var origin: URLOrigin
  var presentWalletWithContext: (PresentingContext) -> Void
  var presentBuySendSwap: () -> Void
  var buySendSwapBackground: InvisibleUIView

  @Environment(\.openURL) private var openWalletURL
  @Environment(\.pixelLength) private var pixelLength
  @Environment(\.sizeCategory) private var sizeCategory
  @ScaledMetric private var blockieSize = 54

  private let currencyFormatter: NumberFormatter = .usdCurrencyFormatter

  init(
    keyringStore: KeyringStore,
    cryptoStore: CryptoStore,
    networkStore: NetworkStore,
    accountActivityStore: AccountActivityStore,
    tabDappStore: TabDappStore,
    origin: URLOrigin,
    presentWalletWithContext: @escaping (PresentingContext) -> Void,
    presentBuySendSwap: @escaping () -> Void,
    buySendSwapBackground: InvisibleUIView
  ) {
    self.keyringStore = keyringStore
    self.cryptoStore = cryptoStore
    self.networkStore = networkStore
    self.accountActivityStore = accountActivityStore
    self.tabDappStore = tabDappStore
    self.origin = origin
    self.presentWalletWithContext = presentWalletWithContext
    self.presentBuySendSwap = presentBuySendSwap
    self.buySendSwapBackground = buySendSwapBackground

    currencyFormatter.currencyCode = accountActivityStore.currencyCode
  }

  @State private var ethPermittedAccounts: [String] = []
  @State private var isConnectHidden: Bool = false

  enum ConnectionStatus {
    case connected
    case disconnected
    case blocked

    func title(_ coin: BraveWallet.CoinType) -> String {
      switch self {
      case .connected:
        return Strings.Wallet.walletPanelConnected
      case .disconnected:
        if coin == .eth {
          return Strings.Wallet.walletPanelConnect
        } else {
          return Strings.Wallet.walletPanelDisconnected
        }
      case .blocked:
        return Strings.Wallet.walletPanelBlocked
      }
    }
  }

  private var accountStatus: ConnectionStatus {
    let selectedAccount = keyringStore.selectedAccount
    switch selectedAccount.coin {
    case .eth:
      return ethPermittedAccounts.contains(selectedAccount.address) ? .connected : .disconnected
    case .sol:
      if !allowSolProviderAccess.value {
        return .blocked
      } else {
        return tabDappStore.solConnectedAddresses.contains(selectedAccount.address)
          ? .connected : .disconnected
      }
    case .fil, .btc, .zec:
      return .blocked
    @unknown default:
      return .blocked
    }
  }

  @ViewBuilder private var connectButton: some View {
    Button {
      if accountStatus == .blocked {
        presentWalletWithContext(.settings)
      } else {
        presentWalletWithContext(
          .editSiteConnection(
            origin,
            handler: { accounts in
              if keyringStore.selectedAccount.coin == .eth {
                ethPermittedAccounts = accounts
              }
              isConnectHidden = isConnectButtonHidden()
            }
          )
        )
      }
    } label: {
      HStack {
        if keyringStore.selectedAccount.coin == .sol {
          Circle()
            .strokeBorder(.white, lineWidth: 1)
            .background(
              Circle()
                .foregroundColor(accountStatus == .connected ? .green : .red)
            )
            .frame(width: 12, height: 12)
          Text(accountStatus.title(keyringStore.selectedAccount.coin))
            .fontWeight(.bold)
            .lineLimit(1)
        } else {
          if accountStatus == .connected {
            Image(systemName: "checkmark")
          }
          Text(accountStatus.title(keyringStore.selectedAccount.coin))
            .fontWeight(.bold)
            .lineLimit(1)
        }
      }
      .foregroundColor(Color(.braveLabel))
      .font(.caption.weight(.semibold))
      .padding(.init(top: 6, leading: 12, bottom: 6, trailing: 12))
      .background(
        Color(.secondaryButtonTint)
          .clipShape(Capsule().inset(by: 0.5).stroke())
      )
      .clipShape(Capsule())
      .contentShape(Capsule())
    }
  }

  private var networkPickerButton: some View {
    NetworkPicker(
      style: .init(textColor: .braveLabel, borderColor: .secondaryButtonTint),
      isForOrigin: true,
      keyringStore: keyringStore,
      networkStore: networkStore
    )
  }

  private var pendingRequestsButton: some View {
    Button {
      presentWalletWithContext(.pendingRequests)
    } label: {
      Image(braveSystemName: "leo.notification.dot")
        .foregroundColor(Color(.braveLabel))
        .frame(minWidth: 30, minHeight: 44)
        .contentShape(Rectangle())
    }
  }

  private var fullscreenButton: some View {
    Button {
      presentWalletWithContext(.default(.portfolio))
    } label: {
      Image(systemName: "arrow.up.left.and.arrow.down.right")
        .foregroundColor(Color(.braveLabel))
        .rotationEffect(.init(degrees: 90))
        .frame(minWidth: 30, minHeight: 44)
        .contentShape(Rectangle())
    }
    .accessibilityLabel(Strings.Wallet.walletFullScreenAccessibilityTitle)
  }

  private var menuButton: some View {
    Menu {
      Button {
        keyringStore.lock()
      } label: {
        Label(Strings.Wallet.lock, braveSystemImage: "leo.lock")
      }
      Button {
        presentWalletWithContext(.settings)
      } label: {
        Label(Strings.Wallet.settings, braveSystemImage: "leo.settings")
      }
      Divider()
      Button {
        openWalletURL(WalletConstants.braveWalletSupportURL)
      } label: {
        Label(Strings.Wallet.helpCenter, braveSystemImage: "leo.info.outline")
      }
    } label: {
      Image(braveSystemName: "leo.more.horizontal")
        .foregroundColor(Color(.braveLabel))
        .frame(minWidth: 30, minHeight: 44)
        .contentShape(Rectangle())
    }
    .accessibilityLabel(Strings.Wallet.otherWalletActionsAccessibilityTitle)
  }

  /// A boolean value indicates to hide or unhide `Connect` button
  private func isConnectButtonHidden() -> Bool {
    let account = keyringStore.selectedAccount
    switch account.coin {
    case .eth:
      return false
    case .sol:
      for domain in Domain.allDomainsWithWalletPermissions(for: .sol) {
        if let accounts = domain.wallet_solanaPermittedAcccounts, !accounts.isEmpty {
          return false
        }
      }
      return true
    case .fil, .btc, .zec:
      return true
    default:
      return true
    }
  }

  var body: some View {
    ScrollView(.vertical, showsIndicators: false) {
      VStack(spacing: 0) {
        if sizeCategory.isAccessibilityCategory {
          VStack {
            Text(Strings.Wallet.braveWallet)
              .foregroundColor(Color(.braveLabel))
              .font(.headline)
              .background(
                Color.clear
              )
            HStack {
              fullscreenButton
              Spacer()
              if cryptoStore.pendingRequest != nil {
                pendingRequestsButton
                Spacer()
              }
              menuButton
            }
          }
          .padding(.horizontal, 16)
          .padding(.vertical, 4)
          .overlay(
            Color(.braveLabel).opacity(0.3)  // Divider
              .frame(height: pixelLength),
            alignment: .bottom
          )
        } else {
          HStack {
            fullscreenButton
            if cryptoStore.pendingRequest != nil {
              // fake bell icon for layout
              pendingRequestsButton
                .hidden()
            }
            Spacer()
            Text(Strings.Wallet.braveWallet)
              .foregroundColor(Color(.braveLabel))
              .font(.headline)
              .background(
                Color.clear
              )
            Spacer()
            if cryptoStore.pendingRequest != nil {
              pendingRequestsButton
            }
            menuButton
          }
          .padding(.horizontal, 16)
          .padding(.vertical, 4)
          .overlay(
            Color(.braveLabel).opacity(0.3)  // Divider
              .frame(height: pixelLength),
            alignment: .bottom
          )
        }
        VStack {
          HStack {
            VStack(alignment: .leading) {
              networkPickerButton
              if !isConnectHidden {
                connectButton
              }
            }
            Spacer()
          }
          .padding(.bottom, 12)
          VStack(spacing: 12) {
            Button {
              presentWalletWithContext(.accountSelection)
            } label: {
              Blockie(address: keyringStore.selectedAccount.blockieSeed)
                .frame(width: blockieSize, height: blockieSize)
                .overlay(
                  RoundedRectangle(cornerRadius: 4)
                    .strokeBorder(Color(.braveLabel).opacity(0.6), style: .init(lineWidth: 1))
                )
                .overlay(
                  Image(systemName: "chevron.down.circle.fill")
                    .font(.footnote)
                    .background(Color(.braveLabel).clipShape(Circle()))
                    .offset(x: -4, y: 4),
                  alignment: .bottomLeading
                )
            }
            VStack(spacing: 4) {
              Text(keyringStore.selectedAccount.name)
                .foregroundColor(Color(.braveLabel))
                .font(.headline)
              AddressView(address: keyringStore.selectedAccount.address) {
                Text(keyringStore.selectedAccount.address.truncatedAddress)
                  .foregroundColor(Color(.braveLabel))
                  .font(.callout)
                  .multilineTextAlignment(.center)
              }
            }
          }
          VStack(spacing: 4) {
            let nativeAsset = accountActivityStore.userAssets.first(where: {
              $0.token.symbol == networkStore.selectedChainForOrigin.symbol
                && $0.token.chainId == networkStore.selectedChainIdForOrigin
            })
            Text(
              String(
                format: "%.04f %@",
                nativeAsset?.totalBalance ?? 0.0,
                networkStore.selectedChainForOrigin.symbol
              )
            )
            .font(.title2.weight(.bold))
            Text(
              currencyFormatter.formatAsFiat(
                (Double(nativeAsset?.price ?? "") ?? 0)
                  * (nativeAsset?.totalBalance ?? 0.0)
              ) ?? ""
            )
            .font(.callout)
          }
          .foregroundColor(Color(.braveLabel))
          .padding(.vertical)
          HStack(spacing: 0) {
            Button {
              presentBuySendSwap()
            } label: {
              Image(braveSystemName: "leo.swap.horizontal")
                .foregroundColor(Color(.braveLabel))
                .imageScale(.large)
                .padding(.horizontal, 44)
                .padding(.vertical, 8)
            }
            .background(buySendSwapBackground)
            Color(.braveLabel).opacity(0.6)
              .frame(width: pixelLength)
            Button {
              presentWalletWithContext(.transactionHistory)
            } label: {
              Image(braveSystemName: "leo.history")
                .foregroundColor(Color(.braveLabel))
                .imageScale(.large)
                .padding(.horizontal, 44)
                .padding(.vertical, 8)
            }
          }
          .overlay(
            RoundedRectangle(cornerRadius: 8, style: .continuous).strokeBorder(
              Color(.braveLabel).opacity(0.6),
              style: .init(lineWidth: pixelLength)
            )
          )
        }
        .padding(EdgeInsets(top: 12, leading: 12, bottom: 24, trailing: 12))
      }
    }
    .foregroundColor(.white)
    .background(
      Color(.braveGroupedBackground)
        .ignoresSafeArea()
    )
    .onChange(of: cryptoStore.selectedTab) { tab in
      DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
        presentWalletWithContext(.default(tab))
      }
    }
    .onChange(of: cryptoStore.pendingRequest) { newValue in
      if newValue != nil {
        // Slight delay to allow dismissal of unlock modal before presenting pending request modal.
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
          presentWalletWithContext(.pendingRequests)
        }
      }
    }
    .onChange(of: keyringStore.selectedAccount) { _ in
      isConnectHidden = isConnectButtonHidden()
    }
    .onChange(of: tabDappStore.latestPendingPermissionRequest) { newValue in
      if let request = newValue, request.requestingOrigin == origin,
        request.coinType == keyringStore.selectedAccount.coin
      {
        presentWalletWithContext(
          .requestPermissions(
            request,
            onPermittedAccountsUpdated: { accounts in
              if request.coinType == .eth {
                ethPermittedAccounts = accounts
              } else if request.coinType == .sol {
                isConnectHidden = false
              }
              tabDappStore.latestPendingPermissionRequest = nil
            }
          )
        )
      }
    }
    .onAppear {
      if let accountCreationRequest = WalletProviderAccountCreationRequestManager.shared
        .firstPendingRequest(
          for: origin,
          coinTypes: WalletConstants.supportedCoinTypes(.dapps).elements
        )
      {
        presentWalletWithContext(.createAccount(accountCreationRequest))
      } else if let request = WalletProviderPermissionRequestsManager.shared.firstPendingRequest(
        for: origin,
        coinTypes: [.eth, .sol]
      ) {
        presentWalletWithContext(
          .requestPermissions(
            request,
            onPermittedAccountsUpdated: { accounts in
              if request.coinType == .eth {
                ethPermittedAccounts = accounts
              } else if request.coinType == .sol {
                isConnectHidden = false
              }
            }
          )
        )
      } else if cryptoStore.pendingRequest != nil {
        // race condition for when `pendingRequest` is assigned in CryptoStore before this view visible
        // Slight delay to allow dismissal of unlock modal before presenting pending request modal.
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
          presentWalletWithContext(.pendingRequests)
        }
      } else {
        cryptoStore.prepare()
      }
      if let url = origin.url, let accounts = Domain.walletPermissions(forUrl: url, coin: .eth) {
        ethPermittedAccounts = accounts
      }

      isConnectHidden = isConnectButtonHidden()

      accountActivityStore.update()
    }
  }
}

struct InvisibleUIView: UIViewRepresentable {
  let uiView = UIView()
  func makeUIView(context: Context) -> UIView {
    uiView.backgroundColor = .clear
    return uiView
  }
  func updateUIView(_ uiView: UIView, context: Context) {
  }
}

#if DEBUG
struct WalletPanelView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      WalletPanelView(
        keyringStore: .previewStoreWithWalletCreated,
        cryptoStore: .previewStore,
        networkStore: .previewStore,
        accountActivityStore: .previewStore,
        tabDappStore: .previewStore,
        origin: .init(url: URL(string: "https://app.uniswap.org")!),
        presentWalletWithContext: { _ in },
        presentBuySendSwap: {},
        buySendSwapBackground: InvisibleUIView()
      )
      WalletPanelView(
        keyringStore: .previewStore,
        cryptoStore: .previewStore,
        networkStore: .previewStore,
        accountActivityStore: .previewStore,
        tabDappStore: .previewStore,
        origin: .init(url: URL(string: "https://app.uniswap.org")!),
        presentWalletWithContext: { _ in },
        presentBuySendSwap: {},
        buySendSwapBackground: InvisibleUIView()
      )
      WalletPanelView(
        keyringStore: {
          let store = KeyringStore.previewStoreWithWalletCreated
          store.lock()
          return store
        }(),
        cryptoStore: .previewStore,
        networkStore: .previewStore,
        accountActivityStore: .previewStore,
        tabDappStore: .previewStore,
        origin: .init(url: URL(string: "https://app.uniswap.org")!),
        presentWalletWithContext: { _ in },
        presentBuySendSwap: {},
        buySendSwapBackground: InvisibleUIView()
      )
    }
    .fixedSize(horizontal: false, vertical: true)
    .previewLayout(.sizeThatFits)
  }
}
#endif
