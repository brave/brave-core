// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import LocalAuthentication
import Shared
import Combine
import Preferences
import BraveUI
import SwiftKeychainWrapper
import os.log

public class WindowProtection {

  private class LockedViewController: UIViewController {
    let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .systemThickMaterial))
    let lockImageView = UIImageView(image: UIImage(named: "browser-lock-icon", in: .module, compatibleWith: nil)!)
    let unlockButton = FilledActionButton(type: .system).then {
      $0.setTitle("Unlock", for: .normal)
      $0.titleLabel?.font = .preferredFont(forTextStyle: .headline)
      $0.titleLabel?.adjustsFontForContentSizeCategory = true
      $0.backgroundColor = .braveBlurpleTint
      $0.isHidden = true
    }

    override func viewDidLoad() {
      super.viewDidLoad()

      view.addSubview(backgroundView)
      view.addSubview(lockImageView)
      view.addSubview(unlockButton)
      backgroundView.snp.makeConstraints {
        $0.edges.equalTo(view)
      }
      lockImageView.snp.makeConstraints {
        $0.center.equalTo(view)
      }
      unlockButton.snp.makeConstraints {
        $0.leading.greaterThanOrEqualToSuperview().offset(20)
        $0.trailing.lessThanOrEqualToSuperview().offset(20)
        $0.centerX.equalToSuperview()
        $0.height.greaterThanOrEqualTo(44)
        $0.width.greaterThanOrEqualTo(230)
        $0.top.equalTo(lockImageView.snp.bottom).offset(60)
      }
    }
  }

  private let lockedViewController = LockedViewController()

  private var cancellables: Set<AnyCancellable> = []
  private var protectedWindow: UIWindow
  private var passcodeWindow: UIWindow
  private var context = LAContext()

  private var isVisible: Bool = false {
    didSet {
      passcodeWindow.isHidden = !isVisible
      if isVisible {
        passcodeWindow.makeKeyAndVisible()
      } else {
        protectedWindow.makeKeyAndVisible()
      }
    }
  }

  var isPassCodeAvailable: Bool {
    var error: NSError?
    if !context.canEvaluatePolicy(.deviceOwnerAuthentication, error: &error),
      (error as? LAError)?.code == .passcodeNotSet {
      return false
    }

    return true
  }

  public init?(window: UIWindow) {
    guard let scene = window.windowScene else { return nil }
    protectedWindow = window

    passcodeWindow = UIWindow(windowScene: scene)
    passcodeWindow.windowLevel = .init(UIWindow.Level.statusBar.rawValue + 1)
    passcodeWindow.rootViewController = lockedViewController

    lockedViewController.unlockButton.addTarget(self, action: #selector(tappedUnlock), for: .touchUpInside)

    NotificationCenter.default.publisher(for: UIApplication.didEnterBackgroundNotification)
      .sink(receiveValue: { [weak self] _ in
        guard let self = self else { return }
        // Update visibility when entering background
        self.isVisible = Preferences.Privacy.lockWithPasscode.value && self.context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil)
      })
      .store(in: &cancellables)

    NotificationCenter.default.publisher(for: UIApplication.willEnterForegroundNotification)
      .merge(with: NotificationCenter.default.publisher(for: UIApplication.didFinishLaunchingNotification))
      .sink(receiveValue: { [weak self] _ in
        guard let self = self else { return }
        self.context = LAContext()  // Reset context for new session
        self.updateVisibleStatusForForeground()
      })
      .store(in: &cancellables)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  @objc private func tappedUnlock() {
    presentLocalAuthentication()
  }

  private func updateVisibleStatusForForeground(_ determineLockWithPasscode: Bool = true, completion: ((Bool) -> Void)? = nil) {
    var error: NSError?
    if !context.canEvaluatePolicy(.deviceOwnerAuthentication, error: &error),
      (error as? LAError)?.code == .passcodeNotSet {
      // User no longer has a passcode set so we can't evaluate the auth policy
      isVisible = false
      completion?(false)
      return
    }

    if determineLockWithPasscode {
      let isLocked = Preferences.Privacy.lockWithPasscode.value
      isVisible = isLocked
      if isLocked {
        presentLocalAuthentication() { status in
          completion?(status)
        }
      }
    } else {
      isVisible = true
      presentLocalAuthentication() { status in
        completion?(status)
      }
    }
  }

  private func presentLocalAuthentication(completion: ((Bool) -> Void)? = nil) {
    if !context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil) {
      completion?(false)
      return
    }
    
    lockedViewController.unlockButton.isHidden = true
    context.evaluatePolicy(.deviceOwnerAuthentication, localizedReason: Strings.authenticationLoginsTouchReason) { success, error in
      DispatchQueue.main.async { [self] in
        if success {
          UIView.animate(
            withDuration: 0.1,
            animations: { [self] in
              lockedViewController.view.alpha = 0.0
            },
            completion: { [self] _ in
              isVisible = false
              lockedViewController.view.alpha = 1.0
              completion?(true)
            })
        } else {
          lockedViewController.unlockButton.isHidden = false
          completion?(false)
          
          if let error = error {
            Logger.module.error("Failed to unlock browser using local authentication: \(error.localizedDescription)")
          }
        }
      }
    }
  }

  func presentAuthenticationForViewController(determineLockWithPasscode: Bool = true, completion: ((Bool) -> Void)? = nil) {
    if isVisible {
      return
    }

    context = LAContext()
    updateVisibleStatusForForeground(determineLockWithPasscode) { status in
      completion?(status)
    }
  }
}
