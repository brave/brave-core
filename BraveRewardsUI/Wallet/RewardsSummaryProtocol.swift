/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import BraveRewards

/// Shared resources for showing summary of all BAT rewards.
protocol RewardsSummaryProtocol {
  var state: RewardsState { get }
  
  /// Month and year of which the rewards summary is shown.
  var summaryPeriod: String { get }
  
  /// Rows showing different types of earnings, tips etc.
  var summaryRows: [RowView] { get }
  
  /// A view informing users about contributing to unverified publishers.
  var disclaimerView: LinkLabel? { get }
}

private struct Activity {
  let value: BATValue
  let title: String
  let color: UIColor
  init?(_ valueString: String, title: String, color: UIColor) {
    // Convert to double to avoid any issues with changing what the "0" string is (i.e. if it were
    // to change to "0.00")
    guard let value = BATValue(probi: valueString), value.doubleValue != 0.0 else {
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
  
  var summaryRows: [RowView] {
    let now = Date()
    guard let activityMonth = ActivityMonth(rawValue: now.currentMonthNumber) else {
      return []
    }
    
    let ledger = state.ledger
    var activities: [Activity] = []
    ledger.balanceReport(for: activityMonth, year: Int32(now.currentYear)) { balance in
      guard let balance = balance else { return }
      activities = [
        Activity(balance.grants, title: Strings.TotalGrantsClaimed, color: BraveUX.adsTintColor),
        Activity(balance.earningFromAds, title: Strings.EarningFromAds, color: BraveUX.adsTintColor),
        Activity(balance.autoContribute, title: Strings.AutoContribute, color: BraveUX.autoContributeTintColor),
        Activity(balance.oneTimeDonation, title: Strings.OneTimeTips, color: BraveUX.tipsTintColor),
        Activity(balance.recurringDonation, title: Strings.MonthlyTips, color: BraveUX.tipsTintColor)
      ].compactMap { $0 }
    }
    return activities.map {
      let bat = $0.value.displayString
      let usd = ledger.dollarStringForBATAmount(bat)
      return RowView(
        title: $0.title,
        cryptoValueColor: $0.color,
        batValue: bat,
        usdDollarValue: usd
      )
    }
  }
  
  var disclaimerView: LinkLabel? {
    let reservedAmount = BATValue(state.ledger.reservedAmount)
    // Don't show the view if there's no pending contributions.
    if reservedAmount.doubleValue <= 0 { return nil }
    
    let text = String(format: Strings.ContributingToUnverifiedSites, reservedAmount.displayString)
    
    return LinkLabel().then {
      $0.textColor = Colors.grey200
      $0.font = UIFont.systemFont(ofSize: 12.0)
      $0.textContainerInset = UIEdgeInsets(top: 8.0, left: 8.0, bottom: 8.0, right: 8.0)
      $0.text = "\(text) \(Strings.DisclaimerLearnMore)"
      $0.setURLInfo([Strings.DisclaimerLearnMore: "learn-more"])
      $0.backgroundColor = UIColor(white: 0.0, alpha: 0.04)
      $0.layer.cornerRadius = 4.0
    }
  }
}
