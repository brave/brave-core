// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import LocalAuthentication
import Shared
import UIKit

public class LoadingViewController: UIViewController {

  let spinner = UIActivityIndicatorView().then {
    $0.snp.makeConstraints { make in
      make.size.equalTo(24)
    }
    $0.hidesWhenStopped = true
    $0.isHidden = true
  }

  var isLoading: Bool = false {
    didSet {
      if isLoading {
        view.addSubview(spinner)
        spinner.snp.makeConstraints {
          $0.center.equalTo(view.snp.center)
        }
        spinner.startAnimating()
      } else {
        spinner.stopAnimating()
        spinner.removeFromSuperview()
      }
    }
  }
}

public class AuthenticationController: LoadingViewController {
  let windowProtection: WindowProtection?
  let requiresAuthentication: Bool

  // MARK: Lifecycle

  init(
    windowProtection: WindowProtection? = nil,
    requiresAuthentication: Bool = false,
    isCancellable: Bool = false,
    unlockScreentitle: String = ""
  ) {
    self.windowProtection = windowProtection
    self.requiresAuthentication = requiresAuthentication

    super.init(nibName: nil, bundle: nil)

    self.windowProtection?.isCancellable = isCancellable
    self.windowProtection?.unlockScreentitle = unlockScreentitle
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  /// A method to ask biometric authentication to user
  /// - Parameter completion: block returning authentication status
  func askForAuthentication(
    viewType: AuthViewType,
    completion: ((Bool, LAError.Code?) -> Void)? = nil
  ) {
    guard let windowProtection = windowProtection else {
      completion?(false, nil)
      return
    }

    if !windowProtection.isPassCodeAvailable {
      if viewType == .tabTray {
        completion?(false, LAError.passcodeNotSet)
      } else {
        showSetPasscodeError(viewType: viewType) {
          completion?(false, LAError.passcodeNotSet)
        }
      }
    } else {
      windowProtection.presentAuthenticationForViewController(
        determineLockWithPasscode: false,
        viewType: viewType
      ) { status, error in
        completion?(status, error)
      }
    }
  }

  /// An alert presenter for passcode error to warn user to setup passcode to use feature
  /// - Parameter completion: block after Ok button is pressed
  func showSetPasscodeError(viewType: AuthViewType, completion: @escaping (() -> Void)) {
    var alertMessage: String?

    switch viewType {
    case .sync:
      alertMessage = Strings.Sync.setPasscodeAlertDescription
    case .tabTray:
      alertMessage = Strings.Privacy.tabTraySetPasscodeAlertDescription
    default:
      alertMessage = nil
    }

    let alert = UIAlertController(
      title: Strings.Sync.setPasscodeAlertTitle,
      message: alertMessage,
      preferredStyle: .alert
    )

    alert.addAction(
      UIAlertAction(
        title: Strings.OKString,
        style: .default,
        handler: { _ in
          completion()
        }
      )
    )

    present(alert, animated: true, completion: nil)
  }
}
