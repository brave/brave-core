// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveVPN
import UIKit

extension BrowserViewController {
  /// Shows a vpn screen based on vpn state.
  public func presentCorrespondingVPNViewController() {
    if BraveSkusManager.keepShowingSessionExpiredState {
      let alert = BraveSkusManager.sessionExpiredStateAlert(loginCallback: { [unowned self] _ in
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
      installVPNProfile: { [weak self] in
        guard let self = self else { return }
        self.dismiss(animated: true) {
          self.present(BraveVPNInstallViewController(), animated: true)
        }
      }
    )
    let vpnPaywallHostingVC = BraveVPNPaywallHostingController(paywallView: vpnPaywallView)
    if UIDevice.current.userInterfaceIdiom == .pad {
      (presentedViewController as? MenuViewController)?.presentInnerMenu(vpnPaywallHostingVC)
    } else {
      present(UINavigationController(rootViewController: vpnPaywallHostingVC), animated: true)
    }
  }
}
