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

  init(_ rewards: BraveRewards, legacyWallet: BraveLedger?) {
    self.rewards = rewards
    self.legacyWallet = legacyWallet

    super.init(style: .insetGrouped)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  private func reloadSections() {
    dataSource.sections = [
      .init(
        rows: [
          Row(
            text: Strings.Rewards.settingsToggleTitle,
            detailText: Strings.Rewards.settingsToggleMessage,
            accessory: .switchToggle(value: rewards.isEnabled, { [unowned self] isOn in
              self.rewards.isEnabled = isOn
            }),
            cellClass: MultilineSubtitleCell.self)
        ],
        footer: .title(Strings.Rewards.settingsFooterMessage)
      )
    ]

    if let ledger = rewards.ledger {
      ledger.rewardsInternalInfo { [weak self] info in
        if let info = info, !info.paymentId.isEmpty {
          self?.dataSource.sections += [
            Section(rows: [
              Row(
                text: Strings.RewardsInternals.title,
                selection: {
                  guard let self = self else { return }
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
