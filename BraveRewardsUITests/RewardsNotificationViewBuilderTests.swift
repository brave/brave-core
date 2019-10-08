// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveRewards
import XCTest
@testable import BraveRewardsUI

class RewardsNotificationViewBuilderTests: XCTestCase {
  
  func testUnhandledNotifications() {
    // Notifications not handled:
    XCTAssertNil(RewardsNotificationViewBuilder.get(notification: notification(.adsLaunch)))
    XCTAssertNil(RewardsNotificationViewBuilder.get(notification: notification(.backupWallet)))
    XCTAssertNil(RewardsNotificationViewBuilder.get(notification: notification(.failedContribution)))
    XCTAssertNil(RewardsNotificationViewBuilder.get(notification: notification(.invalid)))
  }
  
  func testActionNotifiication() {
    XCTAssert(RewardsNotificationViewBuilder.get(notification: notification(.grant)) is WalletActionNotificationView)
    XCTAssert(RewardsNotificationViewBuilder.get(notification: notification(.tipsProcessed)) is WalletActionNotificationView)
    XCTAssert(RewardsNotificationViewBuilder.get(notification: notification(.verifiedPublisher, userInfo: ["name": "abc"])) is WalletActionNotificationView)
    XCTAssert(RewardsNotificationViewBuilder.get(notification: notification(.autoContribute, userInfo: ["result": 0, "amount": "1"])) is WalletActionNotificationView)
  }
  
  func testAlertNotification() {
    XCTAssert(RewardsNotificationViewBuilder.get(notification: notification(.insufficientFunds)) is WalletAlertNotificationView)
    XCTAssert(RewardsNotificationViewBuilder.get(notification: notification(.pendingNotEnoughFunds)) is WalletAlertNotificationView)
    XCTAssert(RewardsNotificationViewBuilder.get(notification: notification(.autoContribute, userInfo: ["result": 15, "amount": "1"])) is WalletAlertNotificationView)
    XCTAssert(RewardsNotificationViewBuilder.get(notification: notification(.autoContribute, userInfo: ["result": 16, "amount": "1"])) is WalletAlertNotificationView)
    XCTAssert(RewardsNotificationViewBuilder.get(notification: notification(.autoContribute, userInfo: ["result": 10, "amount": "1"])) is WalletAlertNotificationView)
  }
  
  private func notification(_ type: RewardsNotification.Kind, userInfo: [AnyHashable: Any]? = nil) -> RewardsNotification {
    return RewardsNotification(id: "1", dateAdded: Date().timeIntervalSince1970, kind: type, userInfo: userInfo)
  }
  
}
