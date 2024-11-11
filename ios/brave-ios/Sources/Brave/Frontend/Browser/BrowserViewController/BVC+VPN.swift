// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveVPN
import UIKit

extension BrowserViewController: BraveVPNPaywallHostingControllerDelegate {

  public func deviceOrientationChanged() {
    if let menuVC = presentedViewController as? MenuViewController {
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

      let vpnPaywallHostingVC = BraveVPNPaywallHostingController(paywallView: vpnPaywallView).then {
        $0.delegate = self
      }
      if UIDevice.current.userInterfaceIdiom == .pad
        && UIDevice.current.orientation.isPortrait
      {
        vpnPaywallHostingVC.title = Strings.VPN.vpnName
        menuVC.presentInnerMenu(vpnPaywallHostingVC)
      } else {
        let navigationController = UINavigationController(
          rootViewController: vpnPaywallHostingVC
        )
        if UIDevice.current.userInterfaceIdiom == .pad {
          if UIDevice.current.orientation.isLandscape {
            navigationController.modalPresentationStyle = .fullScreen
          }
          menuVC.present(navigationController, animated: true)
        } else {
          self.dismiss(animated: true) {
            self.present(navigationController, animated: true)
          }
        }
      }
    } else {
      presentCorrespondingVPNViewController()
    }
  }
}
