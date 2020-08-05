/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveRewards
import BraveShared
import BraveUI

enum RewardsSummaryLink: String {
  case learnMore = "learn-more"
  case showPendingContributions = "show-pending-contributions"
}

/// Shared resources for showing summary of all BAT rewards.
protocol RewardsSummaryProtocol {
  var state: RewardsState { get }
  
  /// Month and year of which the rewards summary is shown.
  var summaryPeriod: String { get }
  
  /// Rows showing different types of earnings, tips etc.
  func summaryRows(_ completion: @escaping ([RowView]) -> Void)
  
  /// A view informing users about contributing to unverified publishers.
  func disclaimerLabels(for pendingContributionTotal: Double) -> [LinkLabel]
}

private struct Activity {
  let value: BATValue
  let title: String
  let color: UIColor
  init?(_ value: Double, title: String, color: UIColor) {
    // Convert to double to avoid any issues with changing what the "0" string is (i.e. if it were
    // to change to "0.00")
    let value = BATValue(value)
    guard value.doubleValue != 0.0 else {
      return nil
    }
    self.value = value
    self.title = title
    self.color = color
  }
}

extension RewardsSummaryProtocol {
  var summaryPeriod: String {
    let now = Date()
    return "\(now.currentMonthName().uppercased()) \(now.currentYear)"
  }
  
  func summaryRows(_ completion: @escaping ([RowView]) -> Void) {
    let now = Date()
    guard let activityMonth = ActivityMonth(rawValue: now.currentMonthNumber) else {
      completion([])
      return
    }
    
    let ledger = state.ledger
    ledger.balanceReport(for: activityMonth, year: Int32(now.currentYear)) { balance in
      guard let balance = balance else { return }
      let rows: [RowView] = [
        Activity(balance.grants, title: Strings.totalGrantsClaimed, color: BraveUX.adsTintColor),
        Activity(balance.earningFromAds, title: Strings.earningFromAds, color: BraveUX.adsTintColor),
        Activity(balance.autoContribute, title: Strings.autoContribute, color: BraveUX.autoContributeTintColor),
        Activity(balance.oneTimeDonation, title: Strings.oneTimeTips, color: BraveUX.tipsTintColor),
        Activity(balance.recurringDonation, title: Strings.monthlyTips, color: BraveUX.tipsTintColor)
      ]
      .compactMap { $0 }
      .map {
        let bat = $0.value.displayString
        let usd = ledger.dollarStringForBATAmount(bat)
        return RowView(
          title: $0.title,
          cryptoValueColor: $0.color,
          batValue: bat,
          usdDollarValue: usd
        )
      }
      completion(rows)
    }
  }
  
  func disclaimerLabels(for pendingContributionTotal: Double) -> [LinkLabel] {
    var labels: [LinkLabel] = []
    
    if Preferences.Rewards.isUsingBAP.value == true {
      labels.append(LinkLabel().then {
        $0.attributedText = {
          let str = NSMutableAttributedString(string: Strings.BATPointsDisclaimer, attributes: [.font: UIFont.systemFont(ofSize: 12.0)])
          if let range = str.string.range(of: Strings.BATPointsDisclaimerBoldedWords) {
            str.addAttribute(.font, value: UIFont.systemFont(ofSize: 12.0, weight: .semibold), range: NSRange(range, in: str.string))
          }
          return str
        }()
        $0.appearanceTextColor = Colors.grey700
      })
    }
    
    let reservedAmount = BATValue(pendingContributionTotal)
    // Don't show the view if there's no pending contributions.
    if reservedAmount.doubleValue > 0 {
      let batAmountText = "\(reservedAmount.displayString) \(Strings.BAT)"
      let text = String(format: Strings.contributingToUnverifiedSites, batAmountText)
      
      labels.append(LinkLabel().then {
        $0.appearanceTextColor = Colors.grey700
        $0.font = UIFont.systemFont(ofSize: 12.0)
        $0.text = "\(text) \(Strings.disclaimerLearnMore)"
        $0.setURLInfo([Strings.disclaimerLearnMore: RewardsSummaryLink.learnMore.rawValue])
      })
      
      labels.append(LinkLabel().then {
        $0.appearanceTextColor = Colors.grey700
        $0.font = UIFont.systemFont(ofSize: 12.0)
        $0.text = Strings.showAllPendingContributions
        $0.setURLInfo([Strings.showAllPendingContributions: RewardsSummaryLink.showPendingContributions.rawValue])
      })
    }
    
    return labels
  }
}
