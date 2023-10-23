// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import Preferences
import Shared

class RewardsNotification: NSObject, BraveNotification {
  enum Action {
    /// The user opened the ad by either clicking on it directly or by swiping and clicking the "view" button
    case opened
    /// The user swiped the ad away
    case dismissed
    /// The user ignored the ad for a given amount of time for it to automatically dismiss
    case timedOut
  }
  
  var view: UIView
  var dismissAction: (() -> Void)?
  var id: String { ad.placementID }
  let ad: NotificationAd
  var dismissPolicy: DismissPolicy {
    guard let adView = view as? AdView else { return .automatic() }
    
    var dismissTimeInterval: TimeInterval = 30
    if !AppConstants.buildChannel.isPublic, let override = Preferences.Rewards.adsDurationOverride.value, override > 0 {
      dismissTimeInterval = TimeInterval(override)
    }
    return .automatic(after: dismissTimeInterval)
  }
  
  private let handler: (Action) -> Void
  
  func willDismiss(timedOut: Bool) {
    guard let adView = view as? AdView else { return }
    handler(timedOut ? .timedOut : .dismissed)
  }
  
  init(
    ad: NotificationAd,
    handler: @escaping (Action) -> Void
  ) {
    self.ad = ad
    self.view = AdView()
    self.handler = handler
    super.init()
    self.setup()
  }
  
  private func setup() {
    guard let adView = view as? AdView else { return }
    
    adView.adContentButton.titleLabel.text = ad.title
    adView.adContentButton.bodyLabel.text = ad.body
    
    adView.adContentButton.addTarget(self, action: #selector(tappedAdView(_:)), for: .touchUpInside)
  }
  
  @objc private func tappedAdView(_ sender: AdContentButton) {
    guard let adView = sender.superview as? AdView else { return }
    dismissAction?()
    handler(.opened)
  }
}

extension RewardsNotification {
  /// Display a "My First Ad" on a presenting controller and be notified if they tap it
  static func displayFirstAd(
    with presenter: BraveNotificationsPresenter,
    on presentingController: UIViewController,
    completion: @escaping (RewardsNotification.Action, URL) -> Void
  ) {
    let notification = NotificationAd.customAd(
      title: Strings.Ads.myFirstAdTitle,
      body: Strings.Ads.myFirstAdBody,
      url: "https://brave.com/my-first-ad"
    )
    
    guard let targetURL = URL(string: notification.targetURL) else {
      assertionFailure("My First Ad URL is not valid: \(notification.targetURL)")
      return
    }
    
    let rewardsNotification = RewardsNotification(ad: notification) { action in
      completion(action, targetURL)
    }
    presenter.display(notification: rewardsNotification, from: presentingController)
  }
}
