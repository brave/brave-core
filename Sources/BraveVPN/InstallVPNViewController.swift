// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import Lottie
import BraveUI
import GuardianConnect

class InstallVPNViewController: VPNSetupLoadingController {

  private var installVPNView: View {
    return view as! View  // swiftlint:disable:this force_cast
  }
  
  private var installVPNProfileRetryCount = 0

  override func loadView() {
    view = View()
  }

  @objc func dismissView() {
    dismiss(animated: true)
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    title = Strings.VPN.installTitle
    installVPNView.installVPNButton.addTarget(self, action: #selector(installVPNAction), for: .touchUpInside)
    installVPNView.contactSupportButton.addTarget(self, action: #selector(contactSupportAction), for: .touchUpInside)
    navigationItem.setLeftBarButton(.init(barButtonSystemItem: .cancel, target: self, action: #selector(dismissView)), animated: true)

    navigationItem.standardAppearance = BraveVPNCommonUI.navigationBarAppearance
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    // navigationItem.standardAppearance does not support tinting the back button for some
    // reason, so we still must apply a custom tint to the bar
    navigationController?.navigationBar.tintColor = .white
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)

    // Reset styling set above
    navigationController?.navigationBar.tintColor = UINavigationBar.appearance().tintColor
  }

  @objc func installVPNAction() {
    isLoading = true
    
    // Used to set whether our current user is actively a paying customer
    // This has to be set before adding the profile otherwise it might fail
    GRDSubscriptionManager.setIsPayingUser(true)
    
    installProfileAndConnectVPNFirstTime { [weak self] status in
      guard let self else { return }
      
      if status {
        self.dismiss(animated: true) {
          self.showSuccessAlert()
        }
      } else {
        // Retry installing profile twice if it fails
        // Error generated is WireGuard capabilities are not yet enabled on this node
        if self.installVPNProfileRetryCount < 2 {
          installVPNAction()
        } else {
          presentErrorVPNInstallProfile()
        }
      }
    }
 
    func presentErrorVPNInstallProfile() {
      let alert = UIAlertController(title: Strings.VPN.vpnConfigGenericErrorTitle,
                                    message: Strings.VPN.vpnConfigGenericErrorBody,
                                    preferredStyle: .alert)
      
      alert.addAction(UIAlertAction(title: Strings.OKString, style: .default))
      
      DispatchQueue.main.async {
        self.present(alert, animated: true)
      }
    }
  }
  
  private func installProfileAndConnectVPNFirstTime(completion: @escaping (Bool) -> Void) {
    installVPNProfileRetryCount += 1
    
    BraveVPN.connectToVPN() { [weak self] status in
      DispatchQueue.main.async {
        self?.isLoading = false
      }

      completion(status)
    }
  }

  @objc private func contactSupportAction() {
    navigationController?.pushViewController(BraveVPNContactFormViewController(), animated: true)
  }

  private func showSuccessAlert() {
      let animation = AnimationView(name: "vpncheckmark", bundle: .module).then {
      $0.bounds = CGRect(x: 0, y: 0, width: 300, height: 200)
      $0.contentMode = .scaleAspectFill
      $0.play()
    }

    let popup = AlertPopupView(imageView: animation,
                                    title: Strings.VPN.installSuccessPopup, message: "",
                                    titleWeight: .semibold, titleSize: 18,
                                    dismissHandler: { true })
    
    popup.showWithType(showType: .flyUp, autoDismissTime: 1.5)
  }
}
