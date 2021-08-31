// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static
import Shared
import BraveShared
import BraveCore
import BraveUI
import DeviceCheck
import Combine

class BraveRewardsSettingsViewController: TableViewController {
    
    let rewards: BraveRewards
    let legacyWallet: BraveLedger?
    var walletTransferLearnMoreTapped: (() -> Void)?
    private var prefsCancellable: AnyCancellable?
    
    init(_ rewards: BraveRewards, legacyWallet: BraveLedger?) {
        self.rewards = rewards
        self.legacyWallet = legacyWallet
        
        super.init(style: .insetGrouped)
        
        prefsCancellable = Preferences.Rewards.transferCompletionAcknowledged
            .objectWillChange
            .receive(on: RunLoop.main)
            .sink(receiveValue: { [weak self] in
                self?.reloadSections()
            })
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    private func reloadSections() {
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
            if Preferences.Rewards.transferDrainID.value == nil {
                if !legacyWallet.isLedgerTransferExpired {
                    legacyWallet.transferrableAmount({ [weak self] total in
                        guard let self = self, total > 0 else { return }
                        self.dataSource.sections.insert(.init(
                            header: .title(Strings.Rewards.walletTransferTitle),
                            rows: [
                                Row(text: Strings.Rewards.legacyWalletTransfer,
                                    detailText: Preferences.Rewards.lastTransferStatus.value.map(Ledger.DrainStatus.init)??.displayString,
                                    selection: { [unowned self] in
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
            } else {
                // Check to see if the transfer already completed and was acknowledged by the user,
                // so we dont show the row for wallet transfer anymore
                if !Preferences.Rewards.transferCompletionAcknowledged.value {
                    legacyWallet.updateDrainStatus { status in
                        if let status = status {
                            self.dataSource.sections.insert(.init(
                                header: .title(Strings.Rewards.walletTransferTitle),
                                rows: [
                                    Row(text: Strings.Rewards.legacyWalletTransfer,
                                        detailText: status.displayString,
                                        selection: { [unowned self] in
                                            let controller = WalletTransferCompleteViewController(status: status)
                                            let container = UINavigationController(rootViewController: controller)
                                            container.modalPresentationStyle = .formSheet
                                            self.present(container, animated: true)
                                        }, image: UIImage(imageLiteralResourceName: "rewards-qr-code").template)
                                ]
                            ), at: 1)
                        }
                    }
                }
            }
        }
        
        if let ledger = rewards.ledger {
            ledger.rewardsInternalInfo { info in
                if let info = info, !info.paymentId.isEmpty {
                    dataSource.sections += [
                        Section(rows: [
                            Row(text: Strings.RewardsInternals.title, selection: {
                                let controller = RewardsInternalsViewController(ledger: ledger, legacyLedger: self.legacyWallet)
                                self.navigationController?.pushViewController(controller, animated: true)
                            }, accessory: .disclosureIndicator)
                        ])
                    ]
                }
            }
        }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = Strings.braveRewardsTitle
        
        rewards.startLedgerService { [weak self] in
            guard let self = self else { return }
            if let legacyWallet = self.legacyWallet, !legacyWallet.isInitialized {
                legacyWallet.initializeLedgerService {
                    self.reloadSections()
                }
            } else {
                self.reloadSections()
            }
        }
    }
}
