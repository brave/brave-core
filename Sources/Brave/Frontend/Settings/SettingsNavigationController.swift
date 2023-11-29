/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit

class SettingsNavigationController: UINavigationController {
  var popoverDelegate: PresentingModalViewControllerDelegate?

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)

    if #available(iOS 16.0, *) {
      self.setNeedsUpdateOfSupportedInterfaceOrientations()
    }
  }
  
  @objc func done() {
    if let delegate = popoverDelegate {
      delegate.dismissPresentedModalViewController(self, animated: true)
    } else {
      self.dismiss(animated: true, completion: nil)
    }
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    if self.view.overrideUserInterfaceStyle == .light || self.overrideUserInterfaceStyle == .light {
      return .darkContent
    }
    return .lightContent
  }

  override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
    return .portrait
  }

  override var preferredInterfaceOrientationForPresentation: UIInterfaceOrientation {
    return .portrait
  }
}

protocol PresentingModalViewControllerDelegate {
  func dismissPresentedModalViewController(_ modalViewController: UIViewController, animated: Bool)
}

class ModalSettingsNavigationController: UINavigationController {
  override var preferredStatusBarStyle: UIStatusBarStyle {
    return .default
  }
}
