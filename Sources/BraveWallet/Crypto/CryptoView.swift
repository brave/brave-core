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
  var presentingContext: PresentingContext

  // in iOS 15, PresentationMode will be available in SwiftUI hosted by UIHostingController
  // but for now we'll have to manage this ourselves
  var dismissAction: (() -> Void)?

  var openWalletURLAction: ((URL) -> Void)?
  
  var faviconRenderer: WalletFaviconRenderer

  public init(
    walletStore: WalletStore,
    keyringStore: KeyringStore,
    presentingContext: PresentingContext,
    faviconRenderer: WalletFaviconRenderer
  ) {
    self.walletStore = walletStore
    self.keyringStore = keyringStore
    self.presentingContext = presentingContext
    self.faviconRenderer = faviconRenderer
  }

  private enum VisibleScreen: Equatable {
    case crypto
    case onboarding
    case unlock
  }

  private var visibleScreen: VisibleScreen {
    let keyring = keyringStore.defaultKeyring
    if !keyring.isKeyringCreated || keyringStore.isOnboardingVisible {
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
      Button(action: {
        if case .requestEthererumPermissions(let request, let onPermittedAccountsUpdated) = presentingContext {
          request.decisionHandler(.rejected)
          onPermittedAccountsUpdated([])
        }
        dismissAction?()
      }) {
        Image("wallet-dismiss", bundle: .current)
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
          Group {
            switch presentingContext {
            case .`default`:
              CryptoContainerView(
                keyringStore: keyringStore,
                cryptoStore: store,
                toolbarDismissContent: dismissButtonToolbarContents
              )
            case .pendingRequests:
              RequestContainerView(
                keyringStore: keyringStore,
                cryptoStore: store,
                toolbarDismissContent: dismissButtonToolbarContents,
                onDismiss: {
                  dismissAction?()
                }
              )
            case .requestEthererumPermissions(let request, let onPermittedAccountsUpdated):
              NewSiteConnectionView(
                origin: request.requestingOrigin,
                coin: request.coinType,
                keyringStore: keyringStore,
                onConnect: {
                  request.decisionHandler(.granted(accounts: $0))
                  onPermittedAccountsUpdated($0)
                  dismissAction?()
                },
                onDismiss: {
                  request.decisionHandler(.rejected)
                  onPermittedAccountsUpdated([])
                  dismissAction?()
                }
              )
            case .panelUnlockOrSetup:
              EmptyView()
            case .accountSelection:
              AccountListView(
                keyringStore: keyringStore,
                onDismiss: {
                  dismissAction?()
                }
              )
            case .transactionHistory:
              NavigationView {
                AccountTransactionListView(
                  keyringStore: walletStore.keyringStore,
                  activityStore: store.accountActivityStore(for: walletStore.keyringStore.selectedAccount),
                  networkStore: store.networkStore
                )
                .toolbar {
                  dismissButtonToolbarContents
                }
              }
              .navigationViewStyle(.stack)
            case .buySendSwap(let destination):
              switch destination.kind {
              case .buy:
                BuyTokenView(
                  keyringStore: keyringStore,
                  networkStore: store.networkStore,
                  buyTokenStore: store.openBuyTokenStore(destination.initialToken),
                  onDismiss: {
                    store.closeBSSStores()
                    dismissAction?()
                  }
                )
              case .send:
                SendTokenView(
                  keyringStore: keyringStore,
                  networkStore: store.networkStore,
                  sendTokenStore: store.openSendTokenStore(destination.initialToken),
                  completion: { success in
                    if success {
                      store.closeBSSStores()
                      dismissAction?()
                    }
                  },
                  onDismiss: {
                    store.closeBSSStores()
                    dismissAction?()
                  }
                )
              case .swap:
                SwapCryptoView(
                  keyringStore: keyringStore,
                  networkStore: store.networkStore,
                  swapTokensStore: store.openSwapTokenStore(destination.initialToken),
                  completion: { success in
                    if success {
                      store.closeBSSStores()
                      dismissAction?()
                    }
                  },
                  onDismiss: {
                    store.closeBSSStores()
                    dismissAction?()
                  }
                )
              }
            case .settings:
              NavigationView {
                WalletSettingsView(
                  settingsStore: store.settingsStore,
                  networkStore: store.networkStore,
                  keyringStore: keyringStore
                )
                .toolbar {
                  dismissButtonToolbarContents
                }
              }
              .navigationViewStyle(.stack)
            case .editSiteConnection(let origin, let handler):
              EditSiteConnectionView(
                keyringStore: keyringStore,
                origin: origin,
                coin: .eth, // TODO: switch to dynamic coin type once we support Solona Dapps
                onDismiss: { accounts in
                  handler(accounts)
                  dismissAction?()
                }
              )
            }
          }
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
    .animation(.default, value: visibleScreen)  // Animate unlock dismiss (required for some reason)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .environment(
      \.openWalletURLAction,
      .init(action: { [openWalletURLAction] url in
        openWalletURLAction?(url)
      }))
    .environment(
      \.faviconRenderer,
       faviconRenderer
    )
    .onChange(of: visibleScreen) { newValue in
      if case .panelUnlockOrSetup = presentingContext, newValue == .crypto {
        dismissAction?()
      }
    }
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
              buyTokenStore: cryptoStore.openBuyTokenStore(action.initialToken),
              onDismiss: { cryptoStore.buySendSwapDestination = nil }
            )
          case .send:
            SendTokenView(
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore,
              sendTokenStore: cryptoStore.openSendTokenStore(action.initialToken),
              onDismiss: { cryptoStore.buySendSwapDestination = nil }
            )
          case .swap:
            SwapCryptoView(
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore,
              swapTokensStore: cryptoStore.openSwapTokenStore(action.initialToken),
              onDismiss: { cryptoStore.buySendSwapDestination = nil }
            )
          }
        }
    )
    .background(
      Color.clear
        .sheet(isPresented: $cryptoStore.isPresentingPendingRequest) {
          if cryptoStore.pendingRequest != nil {
            RequestContainerView(
              keyringStore: keyringStore,
              cryptoStore: cryptoStore,
              toolbarDismissContent: toolbarDismissContent,
              onDismiss: {
                cryptoStore.isPresentingPendingRequest = false
              }
            )
          }
        }
    )
    .environment(
      \.buySendSwapDestination,
      Binding(
        get: { [weak cryptoStore] in cryptoStore?.buySendSwapDestination },
        set: { [weak cryptoStore] destination in
          if cryptoStore?.isPresentingAssetSearch == true {
            cryptoStore?.isPresentingAssetSearch = false
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) {
              cryptoStore?.buySendSwapDestination = destination
            }
          } else {
            cryptoStore?.buySendSwapDestination = destination
          }
        })
    )
  }
}
