// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import UIKit
import BraveRewards
import Static
import Shared

extension PromotionStatus {
    fileprivate var displayText: String {
        let s = Strings.RewardsInternals.self
        switch self {
        case .active: return s.promotionStatusActive
        case .attested: return s.promotionStatusAttested
        case .corrupted: return s.promotionStatusCorrupted
        case .finished: return s.promotionStatusFinished
        case .over: return s.promotionStatusOver
        @unknown default:
            return "-"
        }
    }
}
extension PromotionType {
    fileprivate var displayText: String {
        switch self {
        case .ugp: return "UGP"
        case .ads: return "Ads"
        @unknown default:
            return "-"
        }
    }
}

class RewardsInternalsPromotionListController: TableViewController {
    private let rewards: BraveRewards
    
    init(rewards: BraveRewards) {
        self.rewards = rewards
        super.init(style: .grouped)
    }
    
    @available(*, unavailable)
    required init(coder: NSCoder) {
        fatalError()
    }
    
    override func viewDidLoad() {
        title = Strings.RewardsInternals.promotionsTitle
        
        navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .action, target: self, action: #selector(tappedShare)).then {
            $0.accessibilityLabel = Strings.RewardsInternals.shareInternalsTitle
        }
        
        rewards.ledger.updatePendingAndFinishedPromotions {
            self.reloadData()
        }
    }
    
    func reloadData() {
        let dateFormatter = DateFormatter().then {
            $0.dateStyle = .short
            $0.timeStyle = .short
        }
        let batFormatter = NumberFormatter().then {
            $0.minimumIntegerDigits = 1
            $0.minimumFractionDigits = 1
            $0.maximumFractionDigits = 3
        }
        
        let promotions = (rewards.ledger.pendingPromotions + rewards.ledger.finishedPromotions).sorted(by: { $0.claimedAt < $1.claimedAt })
        dataSource.sections = promotions.map { promo in
            var rows = [
                Row(text: Strings.RewardsInternals.status, detailText: promo.status.displayText),
                Row(text: Strings.RewardsInternals.amount, detailText: "\(batFormatter.string(from: NSNumber(value: promo.approximateValue)) ?? "0.0") \(Strings.BAT)"),
                Row(text: Strings.RewardsInternals.type, detailText: promo.type.displayText),
                Row(text: Strings.RewardsInternals.expiresAt, detailText: dateFormatter.string(from: Date(timeIntervalSince1970: TimeInterval(promo.expiresAt)))),
                Row(text: Strings.RewardsInternals.legacyPromotion, detailText: promo.legacyClaimed ? Strings.yes : Strings.no),
                Row(text: Strings.RewardsInternals.version, detailText: "\(promo.version)"),
            ]
            if promo.status == .finished {
                rows.append(contentsOf: [
                    Row(text: Strings.RewardsInternals.claimedAt, detailText: dateFormatter.string(from: Date(timeIntervalSince1970: TimeInterval(promo.claimedAt)))),
                    Row(text: Strings.RewardsInternals.claimID, detailText: promo.claimId, cellClass: SubtitleCell.self)
                ])
            }
            return .init(
                header: .title(promo.id),
                rows: rows,
                uuid: promo.id
            )
        }
    }
    
    @objc private func tappedShare() {
        let controller = RewardsInternalsShareController(rewards: self.rewards, initiallySelectedSharables: [.promotions])
        let container = UINavigationController(rootViewController: controller)
        present(container, animated: true)
    }
}

/// A file generator that creates JSON files containing all of the promotions that the user has claimed
/// or has pending to claim
struct RewardsInternalsPromotionsGenerator: RewardsInternalsFileGenerator {
    func generateFiles(at path: String, using builder: RewardsInternalsSharableBuilder, completion: @escaping (Error?) -> Void) {
        let ledger = builder.rewards.ledger
        ledger.updatePendingAndFinishedPromotions {
            let promotions = ledger.finishedPromotions + ledger.pendingPromotions
            let promos = promotions.map { promo -> [String: Any] in
                var data: [String: Any] = [
                    "ID": promo.id,
                    "Status": promo.status.displayText,
                    "Amount": promo.approximateValue,
                    "Type": promo.type.displayText,
                    "Expires at": builder.dateAndTimeFormatter.string(from: Date(timeIntervalSince1970: TimeInterval(promo.expiresAt))),
                    "Legacy promotion": promo.legacyClaimed ? "Yes" : "No",
                    "Version": promo.version,
                ]
                if promo.status == .finished {
                    data["Claimed at"] = builder.dateAndTimeFormatter.string(from: Date(timeIntervalSince1970: TimeInterval(promo.claimedAt)))
                    data["Claim ID"] = promo.claimId
                }
                return data
            }
            let data: [String: Any] = [
                "Promotions": promos
            ]
            do {
                try builder.writeJSON(from: data, named: "promotions", at: path)
                completion(nil)
            } catch {
                completion(error)
            }
        }
    }
}
