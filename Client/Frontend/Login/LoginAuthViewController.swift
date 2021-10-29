// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveShared
import BraveUI
import Shared

class LoginAuthViewController: UITableViewController {
        
    private let windowProtection: WindowProtection?

    // MARK: Lifecycle
    
    init(windowProtection: WindowProtection?, requiresAuthentication: Bool = false) {
        self.windowProtection = windowProtection
        self.requiresAuthentication = requiresAuthentication
        super.init(nibName: nil, bundle: nil)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    override func viewDidLoad() {
        if requiresAuthentication, Preferences.Privacy.lockWithPasscode.value {
            askForAuthentication()
        }
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        NotificationCenter.default.do {
            $0.addObserver(self, selector: #selector(removeBackgroundedBlur),
                           name: UIApplication.willEnterForegroundNotification, object: nil)
            $0.addObserver(self, selector: #selector(removeBackgroundedBlur),
                           name: UIApplication.didBecomeActiveNotification, object: nil)
            $0.addObserver(self, selector: #selector(blurContents),
                           name: UIApplication.willResignActiveNotification, object: nil)
            $0.addObserver(self, selector: #selector(blurContents),
                           name: UIApplication.didEnterBackgroundNotification, object: nil)
        }
    }

    deinit {
        NotificationCenter.default.removeObserver(self)
    }
    
    // MARK: Internal
    
    @discardableResult
    func askForAuthentication() -> Bool {
        guard let windowProtection = windowProtection else {
            return false
        }

        if !windowProtection.isPassCodeAvailable {
            showSetPasscodeError()
            return false
        } else {
            windowProtection.presentAuthenticationForViewController(determineLockWithPasscode: false)
            return true
        }
    }

    func showSetPasscodeError() {
        let alert = UIAlertController(
            title: Strings.Login.loginInfoSetPasscodeAlertTitle,
            message: Strings.Login.loginInfoSetPasscodeAlertDescription,
            preferredStyle: .alert)
        
        alert.addAction(UIAlertAction(title: Strings.OKString, style: .default, handler: nil))
        
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
