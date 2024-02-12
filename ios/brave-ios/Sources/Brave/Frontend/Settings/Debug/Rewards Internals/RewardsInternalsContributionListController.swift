// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Static
import Shared

extension BraveCore.BraveRewards.RewardsType {
  fileprivate var displayText: String {
    let s = Strings.RewardsInternals.self
    switch self {
    case .autoContribute: return s.rewardsTypeAutoContribute
    case .oneTimeTip: return s.rewardsTypeOneTimeTip
    case .recurringTip: return s.rewardsTypeRecurringTip
    default:
      return "-"
    }
  }
}
extension BraveCore.BraveRewards.ContributionStep {
  fileprivate var displayText: String {
    let s = Strings.RewardsInternals.self
    switch self {
    case .stepAcOff:
      return s.contributionsStepACOff
    case .stepRewardsOff:
      return s.contributionsStepRewardsOff
    case .stepAcTableEmpty:
      return s.contributionsStepACTableEmpty
    case .stepNotEnoughFunds:
      return s.contributionsStepNotEnoughFunds
    case .stepFailed:
      return s.contributionsStepFailed
    case .stepCompleted:
      return s.contributionsStepCompleted
    case .stepNo:
      return Strings.no
    case .stepStart:
      return s.contributionsStepStart
    case .stepPrepare:
      return s.contributionsStepPrepare
    case .stepReserve:
      return s.contributionsStepReserve
    case .stepExternalTransaction:
      return s.contributionsStepExternalTransaction
    case .stepCreds:
      return s.contributionsStepCreds
    case .stepRetryCount:
      fallthrough
    @unknown default:
      return "-"
    }
  }
}
extension BraveCore.BraveRewards.ContributionProcessor {
  fileprivate var displayText: String {
    let s = Strings.RewardsInternals.self
    switch self {
    case .braveTokens: return s.contributionProcessorBraveTokens
    case .uphold: return s.contributionProcessorUphold
    case .none: return s.contributionProcessorNone
    case .bitflyer, .gemini: fallthrough
    @unknown default:
      return "-"
    }
  }
}

class RewardsInternalsContributionListController: TableViewController {
  private let rewardsAPI: BraveRewardsAPI
  private var contributions: [BraveCore.BraveRewards.ContributionInfo] = []

  init(rewardsAPI: BraveRewardsAPI) {
    self.rewardsAPI = rewardsAPI
    super.init(style: .grouped)
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    title = Strings.RewardsInternals.contributionsTitle

    navigationItem.rightBarButtonItem = UIBarButtonItem(barButtonSystemItem: .action, target: self, action: #selector(tappedShare)).then {
      $0.accessibilityLabel = Strings.RewardsInternals.shareInternalsTitle
    }

    rewardsAPI.allContributions { contributions in
      self.contributions = contributions
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

    dataSource.sections = contributions.map { cont in
      .init(
        header: .title(cont.contributionId),
        rows: [
          Row(text: Strings.RewardsInternals.createdAt, detailText: dateFormatter.string(from: Date(timeIntervalSince1970: TimeInterval(cont.createdAt)))),
          Row(text: Strings.RewardsInternals.type, detailText: cont.type.displayText),
          Row(text: Strings.RewardsInternals.amount, detailText: "\(batFormatter.string(from: NSNumber(value: cont.amount)) ?? "0.0") \(Strings.BAT)"),
          Row(text: Strings.RewardsInternals.step, detailText: cont.step.displayText),
          Row(text: Strings.RewardsInternals.retryCount, detailText: "\(cont.retryCount)"),
          Row(text: Strings.RewardsInternals.processor, detailText: cont.processor.displayText),
          cont.publishers.count > 1
            ? Row(
              text: Strings.RewardsInternals.publishers,
              selection: { [unowned self] in
                let controller = RewardsInternalsContributionPublishersListController(publishers: cont.publishers)
                self.navigationController?.pushViewController(controller, animated: true)
              }, accessory: .disclosureIndicator) : Row(text: Strings.RewardsInternals.publisher, detailText: cont.publishers.first?.publisherKey, cellClass: SubtitleCell.self),
        ],
        uuid: cont.contributionId
      )
    }
  }

  @objc private func tappedShare() {
    let controller = RewardsInternalsShareController(rewardsAPI: self.rewardsAPI, initiallySelectedSharables: [.contributions])
    let container = UINavigationController(rootViewController: controller)
    present(container, animated: true)
  }
}

/// A file generator that create a JSON file that contains all the contributions that user has made
/// including through auto-contribute, tips, etc.
struct RewardsInternalsContributionsGenerator: RewardsInternalsFileGenerator {
  func generateFiles(at path: String, using builder: RewardsInternalsSharableBuilder, completion: @escaping (Error?) -> Void) {
    let rewardsAPI = builder.rewardsAPI
    rewardsAPI.allContributions { contributions in
      let conts = contributions.map { cont -> [String: Any] in
        return [
          "ID": cont.contributionId,
          "Created at": builder.dateAndTimeFormatter.string(from: Date(timeIntervalSince1970: TimeInterval(cont.createdAt))),
          "Type": cont.type.displayText,
          "Amount": cont.amount,
          "Step": cont.step.displayText,
          "Retry Count": cont.retryCount,
          "Processor": cont.processor.displayText,
          "Publishers": cont.publishers.map { pub in
            return [
              "Publisher Key": pub.publisherKey,
              "Total Amount": pub.totalAmount,
              "Contributed Amount": pub.contributedAmount,
            ] as [String: Any]
          },
        ]
      }
      let data: [String: Any] = [
        "Contributions": conts
      ]
      do {
        try builder.writeJSON(from: data, named: "contributions", at: path)
        completion(nil)
      } catch {
        completion(error)
      }
    }
  }
}
