// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards
import Static
import DeviceCheck
import BraveRewardsUI
import Shared

private class WarningCell: MultilineSubtitleCell {
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        
        textLabel?.font = .systemFont(ofSize: 16.0, weight: .semibold)
        detailTextLabel?.font = .systemFont(ofSize: 15.0)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
}

/// A place where all rewards debugging information will live.
class RewardsInternalsViewController: TableViewController {
    
    private let rewards: BraveRewards
    private var internalsInfo: RewardsInternalsInfo?
    
    init(rewards: BraveRewards) {
        self.rewards = rewards
        if #available(iOS 13.0, *) {
            super.init(style: .insetGrouped)
        } else {
            super.init(style: .grouped)
        }
        rewards.ledger.rewardsInternalInfo { info in
            self.internalsInfo = info
        }
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = Strings.RewardsInternals.title
        
        guard let info = internalsInfo else { return }
        
        let dateFormatter = DateFormatter().then {
            $0.dateStyle = .short
        }
        let batFormatter = NumberFormatter().then {
            $0.minimumIntegerDigits = 1
            $0.minimumFractionDigits = 1
            $0.maximumFractionDigits = 3
        }
        
        navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .action, target: self, action: #selector(tappedShare)).then {
            $0.accessibilityLabel = Strings.RewardsInternals.shareInternalsTitle
        }
        
        var sections: [Static.Section] = [
            .init(
                rows: [
                    Row(text: Strings.RewardsInternals.sharingWarningTitle, detailText: Strings.RewardsInternals.sharingWarningMessage, cellClass: WarningCell.self)
                ]
            ),
            .init(
                header: .title(Strings.RewardsInternals.walletInfoHeader),
                rows: [
                    Row(text: Strings.RewardsInternals.keyInfoSeed, detailText: "\(info.isKeyInfoSeedValid ? Strings.RewardsInternals.valid : Strings.RewardsInternals.invalid)"),
                    Row(text: Strings.RewardsInternals.walletPaymentID, detailText: info.paymentId, cellClass: SubtitleCell.self),
                    Row(text: Strings.RewardsInternals.walletCreationDate, detailText: dateFormatter.string(from: Date(timeIntervalSince1970: TimeInterval(info.bootStamp))))
                ]
            ),
            .init(
                header: .title(Strings.RewardsInternals.deviceInfoHeader),
                rows: [
                    Row(text: Strings.RewardsInternals.status, detailText: DCDevice.current.isSupported ? Strings.RewardsInternals.supported : Strings.RewardsInternals.notSupported),
                    Row(text: Strings.RewardsInternals.enrollmentState, detailText: DeviceCheckClient.isDeviceEnrolled() ? Strings.RewardsInternals.enrolled : Strings.RewardsInternals.notEnrolled)
                ]
            )
        ]
        
        if let balance = rewards.ledger.balance {
            let keyMaps = [
                "anonymous": Strings.RewardsInternals.anonymous,
                "blinded": "Rewards \(Strings.BAT)"
            ]
            let walletRows = balance.wallets.lazy.filter({ $0.key != "uphold" }).map { (key, value) -> Row in
                Row(text: keyMaps[key] ?? key, detailText: "\(batFormatter.string(from: value) ?? "0.0") \(Strings.BAT)")
            }
            sections.append(
                .init(
                    header: .title(Strings.RewardsInternals.balanceInfoHeader),
                    rows: [
                        Row(text: Strings.RewardsInternals.totalBalance, detailText: "\(batFormatter.string(from: NSNumber(value: balance.total)) ?? "0.0") \(Strings.BAT)")
                    ] + walletRows
                )
            )
        }
        
        sections.append(
            .init(
                rows: [
//                    Row(text: Strings.RewardsInternals.logsTitle, selection: { [unowned self] in
//                        let controller = RewardsInternalsLogController(rewards: self.rewards)
//                        self.navigationController?.pushViewController(controller, animated: true)
//                    }, accessory: .disclosureIndicator),
                    Row(text: Strings.RewardsInternals.promotionsTitle, selection: { [unowned self] in
                        let controller = RewardsInternalsPromotionListController(rewards: self.rewards)
                        self.navigationController?.pushViewController(controller, animated: true)
                    }, accessory: .disclosureIndicator),
                    Row(text: Strings.RewardsInternals.contributionsTitle, selection: { [unowned self] in
                        let controller = RewardsInternalsContributionListController(rewards: self.rewards)
                        self.navigationController?.pushViewController(controller, animated: true)
                    }, accessory: .disclosureIndicator)
                ]
            )
        )
        
        dataSource.sections = sections
    }
    
    @objc private func tappedShare() {
        let controller = RewardsInternalsShareController(rewards: self.rewards, initiallySelectedSharables: RewardsInternalsSharable.default)
        let container = UINavigationController(rootViewController: controller)
        present(container, animated: true)
    }
}

/// A file generator that creates a JSON file containing basic information such as wallet info, device info
/// and balance info
struct RewardsInternalsBasicInfoGenerator: RewardsInternalsFileGenerator {
    func generateFiles(at path: String, using builder: RewardsInternalsSharableBuilder, completion: @escaping (Error?) -> Void) {
        // Only 1 file to make here
        var internals: RewardsInternalsInfo?
        builder.rewards.ledger.rewardsInternalInfo { info in
            internals = info
        }
        guard let info = internals else {
            completion(RewardsInternalsSharableError.rewardsInternalsUnavailable)
            return
        }
        
        let data: [String: Any] = [
            "Wallet Info": [
                "Key Info Seed": "\(info.isKeyInfoSeedValid ? "Valid" : "Invalid")",
                "Wallet Payment ID": info.paymentId,
                "Wallet Creation Date": builder.dateFormatter.string(from: Date(timeIntervalSince1970: TimeInterval(info.bootStamp)))
            ],
            "Device Info": [
                "DeviceCheck Status": DCDevice.current.isSupported ? "Supported" : "Not supported",
                "DeviceCheck Enrollment State": DeviceCheckClient.isDeviceEnrolled() ? "Enrolled" : "Not enrolled",
                "OS": "\(UIDevice.current.systemName) \(UIDevice.current.systemVersion)",
                "Model": UIDevice.current.model,
            ],
            "Balance Info": builder.rewards.ledger.balance?.wallets ?? ""
        ]
        
        do {
            try builder.writeJSON(from: data, named: "basic", at: path)
            completion(nil)
        } catch {
            completion(error)
        }
    }
}
