// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards
import Static
import DeviceCheck
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
    
    private let ledger: BraveLedger
    private var internalsInfo: RewardsInternalsInfo?
    
    init(ledger: BraveLedger) {
        self.ledger = ledger
        if #available(iOS 13.0, *) {
            super.init(style: .insetGrouped)
        } else {
            super.init(style: .grouped)
        }
        ledger.rewardsInternalInfo { info in
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
        
        navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .action, target: self, action: #selector(tappedShare)).then {
            $0.accessibilityLabel = Strings.RewardsInternals.shareInternalsTitle
        }
        
        let sections: [Static.Section] = [
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
        
        dataSource.sections = sections
    }
    
    @objc private func tappedShare() {
        let controller = RewardsInternalsShareController(ledger: self.ledger, initiallySelectedSharables: RewardsInternalsSharable.default)
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
        builder.ledger.rewardsInternalInfo { info in
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
            ]
        ]
        
        do {
            try builder.writeJSON(from: data, named: "basic", at: path)
            completion(nil)
        } catch {
            completion(error)
        }
    }
}
