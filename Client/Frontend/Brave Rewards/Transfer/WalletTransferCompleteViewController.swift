// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Shared
import BraveRewards
import BraveShared

class WalletTransferCompleteViewController: UIViewController, Themeable {
    
    private let status: DrainStatus?
    
    init(status: DrainStatus?) {
        self.status = status
        super.init(nibName: nil, bundle: nil)
        if status == .complete {
            // Require user to acknowledge by tapping done
            isModalInPresentation = true
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    private var transferCompleteView: WalletTransferCompleteView {
        view as! WalletTransferCompleteView // swiftlint:disable:this force_cast
    }
    
    override func loadView() {
        view = WalletTransferCompleteView()
        applyTheme(Theme.of(nil))
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        navigationItem.hidesBackButton = true
        navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .done, target: self, action: #selector(tappedDone))
        title = Strings.Rewards.walletTransferTitle
        
        if let status = status {
            transferCompleteView.titleLabel.text = status.transferStatusTitle
            transferCompleteView.bodyLabel.text = status.transferStatusBody
        }
    }
    
    override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
        super.traitCollectionDidChange(previousTraitCollection)
        
        if traitCollection.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle {
            applyTheme(Theme.of(nil))
        }
    }
    
    func applyTheme(_ theme: Theme) {
        transferCompleteView.applyTheme(theme)
    }
    
    @objc private func tappedDone() {
        if status == .complete {
            Preferences.Rewards.transferCompletionAcknowledged.value = true
        }
        dismiss(animated: true)
    }
}
