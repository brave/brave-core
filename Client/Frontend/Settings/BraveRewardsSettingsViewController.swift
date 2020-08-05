// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import Static
import Shared
import BraveShared
import BraveRewards
import BraveRewardsUI
import DeviceCheck

private extension ContributionRetry {
    var name: String {
        switch self {
        case .stepCurrent: return "Current"
        case .stepFinal: return "Final"
        case .stepNo: return "No"
        case .stepPayload: return "Payload"
        case .stepPrepare: return "Prepare"
        case .stepProof: return "Proof"
        case .stepReconcile: return "Reconcile"
        case .stepRegister: return "Register"
        case .stepViewing: return "Viewing"
        case .stepVote: return "Vote"
        case .stepWinners: return "Winners"
        default: return "Unknown"
        }
    }
}

class BraveRewardsSettingsViewController: TableViewController {
    
    let rewards: BraveRewards
    
    var tappedShowRewardsSettings: (() -> Void)?
    
    init(_ rewards: BraveRewards) {
        self.rewards = rewards
        super.init(style: .grouped)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = Strings.braveRewardsTitle
        
        var hideIconRow = Row.boolRow(title: Strings.hideRewardsIcon, option: Preferences.Rewards.hideRewardsIcon)
        hideIconRow.detailText = Strings.hideRewardsIconSubtitle
        hideIconRow.cellClass = MultilineSubtitleCell.self
        
        dataSource.sections = [
            Section(rows: [
                hideIconRow
            ])
        ]
        
        if rewards.ledger.isWalletCreated {
            let dateFormatter = DateFormatter().then {
                $0.dateStyle = .short
            }
            var walletCreatedDate: String = "-"
            self.rewards.ledger.rewardsInternalInfo { info in
                guard let info = info else { return }
                walletCreatedDate = dateFormatter.string(from: Date(timeIntervalSince1970: TimeInterval(info.bootStamp)))
            }
            
            dataSource.sections += [
                Section(rows: [
                    Row(text: Strings.openBraveRewardsSettings, selection: { [unowned self] in
                        self.tappedShowRewardsSettings?()
                    }, image: RewardsPanelController.batLogoImage, cellClass: ButtonCell.self)
                ]),
                Section(rows: [
                    Row(text: Strings.walletCreationDate, detailText: walletCreatedDate, selection: {
                        let sheet = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
                        sheet.popoverPresentationController?.sourceView = self.view
                        sheet.popoverPresentationController?.sourceRect = self.view.bounds
                        sheet.addAction(UIAlertAction(title: Strings.copyWalletSupportInfo, style: .default, handler: { [unowned self] _ in
                            self.rewards.ledger.rewardsInternalInfo { info in
                                guard let info = info else { return }
                                let supportInfo = """
                                Device Status: \(DCDevice.current.isSupported ? "Supported" : "Not supported")
                                Enrollment State: \(DeviceCheckClient.isDeviceEnrolled() ? "Enrolled" : "Not enrolled")
                                Key Info Seed: \(info.isKeyInfoSeedValid ? "Valid" : "Invalid")
                                Wallet Payment ID: \(info.paymentId)
                                Wallet created: \(walletCreatedDate)
                                """
                                UIPasteboard.general.string = supportInfo
                            }
                        }))
                        sheet.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil))
                        self.present(sheet, animated: true)
                    })
                ])
            ]
        }
    }
}
