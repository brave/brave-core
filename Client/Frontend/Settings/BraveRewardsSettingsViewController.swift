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

class BraveRewardsSettingsViewController: TableViewController {
    
    let rewards: BraveRewards
    
    var tappedShowRewardsSettings: (() -> Void)?
    
    init(_ rewards: BraveRewards) {
        self.rewards = rewards
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
        
        var hideIconRow = Row.boolRow(title: Strings.hideRewardsIcon, option: Preferences.Rewards.hideRewardsIcon)
        hideIconRow.detailText = Strings.hideRewardsIconSubtitle
        hideIconRow.cellClass = MultilineSubtitleCell.self
        
        dataSource.sections = [
            Section(rows: [
                hideIconRow
            ])
        ]
        
        if rewards.ledger.isWalletCreated {
            dataSource.sections += [
                Section(rows: [
                    Row(text: Strings.openBraveRewardsSettings, selection: { [unowned self] in
                        self.tappedShowRewardsSettings?()
                    }, cellClass: ButtonCell.self)
                ]),
                Section(rows: [
                    Row(text: Strings.RewardsInternals.title, selection: {
                        let controller = RewardsInternalsViewController(rewards: self.rewards)
                        self.navigationController?.pushViewController(controller, animated: true)
                    }, accessory: .disclosureIndicator)
                ])
            ]
        }
    }
}
