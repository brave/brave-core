// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveVPN
import UIKit

extension BrowserViewController {
  public func vpnSessionExpiredStateAlert(
    loginCallback: @escaping (UIAlertAction) -> Void
  ) -> UIAlertController {
    let alert = UIAlertController(
      title: Strings.VPN.sessionExpiredTitle,
      message: Strings.VPN.sessionExpiredDescription,
      preferredStyle: .alert
    )

    let loginButton = UIAlertAction(
      title: Strings.VPN.sessionExpiredLoginButton,
      style: .default,
      handler: loginCallback
    )
    let dismissButton = UIAlertAction(
      title: Strings.VPN.sessionExpiredDismissButton,
      style: .cancel
    )
    alert.addAction(loginButton)
    alert.addAction(dismissButton)

    return alert
  }

  /// Shows a vpn screen based on vpn state.
  public func presentCorrespondingVPNViewController() {
    if BraveVPN.isSkusCredentialSessionExpired {
      let alert = vpnSessionExpiredStateAlert(loginCallback: { [unowned self] _ in
        self.openURLInNewTab(
          .brave.account,
          isPrivate: self.privateBrowsingManager.isPrivateBrowsing,
          isPrivileged: false
        )
      })

      present(alert, animated: true)
      return
    }

    guard BraveVPN.vpnState.isPaywallEnabled else { return }

    let vpnPaywallView = BraveVPNPaywallView(
      openVPNAuthenticationInNewTab: { [weak self] in
        guard let self = self else { return }

        self.popToBVC()

        self.openURLInNewTab(
          .brave.braveVPNRefreshCredentials,
          isPrivate: self.privateBrowsingManager.isPrivateBrowsing,
          isPrivileged: false
        )
      },
      openDirectCheckoutInNewTab: { [weak self] in
        guard let self else { return }
        popToBVC()
        openURLInNewTab(
          .brave.braveVPNCheckoutURL,
          isPrivate: self.privateBrowsingManager.isPrivateBrowsing,
          isPrivileged: false
        )
      },
      openLearnMoreInNewTab: { [weak self] in
        guard let self else { return }
        popToBVC()
        openURLInNewTab(
          .brave.braveVPNLearnMoreURL,
          isPrivate: self.privateBrowsingManager.isPrivateBrowsing,
          isPrivileged: false
        )
      },
      installVPNProfile: { [weak self] in
        guard let self = self else { return }
        self.dismiss(animated: true) {
          self.present(BraveVPNInstallViewController(), animated: true)
        }
      }
    )
    let vpnPaywallHostingVC = BraveVPNPaywallHostingController(paywallView: vpnPaywallView)
    popToBVC(isAnimated: true) { [weak self] in
      self?.present(UINavigationController(rootViewController: vpnPaywallHostingVC), animated: true)
    }
  }
}
