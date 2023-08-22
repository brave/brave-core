// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import BraveUI
import Shared
import UIKit
import LocalAuthentication
import Combine

class LoginAuthViewController: UITableViewController {

  private let windowProtection: WindowProtection?
  private var localAuthObservers = Set<AnyCancellable>()

  // MARK: Lifecycle

  init(windowProtection: WindowProtection?, requiresAuthentication: Bool = false) {
    self.windowProtection = windowProtection
    self.requiresAuthentication = requiresAuthentication
    super.init(nibName: nil, bundle: nil)
    
    windowProtection?.isCancellable = true
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)

    if requiresAuthentication, Preferences.Privacy.lockWithPasscode.value {
      askForAuthentication() { [weak self] success, error in
        if !success, error != .userCancel {
          self?.navigationController?.popViewController(animated: true)
        }
      }
    }
    
    NotificationCenter.default.do {
      $0.addObserver(
        self, selector: #selector(removeBackgroundedBlur),
        name: UIApplication.willEnterForegroundNotification, object: nil)
      $0.addObserver(
        self, selector: #selector(removeBackgroundedBlur),
        name: UIApplication.didBecomeActiveNotification, object: nil)
      $0.addObserver(
        self, selector: #selector(blurContents),
        name: UIApplication.willResignActiveNotification, object: nil)
      $0.addObserver(
        self, selector: #selector(blurContents),
        name: UIApplication.didEnterBackgroundNotification, object: nil)
    }
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  // MARK: Internal

  func askForAuthentication(completion: ((Bool, LAError.Code?) -> Void)? = nil) {
    guard let windowProtection = windowProtection else {
      completion?(false, nil)
      return
    }

    if !windowProtection.isPassCodeAvailable {
      showSetPasscodeError() {
        completion?(false, .passcodeNotSet)
      }
    } else {
      windowProtection.presentAuthenticationForViewController(determineLockWithPasscode: false, viewType: .passwords) { status, error in
        completion?(status, error)
      }
    }
  }

  func showSetPasscodeError(completion: @escaping (() -> Void)) {
    let alert = UIAlertController(
      title: Strings.Login.loginInfoSetPasscodeAlertTitle,
      message: Strings.Login.loginInfoSetPasscodeAlertDescription,
      preferredStyle: .alert)

    alert.addAction(
      UIAlertAction(title: Strings.OKString, style: .default, handler: { _ in
          completion()
      })
    )
    
    present(alert, animated: true, completion: nil)
  }

  // MARK: Private

  private var blurredSnapshotView: UIView?
  private let requiresAuthentication: Bool

  @objc private func blurContents() {
    if blurredSnapshotView == nil {
      blurredSnapshotView = createBlurredContentView()
    }
  }

  @objc private func removeBackgroundedBlur() {
    blurredSnapshotView?.removeFromSuperview()
    blurredSnapshotView = nil
  }

  private func createBlurredContentView() -> UIView? {
    guard let snapshot = view.screenshot() else {
      return nil
    }

    let blurContentView = UIView(frame: view.frame)
    let snapshotImageView = UIImageView(image: snapshot)
    let blurVisualEffectView = UIVisualEffectView(effect: UIBlurEffect(style: .systemUltraThinMaterialDark))

    view.addSubview(blurContentView)

    blurContentView.do {
      $0.snp.makeConstraints { $0.edges.equalToSuperview() }
      $0.addSubview(snapshotImageView)
      $0.addSubview(blurVisualEffectView)
    }

    snapshotImageView.snp.makeConstraints { $0.edges.equalToSuperview() }
    blurVisualEffectView.snp.makeConstraints { $0.edges.equalToSuperview() }

    view.layoutIfNeeded()

    return blurContentView
  }
}
