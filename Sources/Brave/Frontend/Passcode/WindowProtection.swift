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
import os.log

public enum AuthViewType {
  case external, general, sync, tabTray, passwords
}

public class WindowProtection {

  private class LockedViewController: UIViewController {
    let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .systemThickMaterial))
    let lockImageView = UIImageView(image: UIImage(named: "browser-lock-icon", in: .module, compatibleWith: nil)!)
    let titleLabel = UILabel().then {
      $0.font = .preferredFont(for: .title3, weight: .semibold)
      $0.adjustsFontForContentSizeCategory = true
      $0.textColor = .bravePrimary
      $0.numberOfLines = 0
      $0.textAlignment = .center
    }
    let unlockButton = FilledActionButton(type: .system).then {
      $0.setTitle(Strings.unlockButtonTitle, for: .normal)
      $0.titleLabel?.font = .preferredFont(forTextStyle: .headline)
      $0.titleLabel?.adjustsFontForContentSizeCategory = true
      $0.backgroundColor = .braveBlurpleTint
      $0.isHidden = true
    }
    let cancelButton = ActionButton(type: .system).then {
      $0.setTitle(Strings.cancelButtonTitle, for: .normal)
      $0.titleLabel?.font = .preferredFont(forTextStyle: .headline)
      $0.titleLabel?.adjustsFontForContentSizeCategory = true
      $0.setTitle(Strings.cancelButtonTitle, for: .normal)
      $0.tintColor = .braveLabel
      $0.isHidden = true
    }

    override func viewDidLoad() {
      super.viewDidLoad()

      view.addSubview(backgroundView)
      view.addSubview(titleLabel)
      view.addSubview(lockImageView)
      view.addSubview(unlockButton)
      view.addSubview(cancelButton)
      backgroundView.snp.makeConstraints {
        $0.edges.equalTo(view)
      }
      titleLabel.snp.makeConstraints {
        $0.leading.greaterThanOrEqualToSuperview().offset(20)
        $0.trailing.lessThanOrEqualToSuperview().offset(-20)
        $0.centerX.equalToSuperview()
        $0.bottom.equalTo(lockImageView.snp.top).offset(-40)
      }
      lockImageView.snp.makeConstraints {
        $0.center.equalTo(view)
      }
      unlockButton.snp.makeConstraints {
        $0.leading.greaterThanOrEqualToSuperview().offset(20)
        $0.trailing.lessThanOrEqualToSuperview().offset(-20)
        $0.centerX.equalToSuperview()
        $0.height.greaterThanOrEqualTo(44)
        $0.width.greaterThanOrEqualTo(230)
        $0.top.equalTo(lockImageView.snp.bottom).offset(60)
      }
      cancelButton.snp.makeConstraints {
        $0.leading.greaterThanOrEqualToSuperview().offset(20)
        $0.trailing.lessThanOrEqualToSuperview().offset(-20)
        $0.centerX.equalToSuperview()
        $0.height.greaterThanOrEqualTo(44)
        $0.width.greaterThanOrEqualTo(230)
        $0.top.equalTo(unlockButton.snp.bottom).offset(15)
      }
    }
  }

  private let lockedViewController = LockedViewController()

  private var cancellables: Set<AnyCancellable> = []
  private var protectedWindow: UIWindow
  private var passcodeWindow: UIWindow
  private var context = LAContext()
  private var viewType: AuthViewType = .general

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

  var isCancellable: Bool = false {
    didSet {
      if oldValue != isCancellable {
        lockedViewController.cancelButton.isHidden = !isCancellable
      }
    }
  }
  
  var unlockScreentitle: String = "" {
    didSet {
      lockedViewController.titleLabel.isHidden = unlockScreentitle.isEmpty
      lockedViewController.titleLabel.text = unlockScreentitle
    }
  }
  
  private let onCancelPressed = PassthroughSubject<Void, Never>()
  private let didFinalizeAuthentication = PassthroughSubject<(Bool, AuthViewType), Never>()

  var cancelPressed: AnyPublisher<Void, Never> {
    onCancelPressed.eraseToAnyPublisher()
  }
  
  var finalizedAuthentication: AnyPublisher<(Bool, AuthViewType), Never> {
    didFinalizeAuthentication.eraseToAnyPublisher()
  }
    
  public init?(window: UIWindow) {
    guard let scene = window.windowScene else { return nil }
    protectedWindow = window

    passcodeWindow = UIWindow(windowScene: scene)
    passcodeWindow.windowLevel = .init(UIWindow.Level.statusBar.rawValue + 1)
    passcodeWindow.rootViewController = lockedViewController

    lockedViewController.unlockButton.addTarget(self, action: #selector(tappedUnlock), for: .touchUpInside)
    lockedViewController.cancelButton.addTarget(self, action: #selector(tappedCancel), for: .touchUpInside)
    
    NotificationCenter.default.publisher(for: UIApplication.didEnterBackgroundNotification)
      .sink(receiveValue: { [weak self] _ in
        guard let self = self else { return }
        // Should set as non cancallable for browser lock
        self.isCancellable = false
        // Update visibility when entering background
        self.isVisible = Preferences.Privacy.lockWithPasscode.value && self.context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil)
      })
      .store(in: &cancellables)

    NotificationCenter.default.publisher(for: UIApplication.willEnterForegroundNotification)
      .merge(with: NotificationCenter.default.publisher(for: UIApplication.didFinishLaunchingNotification))
      .sink(receiveValue: { [weak self] _ in
        guard let self = self else { return }
        self.context = LAContext()  // Reset context for new session
        self.updateVisibleStatusForForeground(viewType: .external)
      })
      .store(in: &cancellables)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  @objc private func tappedUnlock() {
    presentLocalAuthentication(viewType: viewType)
  }
  
  @objc private func tappedCancel() {
    onCancelPressed.send(())
    isVisible = false
  }

  private func updateVisibleStatusForForeground(_ determineLockWithPasscode: Bool = true, viewType: AuthViewType = .general, completion: ((Bool, LAError.Code?) -> Void)? = nil) {
    var error: NSError?
    if !context.canEvaluatePolicy(.deviceOwnerAuthentication, error: &error),
      (error as? LAError)?.code == .passcodeNotSet {
      // User no longer has a passcode set so we can't evaluate the auth policy
      isVisible = false
      completion?(false, .passcodeNotSet)
      return
    }

    if determineLockWithPasscode {
      let isLocked = Preferences.Privacy.lockWithPasscode.value
      isVisible = isLocked
      if isLocked {
        presentLocalAuthentication(viewType: viewType) { status, error in
          completion?(status, error)
        }
      }
    } else {
      isVisible = true
      presentLocalAuthentication(viewType: viewType) { status, error in
        completion?(status, error)
      }
    }
  }

  private func presentLocalAuthentication(viewType: AuthViewType, completion: ((Bool, LAError.Code?) -> Void)? = nil) {
    self.viewType = viewType

    if !context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil) {
      completion?(false, .passcodeNotSet)
      return
    }
    
    lockedViewController.unlockButton.isHidden = true
    if viewType == .external {
      isCancellable = false
    }
    
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
              completion?(true, nil)
            })
        } else {
          lockedViewController.unlockButton.isHidden = viewType == .general

          let errorPolicy = error as? LAError
          completion?(false, errorPolicy?.code)
          
          if let error = error {
            Logger.module.error("Failed to unlock browser using local authentication: \(error.localizedDescription)")
          }
        }
        
        self.didFinalizeAuthentication.send((success, viewType))
      }
    }
  }

  func presentAuthenticationForViewController(determineLockWithPasscode: Bool = true, viewType: AuthViewType, completion: ((Bool, LAError.Code?) -> Void)? = nil) {
    if isVisible {
      return
    }
    
    self.viewType = viewType

    context = LAContext()
    updateVisibleStatusForForeground(determineLockWithPasscode, viewType: viewType) { status, error in
      completion?(status, error)
    }
  }
}
