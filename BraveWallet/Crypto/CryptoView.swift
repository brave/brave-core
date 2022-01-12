/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import UIKit
import SwiftUI
import BraveCore
import Introspect
import BraveUI

public struct CryptoView: View {
  var walletStore: WalletStore
  @ObservedObject var keyringStore: KeyringStore
  
  // in iOS 15, PresentationMode will be available in SwiftUI hosted by UIHostingController
  // but for now we'll have to manage this ourselves
  var dismissAction: (() -> Void)?
  
  var openWalletURLAction: ((URL) -> Void)?
  
  public init(
    walletStore: WalletStore,
    keyringStore: KeyringStore
  ) {
    self.walletStore = walletStore
    self.keyringStore = keyringStore
  }
  
  private enum VisibleScreen: Equatable {
    case crypto
    case onboarding
    case unlock
  }
  
  private var visibleScreen: VisibleScreen {
    let keyring = keyringStore.keyring
    if !keyring.isDefaultKeyringCreated || keyringStore.isOnboardingVisible {
      return .onboarding
    }
    if keyring.isLocked || keyringStore.isRestoreFromUnlockBiometricsPromptVisible {
      return .unlock
    }
    return .crypto
  }
  
  @ToolbarContentBuilder
  private var dismissButtonToolbarContents: some ToolbarContent {
    ToolbarItemGroup(placement: .cancellationAction) {
      Button(action: { dismissAction?() }) {
        Image("wallet-dismiss")
          .renderingMode(.template)
          .foregroundColor(Color(.braveOrange))
      }
    }
  }
  
  public var body: some View {
    ZStack {
      switch visibleScreen {
      case .crypto:
        if let store = walletStore.cryptoStore {
          CryptoContainerView(
            keyringStore: keyringStore,
            cryptoStore: store,
            toolbarDismissContent: dismissButtonToolbarContents
          )
          .transition(.asymmetric(insertion: .identity, removal: .opacity))
        }
      case .unlock:
        UIKitNavigationView {
          UnlockWalletView(keyringStore: keyringStore)
            .toolbar {
              dismissButtonToolbarContents
            }
        }
        .transition(.move(edge: .bottom))
        .zIndex(1)  // Needed or the dismiss animation messes up
      case .onboarding:
        UIKitNavigationView {
          SetupCryptoView(keyringStore: keyringStore)
            .toolbar {
              dismissButtonToolbarContents
            }
        }
        .transition(.move(edge: .bottom))
        .zIndex(2)  // Needed or the dismiss animation messes up
      }
    }
    .animation(.default, value: visibleScreen) // Animate unlock dismiss (required for some reason)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .environment(\.openWalletURLAction, .init(action: { url in
      openWalletURLAction?(url)
    }))
  }
}

private struct CryptoContainerView<DismissContent: ToolbarContent>: View {
  var keyringStore: KeyringStore
  @ObservedObject var cryptoStore: CryptoStore
  var toolbarDismissContent: DismissContent
  
  var body: some View {
    UIKitNavigationView {
      CryptoPagesView(cryptoStore: cryptoStore, keyringStore: keyringStore)
        .toolbar {
          toolbarDismissContent
        }
    }
    .background(
      Color.clear
        .sheet(item: $cryptoStore.buySendSwapDestination) { action in
          switch action.kind {
          case .buy:
            BuyTokenView(
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore,
              buyTokenStore: cryptoStore.openBuyTokenStore(action.initialToken)
            )
          case .send:
            SendTokenView(
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore,
              sendTokenStore: cryptoStore.openSendTokenStore(action.initialToken)
            )
          case .swap:
            SwapCryptoView(
              keyringStore: keyringStore,
              ethNetworkStore: cryptoStore.networkStore,
              swapTokensStore: cryptoStore.openSwapTokenStore(action.initialToken)
            )
          }
        }
    )
    .background(
      Color.clear
        .sheet(isPresented: $cryptoStore.isPresentingTransactionConfirmations) {
          if !cryptoStore.unapprovedTransactions.isEmpty {
            TransactionConfirmationView(
              transactions: cryptoStore.unapprovedTransactions,
              confirmationStore: cryptoStore.openConfirmationStore(),
              networkStore: cryptoStore.networkStore,
              keyringStore: keyringStore
            )
          }
        }
    )
    .environment(\.buySendSwapDestination, $cryptoStore.buySendSwapDestination)
  }
}
