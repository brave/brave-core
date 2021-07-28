// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import UIKit
import LocalAuthentication
import Shared
import Combine
import BraveShared
import BraveUI
import SwiftKeychainWrapper

private let log = Logger.browserLogger

class WindowProtection {
    
    private class LockedViewController: UIViewController {
        let backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .systemThickMaterial))
        let lockImageView = UIImageView(image: UIImage(imageLiteralResourceName: "browser-lock-icon"))
        let unlockButton = FilledActionButton(type: .system).then {
            $0.setTitle("Unlock", for: .normal)
            $0.titleLabel?.font = .preferredFont(forTextStyle: .headline)
            $0.titleLabel?.adjustsFontForContentSizeCategory = true
            $0.backgroundColor = .braveBlurple
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
    
    init?(window: UIWindow) {
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
                self.isVisible = Preferences.Privacy.lockWithPasscode.value &&
                    self.context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil)
            })
            .store(in: &cancellables)
        
        NotificationCenter.default.publisher(for: UIApplication.willEnterForegroundNotification)
            .merge(with: NotificationCenter.default.publisher(for: UIApplication.didFinishLaunchingNotification))
            .sink(receiveValue: { [weak self] _ in
                guard let self = self else { return }
                self.context = LAContext() // Reset context for new session
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
    
    private func updateVisibleStatusForForeground() {
        var error: NSError?
        if !context.canEvaluatePolicy(.deviceOwnerAuthentication, error: &error),
           (error as? LAError)?.code == .passcodeNotSet {
            // User no longer has a passcode set so we can't evaluate the auth policy
            isVisible = false
            return
        }
        let isLocked = Preferences.Privacy.lockWithPasscode.value
        isVisible = isLocked
        if isLocked {
            presentLocalAuthentication()
        }
    }
    
    private func presentLocalAuthentication() {
        if !context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil) {
            return
        }
        lockedViewController.unlockButton.isHidden = true
        context.evaluatePolicy(.deviceOwnerAuthentication, localizedReason: Strings.authenticationLoginsTouchReason) { success, error in
            DispatchQueue.main.async { [self] in
                if success {
                    UIView.animate(withDuration: 0.1, animations: {
                        lockedViewController.view.alpha = 0.0
                    }, completion: { _ in
                        isVisible = false
                        lockedViewController.view.alpha = 1.0
                    })
                } else {
                    lockedViewController.unlockButton.isHidden = false
                    if let error = error {
                        log.error("Failed to unlock browser using local authentication: \(error.localizedDescription)")
                    }
                }
            }
        }
    }
}
