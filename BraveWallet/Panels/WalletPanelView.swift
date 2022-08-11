// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveCore
import DesignSystem
import Strings
import Data

public protocol WalletSiteConnectionDelegate {
  /// A list of accounts connected to this webpage (addresses)
  var connectedAccounts: [String] { get }
  /// Update the connection status for a given account
  func updateConnectionStatusForAccountAddress(_ address: String)
}

public struct WalletPanelContainerView: View {
  var walletStore: WalletStore
  @ObservedObject var keyringStore: KeyringStore
  var origin: URLOrigin
  var presentWalletWithContext: ((PresentingContext) -> Void)?
  var presentBuySendSwap: (() -> Void)?
  /// An invisible `UIView` background lives in SwiftUI for UIKit API to reference later
  var buySendSwapBackground: InvisibleUIView = .init()
  
  private enum VisibleScreen: Equatable {
    case loading
    case panel
    case onboarding
    case unlock
  }

  private var visibleScreen: VisibleScreen {
    let keyring = keyringStore.defaultKeyring
    if keyringStore.defaultKeyring.id.isEmpty || keyringStore.selectedAccount.address.isEmpty {
      return .loading
    }
    if !keyring.isKeyringCreated || keyringStore.isOnboardingVisible {
      return .onboarding
    }
    if keyring.isLocked || keyringStore.isRestoreFromUnlockBiometricsPromptVisible {
      return .unlock
    }
    return .panel
  }
  
  private var lockedView: some View {
    VStack(spacing: 36) {
      Image("graphic-lock", bundle: .current)
        .resizable()
        .aspectRatio(contentMode: .fit)
        .frame(maxWidth: 150)
      Button {
        presentWalletWithContext?(.panelUnlockOrSetup)
      } label: {
        HStack(spacing: 4) {
          Image(braveSystemName: "brave.unlock")
          Text(Strings.Wallet.walletPanelUnlockWallet)
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
          .hidden() // used for sizing to prevent #5378
          .accessibilityHidden(true)
        Color.white
          .overlay(ProgressView())
      case .panel:
        if let cryptoStore = walletStore.cryptoStore {
          WalletPanelView(
            keyringStore: keyringStore,
            cryptoStore: cryptoStore,
            networkStore: cryptoStore.networkStore,
            accountActivityStore: cryptoStore.accountActivityStore(for: keyringStore.selectedAccount),
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
    .onChange(of: keyringStore.defaultKeyring) { newValue in
      if visibleScreen != .panel, !keyringStore.lockedManually {
        presentWalletWithContext?(.panelUnlockOrSetup)
      }
    }
  }
}

struct WalletPanelView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var cryptoStore: CryptoStore
  @ObservedObject var networkStore: NetworkStore
  @ObservedObject var accountActivityStore: AccountActivityStore
  var origin: URLOrigin
  var presentWalletWithContext: (PresentingContext) -> Void
  var presentBuySendSwap: () -> Void
  var buySendSwapBackground: InvisibleUIView
  
  @Environment(\.pixelLength) private var pixelLength
  @Environment(\.sizeCategory) private var sizeCategory
  @ScaledMetric private var blockieSize = 54
  
  private let currencyFormatter: NumberFormatter = .usdCurrencyFormatter
  
  init(
    keyringStore: KeyringStore,
    cryptoStore: CryptoStore,
    networkStore: NetworkStore,
    accountActivityStore: AccountActivityStore,
    origin: URLOrigin,
    presentWalletWithContext: @escaping (PresentingContext) -> Void,
    presentBuySendSwap: @escaping () -> Void,
    buySendSwapBackground: InvisibleUIView
  ) {
    self.keyringStore = keyringStore
    self.cryptoStore = cryptoStore
    self.networkStore = networkStore
    self.accountActivityStore = accountActivityStore
    self.origin = origin
    self.presentWalletWithContext = presentWalletWithContext
    self.presentBuySendSwap = presentBuySendSwap
    self.buySendSwapBackground = buySendSwapBackground
    
    currencyFormatter.currencyCode = accountActivityStore.currencyCode
  }
  
  @State private var permittedAccounts: [String] = []
  
  private var isConnected: Bool {
    return permittedAccounts.contains(keyringStore.selectedAccount.address)
  }
  
  private var connectButton: some View {
    Button {
      presentWalletWithContext(.editSiteConnection(origin, handler: { accounts in
        permittedAccounts = accounts
      }))
    } label: {
      HStack {
        if isConnected {
          Image(systemName: "checkmark")
        }
        Text(isConnected ? Strings.Wallet.walletPanelConnected : Strings.Wallet.walletPanelConnect)
          .fontWeight(.bold)
          .lineLimit(1)
      }
      .foregroundColor(.white)
      .font(.caption.weight(.semibold))
      .padding(.init(top: 6, leading: 12, bottom: 6, trailing: 12))
      .background(
        Color.white.opacity(0.5)
          .clipShape(Capsule().inset(by: 0.5).stroke())
      )
      .clipShape(Capsule())
      .contentShape(Capsule())
    }
  }
  
  private var networkPickerButton: some View {
    NetworkPicker(
      style: .init(textColor: .white, borderColor: .white),
      keyringStore: keyringStore,
      networkStore: networkStore
    )
  }
  
  private var pendingRequestsButton: some View {
    Button(action: { presentWalletWithContext(.pendingRequests) }) {
      Image(braveSystemName: "brave.bell.badge")
        .foregroundColor(.white)
        .frame(minWidth: 30, minHeight: 44)
        .contentShape(Rectangle())
    }
  }
  
  private var fullscreenButton: some View {
    Button {
      presentWalletWithContext(.default)
    } label: {
      Image(systemName: "arrow.up.left.and.arrow.down.right")
        .rotationEffect(.init(degrees: 90))
        .frame(minWidth: 30, minHeight: 44)
        .contentShape(Rectangle())
    }
    .accessibilityLabel(Strings.Wallet.walletFullScreenAccessibilityTitle)
  }
  
  private var menuButton: some View {
    Menu {
      Button(action: { keyringStore.lock() }) {
        Label(Strings.Wallet.lock, braveSystemImage: "brave.lock")
      }
      Divider()
      Button(action: { presentWalletWithContext(.settings) }) {
        Label(Strings.Wallet.settings, braveSystemImage: "brave.gear")
      }
    } label: {
      Image(systemName: "ellipsis")
        .frame(minWidth: 30, minHeight: 44)
        .contentShape(Rectangle())
    }
    .accessibilityLabel(Strings.Wallet.otherWalletActionsAccessibilityTitle)
  }
  
  var body: some View {
    ScrollView(.vertical, showsIndicators: false) {
      VStack(spacing: 0) {
        if sizeCategory.isAccessibilityCategory {
          VStack {
            Text(Strings.Wallet.braveWallet)
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
            Color.white.opacity(0.3) // Divider
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
            Color.white.opacity(0.3) // Divider
              .frame(height: pixelLength),
            alignment: .bottom
          )
        }
        VStack {
          if sizeCategory.isAccessibilityCategory {
            VStack {
              connectButton
              networkPickerButton
            }
          } else {
            HStack {
              connectButton
              Spacer()
              networkPickerButton
            }
          }
          VStack(spacing: 12) {
            Button {
              presentWalletWithContext(.accountSelection)
            } label: {
              Blockie(address: keyringStore.selectedAccount.address)
                .frame(width: blockieSize, height: blockieSize)
                .overlay(
                  Circle().strokeBorder(lineWidth: 2, antialiased: true)
                )
                .overlay(
                  Image(systemName: "chevron.down.circle.fill")
                    .font(.footnote)
                    .background(Color(.braveLabel).clipShape(Circle())),
                  alignment: .bottomLeading
                )
            }
            VStack(spacing: 4) {
              Text(keyringStore.selectedAccount.name)
                .font(.headline)
              AddressView(address: keyringStore.selectedAccount.address) {
                Text(keyringStore.selectedAccount.address.truncatedAddress)
                  .font(.callout)
                  .multilineTextAlignment(.center)
              }
            }
          }
          VStack(spacing: 4) {
            let nativeAsset = accountActivityStore.assets.first(where: { $0.token.symbol == networkStore.selectedChain.symbol })
            Text(String(format: "%.04f %@", nativeAsset?.decimalBalance ?? 0.0, networkStore.selectedChain.symbol))
              .font(.title2.weight(.bold))
            Text(currencyFormatter.string(from: NSNumber(value: (Double(nativeAsset?.price ?? "") ?? 0) * (nativeAsset?.decimalBalance ?? 0.0))) ?? "")
              .font(.callout)
          }
          .padding(.vertical)
          HStack(spacing: 0) {
            Button {
              presentBuySendSwap()
            } label: {
              Image(braveSystemName: "brave.arrow.left.arrow.right")
                .imageScale(.large)
                .padding(.horizontal, 44)
                .padding(.vertical, 8)
            }
            .background(buySendSwapBackground)
            Color.white.opacity(0.6)
              .frame(width: pixelLength)
            Button {
              presentWalletWithContext(.transactionHistory)
            } label: {
              Image(braveSystemName: "brave.history")
                .imageScale(.large)
                .padding(.horizontal, 44)
                .padding(.vertical, 8)
            }
          }
          .overlay(RoundedRectangle(cornerRadius: 8, style: .continuous).strokeBorder(Color.white.opacity(0.6), style: .init(lineWidth: pixelLength)))
        }
        .padding(EdgeInsets(top: 12, leading: 12, bottom: 24, trailing: 12))
      }
    }
    .foregroundColor(.white)
    .background(
      BlockieMaterial(address: keyringStore.selectedAccount.id)
      .ignoresSafeArea()
    )
    .onChange(of: cryptoStore.pendingRequest) { newValue in
      if newValue != nil {
        presentWalletWithContext(.pendingRequests)
      }
    }
    .onAppear {
      let permissionRequestManager = WalletProviderPermissionRequestsManager.shared
      if let request = permissionRequestManager.pendingRequests(for: origin, coinType: .eth).first {
        presentWalletWithContext(.requestEthererumPermissions(request, onPermittedAccountsUpdated: { accounts in
          permittedAccounts = accounts
        }))
      } else {
        cryptoStore.prepare()
      }
      if let url = origin.url, let accounts = Domain.ethereumPermissions(forUrl: url) {
        permittedAccounts = accounts
      }
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
