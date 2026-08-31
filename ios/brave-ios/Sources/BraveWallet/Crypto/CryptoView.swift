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
          .foregroundColor(Color(braveSystemName: .iconInteractive))
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
            case .pendingRequests:
              RequestContainerView(
                keyringStore: keyringStore,
                cryptoStore: store,
                toolbarDismissContent: dismissButtonToolbarContents,
                onDismiss: {
                  dismissAction()
                },
                onViewInActivity: {
                  openWalletURLAction?(.webUI.wallet.activity)
                }
              )
              .onDisappear {
                // onDisappear allows us to catch all cases (swipe, cancel, confirm/approve/sign)
                store.isPresentingPendingRequest = false
                store.prepare()
                dismissAction()
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
            case .webUI(let action):
              if action == .backup {
                NavigationView {
                  BackupWalletView(
                    password: nil,
                    keyringStore: keyringStore
                  )
                }
                .accentColor(Color(braveSystemName: .primitivePrimary40))
                .navigationViewStyle(.stack)
              } else {
                EmptyView()  // screen will be handled via `visibleScreen`
              }
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
            Group {
              if case .webUI(let action) = presentingContext,
                case .onboarding(let isNewAccount) = action
              {
                LegalView(
                  keyringStore: keyringStore,
                  setupOption: isNewAccount ? .new : .restore,
                  dismissAction: dismissAction
                )
              } else {
                SetupCryptoView(keyringStore: keyringStore, dismissAction: dismissAction)
              }
            }
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
    .onChange(of: visibleScreen) { oldValue, newValue in
      guard newValue == .crypto else { return }
      switch presentingContext {
      case .panelUnlockOrSetup, .webUI(.unlock), .webUI(.onboarding):
        // 1. wallet is unlocked from wallet panel
        // 2. wallet is unlocked from wallet webui
        // 3. onboarding is completed from wallet webui
        dismissAction()
      default:
        openWalletURLAction?(.webUI.wallet.home)
      }
    }
    .onChange(of: keyringStore.isWalletWebUIBackedUp) { oldValue, newValue in
      // back up wallet from wallet webui
      if case .webUI(let action) = presentingContext,
        action == .backup,
        !oldValue,
        newValue
      {
        dismissAction()
      }
    }
  }

  private func dismissAction() {
    presentationMode.dismiss()
  }
}
