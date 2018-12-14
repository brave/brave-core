/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import SwiftKeychainWrapper
import LocalAuthentication
import pop

/// Displays a app-wide authentication prompt if the user has a passcode enabled
class AppAuthenticator {
    /// Whether or not its currently prompting the user to authenticate
    private var isPrompting = false
    /// Whether or not the user can cancel entering the passcode
    var isPasscodeEntryCancellable: Bool
    /// A block to execute if the user cancels authenticating (assuming `isPasscodeEntryCancellable` is true)
    var didCancelPasscodeEntry: (() -> Void)?
    
    let protectedWindow: UIWindow
    
    private var _window: UIWindow?
    private var window: UIWindow? {
        get {
            if _window == nil {
                let w = UIWindow()
                w.backgroundColor = .clear
                w.rootViewController = self.blurController
                _window = w
            }
            return _window
        }
    }
    
    private let blurController = BlurController()
    
    /// Create an authenticator which protects a specific window
    init(protectedWindow: UIWindow, promptImmediately: Bool, isPasscodeEntryCancellable: Bool = true) {
        self.protectedWindow = protectedWindow
        self.isPasscodeEntryCancellable = isPasscodeEntryCancellable
        
        if promptImmediately {
            promptUserForAuthentication()
        }
    }
    
    /// Dismisses the authentication prompt
    func dismissPrompt() {
        blurController.dismiss(animated: true)
        isPrompting = false
        hideBackgroundedBlur()
    }
    
    /// Show the background blur without prompting the user
    func showBackgroundBlur() {
        guard let window = window, !window.isKeyWindow else { return }
        
        window.basicAnimate(property: kPOPViewAlpha, key: "alpha") { animation, inProgress in
            if !inProgress {
                window.alpha = 0.0
                window.makeKeyAndVisible()
            }
            animation.duration = 0.1
            animation.toValue = 1.0
            animation.completionBlock = nil
        }
    }
    
    /// Hide the background blur
    func hideBackgroundedBlur() {
        guard let window = window, window.isKeyWindow, !isPrompting else { return }
        
        window.basicAnimate(property: kPOPViewAlpha, key: "alpha") { animation, _ in
            animation.toValue = 0.0
            animation.duration = 0.1
            animation.completionBlock = { _, _ in
                self._window = nil
                self.protectedWindow.makeKeyAndVisible()
            }
        }
    }
    
    /// Prompt the user for authentication based on settings
    func promptUserForAuthentication() {
        guard let authInfo = KeychainWrapper.sharedAppContainerKeychain.authenticationInfo(), !isPrompting else { return }
        
        showBackgroundBlur()
        AppAuthenticator.presentAuthenticationUsingInfo(
            authInfo,
            touchIDReason: Strings.AuthenticationLoginsTouchReason,
            success: {
                self.dismissPrompt()
            },
            cancel: {
                if self.isPasscodeEntryCancellable {
                    self.dismissPrompt()
                    self.didCancelPasscodeEntry?()
                } else {
                    AppAuthenticator.presentPasscodeAuthentication(self.blurController, delegate: self, isCancellable: self.isPasscodeEntryCancellable)
                }
            },
            fallback: {
                AppAuthenticator.presentPasscodeAuthentication(self.blurController, delegate: self, isCancellable: self.isPasscodeEntryCancellable)
            }
        )
        isPrompting = true
    }
}

extension AppAuthenticator {
    /// A basic controller to house the background blur and present the manual passcode entry
    private class BlurController: UIViewController {
        override func viewDidLoad() {
            super.viewDidLoad()
            
            let backgroundedBlur = UIVisualEffectView(effect: UIBlurEffect(style: .light))
            view.addSubview(backgroundedBlur)
            backgroundedBlur.snp.makeConstraints { $0.edges.equalTo(self.view) }
        }
    }
}

// MARK: - Auth Presentation
extension AppAuthenticator {
    private static func presentAuthenticationUsingInfo(_ authenticationInfo: AuthenticationKeychainInfo, touchIDReason: String, success: (() -> Void)?, cancel: (() -> Void)?, fallback: (() -> Void)?) {
        if authenticationInfo.useTouchID {
            let localAuthContext = LAContext()
            localAuthContext.localizedFallbackTitle = Strings.AuthenticationEnterPasscode
            localAuthContext.evaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, localizedReason: touchIDReason) { didSucceed, error in
                if didSucceed {
                    // Update our authentication info's last validation timestamp so we don't ask again based
                    // on the set required interval
                    authenticationInfo.recordValidation()
                    KeychainWrapper.sharedAppContainerKeychain.setAuthenticationInfo(authenticationInfo)
                    DispatchQueue.main.async {
                        success?()
                    }
                    return
                }
                
                guard let authError = error,
                    let code = LAError.Code(rawValue: authError._code) else {
                        return
                }
                
                DispatchQueue.main.async {
                    switch code {
                    case .userFallback, .biometryNotEnrolled, .biometryNotAvailable, .biometryLockout:
                        fallback?()
                    case .userCancel:
                        cancel?()
                    default:
                        cancel?()
                    }
                }
            }
        } else {
            fallback?()
        }
    }
    
    private static func presentPasscodeAuthentication(_ controller: UIViewController, delegate: PasscodeEntryDelegate?, isCancellable: Bool) {
        let passcodeVC = PasscodeEntryViewController()
        passcodeVC.isCancellable = isCancellable
        passcodeVC.delegate = delegate
        let navController = UINavigationController(rootViewController: passcodeVC)
        navController.modalPresentationStyle = .formSheet
        controller.present(navController, animated: true, completion: nil)
    }
}

// MARK: - PasscodeEntryDelegate
extension AppAuthenticator: PasscodeEntryDelegate {
    func passcodeValidationDidSucceed() {
        dismissPrompt()
    }
    
    func userDidCancelValidation() {
        dismissPrompt()
        didCancelPasscodeEntry?()
    }
}
