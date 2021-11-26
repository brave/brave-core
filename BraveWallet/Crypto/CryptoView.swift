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
  @ObservedObject var walletStore: WalletStore
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
        UIKitNavigationView {
          CryptoPagesView(walletStore: walletStore)
            .toolbar {
              dismissButtonToolbarContents
            }
        }
        .background(
          Color.clear
            .sheet(item: $walletStore.buySendSwapDestination) { action in
              switch action {
              case .buy:
                BuyTokenView(
                  keyringStore: walletStore.keyringStore,
                  networkStore: walletStore.networkStore,
                  buyTokenStore: walletStore.buyTokenStore
                )
              case .send:
                SendTokenView(
                  keyringStore: walletStore.keyringStore,
                  networkStore: walletStore.networkStore,
                  sendTokenStore: walletStore.sendTokenStore
                )
              case .swap:
                SwapCryptoView(
                  keyringStore: walletStore.keyringStore,
                  ethNetworkStore: walletStore.networkStore,
                  swapTokensStore: walletStore.swapTokenStore
                )
              }
            }
        )
        .background(
          Color.clear
            .sheet(isPresented: $walletStore.isPresentingTransactionConfirmations) {
              if !walletStore.unapprovedTransactions.isEmpty {
                TransactionConfirmationView(
                  transactions: walletStore.unapprovedTransactions,
                  confirmationStore: walletStore.confirmationStore,
                  networkStore: walletStore.networkStore,
                  keyringStore: walletStore.keyringStore
                )
              }
            }
        )
        .transition(.asymmetric(insertion: .identity, removal: .opacity))
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
    .environment(\.buySendSwapDestination, $walletStore.buySendSwapDestination)
  }
}
