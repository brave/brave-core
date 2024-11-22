// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
import Foundation
import Introspect
import Preferences
import SwiftUI
import UIKit

public struct CryptoView: View {
  var walletStore: WalletStore
  @ObservedObject var keyringStore: KeyringStore
  var webImageDownloader: WebImageDownloaderType
  var presentingContext: PresentingContext
  @Environment(\.presentationMode) @Binding private var presentationMode

  var openWalletURLAction: ((URL) -> Void)?

  var appRatingRequestAction: (() -> Void)?

  @ObservedObject var isOnboardingCompleted = Preferences.Wallet.isOnboardingCompleted

  public init(
    walletStore: WalletStore,
    keyringStore: KeyringStore,
    webImageDownloader: WebImageDownloaderType,
    presentingContext: PresentingContext
  ) {
    self.walletStore = walletStore
    self.keyringStore = keyringStore
    self.webImageDownloader = webImageDownloader
    self.presentingContext = presentingContext
  }

  private enum VisibleScreen: Equatable {
    case crypto
    case onboarding
    case unlock
  }

  private var visibleScreen: VisibleScreen {
    if !keyringStore.isWalletCreated || keyringStore.isOnboardingVisible {
      return .onboarding
    }
    if keyringStore.isWalletLocked || keyringStore.isRestoreFromUnlockBiometricsPromptVisible {
      return .unlock
    }
    return .crypto
  }

  @ToolbarContentBuilder
  private var dismissButtonToolbarContents: some ToolbarContent {
    ToolbarItemGroup(placement: .cancellationAction) {
      Button {
        if case .requestPermissions(let request, let onPermittedAccountsUpdated) = presentingContext
        {
          request.decisionHandler(.rejected)
          onPermittedAccountsUpdated([])
        }
        dismissAction()
      } label: {
        Image("wallet-dismiss", bundle: .module)
          .renderingMode(.template)
          .foregroundColor(Color(.braveBlurpleTint))
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
            case .`default`(let selectedTab):
              CryptoContainerView(
                keyringStore: keyringStore,
                cryptoStore: store,
                toolbarDismissContent: dismissButtonToolbarContents,
                selectedTab: selectedTab
              )
            case .pendingRequests:
              RequestContainerView(
                keyringStore: keyringStore,
                cryptoStore: store,
                toolbarDismissContent: dismissButtonToolbarContents,
                onDismiss: {
                  dismissAction()
                },
                onViewInActivity: {
                  store.selectedTab = .activity
                }
              )
              .onDisappear {
                // onDisappear allows us to catch all cases (swipe, cancel, confirm/approve/sign)
                store.isPresentingPendingRequest = false
                store.prepare()
              }
            case .requestPermissions(let request, let onPermittedAccountsUpdated):
              NewSiteConnectionView(
                origin: request.requestingOrigin,
                accounts: request.requestingAccounts,
                coin: request.coinType,
                keyringStore: keyringStore,
                onConnect: {
                  request.decisionHandler(.granted(accounts: $0))
                  onPermittedAccountsUpdated($0)
                  dismissAction()
                },
                onDismiss: {
                  request.decisionHandler(.rejected)
                  onPermittedAccountsUpdated([])
                  dismissAction()
                }
              )
            case .panelUnlockOrSetup:
              EmptyView()
            case .accountSelection:
              NavigationView {
                AccountSelectionView(
                  keyringStore: keyringStore,
                  networkStore: store.networkStore,
                  onDismiss: {
                    dismissAction()
                  }
                )
              }
              .navigationViewStyle(.stack)
            case .transactionHistory:
              NavigationView {
                AccountTransactionListView(
                  activityStore: store.accountActivityStore(
                    for: walletStore.keyringStore.selectedAccount,
                    isWalletPanel: true
                  ),
                  networkStore: store.networkStore
                )
                .onDisappear {
                  store.closeAccountActivityStore(for: walletStore.keyringStore.selectedAccount)
                }
                .toolbar {
                  dismissButtonToolbarContents
                }
              }
              .navigationViewStyle(.stack)
            case .walletAction(let destination):
              switch destination.kind {
              case .buy:
                BuyTokenView(
                  keyringStore: keyringStore,
                  networkStore: store.networkStore,
                  buyTokenStore: store.openBuyTokenStore(destination.initialToken),
                  onDismiss: {
                    store.closeWalletActionStores()
                    dismissAction()
                  }
                )
              case .send:
                SendTokenView(
                  keyringStore: keyringStore,
                  networkStore: store.networkStore,
                  sendTokenStore: store.openSendTokenStore(destination.initialToken),
                  completion: { success in
                    if success {
                      store.closeWalletActionStores()
                      dismissAction()
                    }
                  },
                  onDismiss: {
                    store.closeWalletActionStores()
                    dismissAction()
                  }
                )
              case .swap:
                SwapCryptoView(
                  keyringStore: keyringStore,
                  networkStore: store.networkStore,
                  swapTokensStore: store.openSwapTokenStore(destination.initialToken),
                  completion: { success in
                    if success {
                      store.closeWalletActionStores()
                      dismissAction()
                    }
                  },
                  onDismiss: {
                    store.closeWalletActionStores()
                    dismissAction()
                  }
                )
              case .deposit(let query):
                DepositTokenView(
                  keyringStore: keyringStore,
                  networkStore: store.networkStore,
                  depositTokenStore: store.openDepositTokenStore(
                    prefilledToken: destination.initialToken,
                    prefilledAccount: destination.initialAccount
                  ),
                  prefilledQuery: query,
                  onDismiss: {
                    store.closeWalletActionStores()
                    dismissAction()
                  }
                )
              }
            case .settings:
              NavigationView {
                Web3SettingsView(
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
                coin: keyringStore.selectedAccount.coin,
                onDismiss: { accounts in
                  handler(accounts)
                  dismissAction()
                }
              )
            case .createAccount(let request):
              NavigationView {
                AddAccountView(
                  keyringStore: keyringStore,
                  networkStore: store.networkStore,
                  preSelectedCoin: request.coinType,
                  onCreate: {
                    // request is fullfilled.
                    request.responseHandler(.created)
                    dismissAction()
                  },
                  onDismiss: {
                    // request get declined by clicking `Cancel`
                    request.responseHandler(.rejected)
                    dismissAction()
                  }
                )
              }
              .navigationViewStyle(.stack)
            }
          }
          .transition(.asymmetric(insertion: .identity, removal: .opacity))
        }
      case .unlock:
        UIKitNavigationView {
          UnlockWalletView(keyringStore: keyringStore, dismissAction: dismissAction)
            .toolbar {
              dismissButtonToolbarContents
            }
        }
        .transition(.move(edge: .bottom))
        .zIndex(1)  // Needed or the dismiss animation messes up
      case .onboarding:
        if isOnboardingCompleted.value {
          UIKitNavigationView {
            OnboardingCompletedView(keyringStore: keyringStore)
          }
          .transition(.move(edge: .bottom))
          .zIndex(2)  // Needed or the dismiss animation messes up
        } else {
          UIKitNavigationView {
            SetupCryptoView(keyringStore: keyringStore, dismissAction: dismissAction)
              .toolbar {
                ToolbarItemGroup(placement: .destructiveAction) {
                  Button {
                    dismissAction()
                  } label: {
                    Text(Strings.CancelString)
                  }
                }
              }
          }
          .transition(.move(edge: .bottom))
          .zIndex(3)  // Needed or the dismiss animation messes up
        }
      }
    }
    // Animate unlock dismiss (required for some reason)
    .animation(.default, value: visibleScreen)
    .frame(maxWidth: .infinity, maxHeight: .infinity)
    .environment(
      \.openURL,
      .init(handler: { [openWalletURLAction] url in
        openWalletURLAction?(url)
        return .handled
      })
    )
    .environment(
      \.appRatingRequestAction,
      .init(action: { [appRatingRequestAction] in
        appRatingRequestAction?()
      })
    )
    .environment(\.webImageDownloader, webImageDownloader)
    .onChange(of: visibleScreen) { newValue in
      if case .panelUnlockOrSetup = presentingContext, newValue == .crypto {
        dismissAction()
      }
    }
  }

  private func dismissAction() {
    presentationMode.dismiss()
  }
}

private struct CryptoContainerView<DismissContent: ToolbarContent>: View {
  var keyringStore: KeyringStore
  @ObservedObject var cryptoStore: CryptoStore
  var toolbarDismissContent: DismissContent
  var selectedTab: CryptoTab
  @ToolbarContentBuilder
  // This toolbar content is for `PendingRequestView` which is presented on top of full screen wallet
  private var pendingRequestToolbarDismissContent: some ToolbarContent {
    ToolbarItemGroup(placement: .cancellationAction) {
      Button {
        cryptoStore.isPresentingPendingRequest = false
      } label: {
        Image("wallet-dismiss", bundle: .module)
          .renderingMode(.template)
          .foregroundColor(Color(.braveBlurpleTint))
      }
    }
  }

  var body: some View {
    CryptoTabsView(
      cryptoStore: cryptoStore,
      keyringStore: keyringStore,
      toolbarDismissContent: toolbarDismissContent
    )
    .background(
      Color.clear
        .sheet(item: $cryptoStore.walletActionDestination) { action in
          switch action.kind {
          case .buy:
            BuyTokenView(
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore,
              buyTokenStore: cryptoStore.openBuyTokenStore(action.initialToken),
              onDismiss: { cryptoStore.walletActionDestination = nil }
            )
          case .send:
            SendTokenView(
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore,
              sendTokenStore: cryptoStore.openSendTokenStore(action.initialToken),
              onDismiss: { cryptoStore.walletActionDestination = nil }
            )
          case .swap:
            SwapCryptoView(
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore,
              swapTokensStore: cryptoStore.openSwapTokenStore(action.initialToken),
              onDismiss: { cryptoStore.walletActionDestination = nil }
            )
          case .deposit(let query):
            DepositTokenView(
              keyringStore: keyringStore,
              networkStore: cryptoStore.networkStore,
              depositTokenStore: cryptoStore.openDepositTokenStore(
                prefilledToken: action.initialToken,
                prefilledAccount: action.initialAccount
              ),
              prefilledQuery: query,
              onDismiss: { cryptoStore.walletActionDestination = nil }
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
              toolbarDismissContent: pendingRequestToolbarDismissContent,
              onDismiss: {
                cryptoStore.isPresentingPendingRequest = false
              },
              onViewInActivity: {
                cryptoStore.selectedTab = .activity
              }
            )
            .onDisappear {
              cryptoStore.prepare()
            }
          }
        }
    )
    .environment(
      \.walletActionDestination,
      Binding(
        get: { [weak cryptoStore] in cryptoStore?.walletActionDestination },
        set: { [weak cryptoStore] destination in
          if cryptoStore?.isPresentingAssetSearch == true {
            cryptoStore?.isPresentingAssetSearch = false
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) {
              cryptoStore?.walletActionDestination = destination
            }
          } else {
            cryptoStore?.walletActionDestination = destination
          }
        }
      )
    )
  }
}
