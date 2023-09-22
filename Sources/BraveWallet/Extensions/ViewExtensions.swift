// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import SwiftUI
import UIKit
import BraveCore

extension View {
  /// Helper for `hidden()` modifier that accepts a boolean determining if we should hide the view or not.
  @ViewBuilder public func hidden(isHidden: Bool) -> some View {
    if isHidden {
      self.hidden()
    } else {
      self
    }
  }
  /// This function will use the help from `introspectViewController` to find the
  /// containing `UIViewController` of the current SwiftUI view and configure its navigation
  /// bar appearance to be transparent.
  /// It also takes two parameters to custom the back button title and back button display mode.
  public func transparentNavigationBar(backButtonTitle: String? = nil, backButtonDisplayMode: UINavigationItem.BackButtonDisplayMode = .default) -> some View {
    self.introspectViewController { vc in
      let appearance = UINavigationBarAppearance()
      appearance.configureWithTransparentBackground()
      vc.navigationItem.compactAppearance = appearance
      vc.navigationItem.scrollEdgeAppearance = appearance
      vc.navigationItem.standardAppearance = appearance
      vc.navigationItem.backButtonTitle = backButtonTitle
      vc.navigationItem.backButtonDisplayMode = backButtonDisplayMode
    }
  }
  
  func addAccount(
    keyringStore: KeyringStore,
    networkStore: NetworkStore,
    accountNetwork: BraveWallet.NetworkInfo?,
    isShowingConfirmation: Binding<Bool>,
    isShowingAddAccount: Binding<Bool>,
    onConfirmAddAccount: @escaping () -> Void,
    onCancelAddAccount: (() -> Void)?,
    onAddAccountDismissed: @escaping () -> Void
  ) -> some View {
    self.background(
      Color.clear
        .alert(isPresented: isShowingConfirmation) {
          Alert(
            title: Text(String.localizedStringWithFormat(Strings.Wallet.createAccountAlertTitle, accountNetwork?.shortChainName ?? "")),
            message: Text(Strings.Wallet.createAccountAlertMessage),
            primaryButton: .default(Text(Strings.yes), action: {
              onConfirmAddAccount()
            }),
            secondaryButton: .cancel(Text(Strings.no), action: {
              onCancelAddAccount?()
            })
          )
        }
    )
    .background(
      Color.clear
        .sheet(
          isPresented: isShowingAddAccount
        ) {
          NavigationView {
            AddAccountView(
              keyringStore: keyringStore,
              networkStore: networkStore,
              preSelectedCoin: accountNetwork?.coin,
              preSelectedFilecoinNetwork: accountNetwork
            )
          }
          .navigationViewStyle(.stack)
          .onDisappear { onAddAccountDismissed() }
        }
    )
  }
}
