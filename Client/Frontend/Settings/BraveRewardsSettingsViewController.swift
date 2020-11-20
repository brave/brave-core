// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static
import Shared
import BraveShared
import BraveRewards
import BraveUI
import DeviceCheck

class BraveRewardsSettingsViewController: TableViewController {
    
    let rewards: BraveRewards
    let legacyWallet: BraveLedger?
    var walletTransferLearnMoreTapped: (() -> Void)?
    
    init(_ rewards: BraveRewards, legacyWallet: BraveLedger?) {
        self.rewards = rewards
        self.legacyWallet = legacyWallet
        
        if #available(iOS 13.0, *) {
            super.init(style: .insetGrouped)
        } else {
            super.init(style: .grouped)
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = Strings.braveRewardsTitle
        
        dataSource.sections = [
            .init(
                rows: [
                    Row(text: Strings.Rewards.settingsToggleTitle,
                        detailText: Strings.Rewards.settingsToggleMessage,
                        accessory: .switchToggle(value: rewards.isEnabled, { [unowned self] isOn in
                            self.rewards.isEnabled = isOn
                        }),
                        cellClass: MultilineSubtitleCell.self)
                ],
                footer: .title(Strings.Rewards.settingsFooterMessage)
            )
        ]
        
        if let legacyWallet = legacyWallet {
            legacyWallet.transferrableAmount({ [weak self] total in
                guard let self = self, total > 0 else { return }
                self.dataSource.sections.insert(.init(
                    header: .title(Strings.Rewards.walletTransferTitle),
                    rows: [
                        Row(text: Strings.Rewards.legacyWalletTransfer, selection: { [unowned self] in
                            guard let legacyWallet = self.legacyWallet else { return }
                            let controller = WalletTransferViewController(legacyWallet: legacyWallet)
                            controller.learnMoreHandler = { [weak self] in
                                self?.walletTransferLearnMoreTapped?()
                            }
                            let container = UINavigationController(rootViewController: controller)
                            container.modalPresentationStyle = .formSheet
                            self.present(container, animated: true)
                        }, image: UIImage(imageLiteralResourceName: "rewards-qr-code").template)
                    ]
                ), at: 1)
            })
        }
        
        if rewards.ledger.isWalletCreated {
            dataSource.sections += [
                Section(rows: [
                    Row(text: Strings.RewardsInternals.title, selection: {
                        let controller = RewardsInternalsViewController(ledger: self.rewards.ledger)
                        self.navigationController?.pushViewController(controller, animated: true)
                    }, accessory: .disclosureIndicator)
                ])
            ]
        }
    }
}
