// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import SwiftUI
import UIKit

extension View {
  /// Helper for `hidden()` modifier that accepts a boolean determining if we should hide the view or not.
  @ViewBuilder public func hidden(isHidden: Bool) -> some View {
    self  // use opacity to avoid identity resets when not required
      .opacity(isHidden ? 0 : 1)
      .accessibilityHidden(isHidden)
  }

  /// This function will use the help from `introspectViewController` to find the
  /// containing `UIViewController` of the current SwiftUI view and configure its navigation
  /// bar appearance to be transparent.
  /// It also takes two parameters to custom the back button title and back button display mode.
  public func transparentNavigationBar(
    backButtonTitle: String? = nil,
    backButtonDisplayMode: UINavigationItem.BackButtonDisplayMode = .default
  ) -> some View {
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

  func applyRegularNavigationAppearance() -> some View {
    introspectViewController(customize: { vc in
      vc.navigationItem.do {
        let appearance: UINavigationBarAppearance = {
          let appearance = UINavigationBarAppearance()
          appearance.configureWithOpaqueBackground()
          appearance.titleTextAttributes = [.foregroundColor: UIColor.braveLabel]
          appearance.largeTitleTextAttributes = [.foregroundColor: UIColor.braveLabel]
          appearance.backgroundColor = .braveBackground
          return appearance
        }()
        $0.standardAppearance = appearance
        $0.compactAppearance = appearance
        $0.scrollEdgeAppearance = appearance
      }
    })
  }

  func transparentUnlessScrolledNavigationAppearance() -> some View {
    introspectViewController(customize: { vc in
      vc.navigationItem.do {
        // no shadow when content is at top.
        let noShadowAppearance: UINavigationBarAppearance = {
          let appearance = UINavigationBarAppearance()
          appearance.configureWithTransparentBackground()
          appearance.titleTextAttributes = [.foregroundColor: UIColor.braveLabel]
          appearance.largeTitleTextAttributes = [.foregroundColor: UIColor.braveLabel]
          appearance.backgroundColor = .clear
          appearance.shadowColor = .clear
          return appearance
        }()
        $0.scrollEdgeAppearance = noShadowAppearance
        $0.compactScrollEdgeAppearance = noShadowAppearance
        // shadow when content is scrolled behind navigation bar.
        let shadowAppearance: UINavigationBarAppearance = {
          let appearance = UINavigationBarAppearance()
          appearance.configureWithOpaqueBackground()
          appearance.titleTextAttributes = [.foregroundColor: UIColor.braveLabel]
          appearance.largeTitleTextAttributes = [.foregroundColor: UIColor.braveLabel]
          return appearance
        }()
        $0.standardAppearance = shadowAppearance
        $0.compactAppearance = shadowAppearance
      }
    })
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
            title: Text(
              String.localizedStringWithFormat(
                Strings.Wallet.createAccountAlertTitle,
                accountNetwork?.shortChainName ?? ""
              )
            ),
            message: Text(Strings.Wallet.createAccountAlertMessage),
            primaryButton: .default(
              Text(Strings.yes),
              action: {
                onConfirmAddAccount()
              }
            ),
            secondaryButton: .cancel(
              Text(Strings.no),
              action: {
                onCancelAddAccount?()
              }
            )
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
              preSelectedAccountNetwork: accountNetwork
            )
          }
          .navigationViewStyle(.stack)
          .onDisappear { onAddAccountDismissed() }
        }
    )
  }

  func errorAlert(errorMessage: Binding<String?>) -> some View {
    alert(
      isPresented: Binding(
        get: { errorMessage.wrappedValue != nil },
        set: { _, _ in
          errorMessage.wrappedValue = nil
        }
      )
    ) {
      Alert(
        title: Text(Strings.Wallet.errorAlertTitle),
        message: Text(errorMessage.wrappedValue ?? Strings.Wallet.unknownError),
        dismissButton: .default(Text(Strings.OKString))
      )
    }
  }
}
