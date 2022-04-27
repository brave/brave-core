// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import SwiftUI
import BraveUI
import struct Shared.Strings

public protocol WalletSiteConnectionDelegate {
  /// A list of accounts connected to this webpage (addresses)
  var connectedAccounts: [String] { get }
  /// Update the connection status for a given account
  func updateConnectionStatusForAccountAddress(_ address: String)
}

public struct WalletPanelContainerView: View {
  var walletStore: WalletStore
  @ObservedObject var keyringStore: KeyringStore
  var origin: URL
  var presentWalletWithContext: ((PresentingContext) -> Void)?
  
  // When the screen first apperas the keyring is set as the default value
  // which causes an unnessary animation
  @State private var fetchingInitialKeyring: Bool = true
  
  private enum VisibleScreen: Equatable {
    case panel
    case onboarding
    case unlock
  }
  
  private var visibleScreen: VisibleScreen {
    let keyring = keyringStore.keyring
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
      Image("graphic-lock")
        .resizable()
        .aspectRatio(contentMode: .fit)
        .frame(maxWidth: 150)
      Button {
        presentWalletWithContext?(.panelUnlockOrSetup)
      } label: {
        HStack(spacing: 4) {
          Image("brave.unlock")
          Text("Unlock Wallet")
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
          Text("Brave Wallet")
            .foregroundColor(Color(.bravePrimary))
            .font(.headline)
          Text("Use this panel to securely access web3 and all your crypto assets.")
            .foregroundColor(Color(.secondaryBraveLabel))
            .font(.subheadline)
        }
        .multilineTextAlignment(.center)
        Button {
          presentWalletWithContext?(.panelUnlockOrSetup)
        } label: {
          Text("Learn More")
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
      case .panel:
        if let cryptoStore = walletStore.cryptoStore {
          WalletPanelView(
            keyringStore: keyringStore,
            cryptoStore: cryptoStore,
            networkStore: cryptoStore.networkStore,
            origin: origin,
            presentWalletWithContext: { context in
              self.presentWalletWithContext?(context)
            }
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
    .animation(fetchingInitialKeyring ? nil : .default, value: visibleScreen)
    .frame(idealWidth: 320, maxWidth: .infinity)
    .onAppear {
      fetchingInitialKeyring = keyringStore.keyring.id.isEmpty
    }
    .onChange(of: keyringStore.keyring) { newValue in
      fetchingInitialKeyring = false
      if visibleScreen != .panel {
        presentWalletWithContext?(.panelUnlockOrSetup)
      }
    }
  }
}

struct WalletPanelView: View {
  @ObservedObject var keyringStore: KeyringStore
  @ObservedObject var cryptoStore: CryptoStore
  @ObservedObject var networkStore: NetworkStore
  var origin: URL
  var presentWalletWithContext: (PresentingContext) -> Void
  
  @Environment(\.pixelLength) private var pixelLength
  @Environment(\.sizeCategory) private var sizeCategory
  @ScaledMetric private var blockieSize = 54
  
  private var connectButton: some View {
    Button {
      
    } label: {
      HStack {
        Image(systemName: "checkmark")
        Text("Connected…")
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
    Button {
      
    } label: {
      HStack {
        Text(networkStore.selectedChain.shortChainName)
          .fontWeight(.bold)
          .lineLimit(1)
        Image(systemName: "chevron.down.circle")
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
  
  var body: some View {
    ScrollView(.vertical, showsIndicators: false) {
      VStack(spacing: 0) {
        Text("Brave Wallet")
          .font(.headline)
          .padding(16)
          .frame(maxWidth: .infinity)
          .overlay(
            Color.white.opacity(0.3) // Divider
              .frame(height: pixelLength),
            alignment: .bottom
          )
          .background(
            Color.clear
          )
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
              Text(keyringStore.selectedAccount.address.truncatedAddress)
                .font(.callout)
            }
          }
          VStack(spacing: 4) {
            Text("0.31178 ETH")
              .font(.title2.weight(.bold))
            Text("$872.48")
              .font(.callout)
          }
          .padding(.vertical)
          HStack(spacing: 0) {
            Button {
              
            } label: {
              Image("brave.arrow.left.arrow.right")
                .imageScale(.large)
                .padding(.horizontal, 44)
                .padding(.vertical, 8)
            }
            Color.white.opacity(0.6)
              .frame(width: pixelLength)
            Button {
            } label: {
              Image("brave.history")
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
      LinearGradient(
        colors: [.green, .blue],
        startPoint: .top,
        endPoint: .bottom
      )
      .ignoresSafeArea()
    )
    .onChange(of: cryptoStore.pendingWebpageRequest) { newValue in
      if newValue != nil {
        presentWalletWithContext(.webpageRequests)
      }
    }
    .onAppear {
      let permissionRequestManager = WalletProviderPermissionRequestsManager.shared
      if let request = permissionRequestManager.pendingRequests(for: origin).first {
        presentWalletWithContext(.requestEthererumPermissions(request))
      } else {
        cryptoStore.fetchPendingRequests()
      }
    }
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
        origin: URL(string: "https://app.uniswap.org")!,
        presentWalletWithContext: { _ in }
      )
      WalletPanelView(
        keyringStore: .previewStore,
        cryptoStore: .previewStore,
        networkStore: .previewStore,
        origin: URL(string: "https://app.uniswap.org")!,
        presentWalletWithContext: { _ in }
      )
      WalletPanelView(
        keyringStore: {
          let store = KeyringStore.previewStoreWithWalletCreated
          store.lock()
          return store
        }(),
        cryptoStore: .previewStore,
        networkStore: .previewStore,
        origin: URL(string: "https://app.uniswap.org")!,
        presentWalletWithContext: { _ in }
      )
    }
    .fixedSize(horizontal: false, vertical: true)
    .previewLayout(.sizeThatFits)
  }
}
#endif
